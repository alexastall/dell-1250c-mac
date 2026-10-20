/*
 * rastertohbpl1 - CUPS raster filter for HBPL version 1 printers
 * (Dell 1250c, Xerox Phaser 6000/6010, Dell C1660w/C1760nw,
 * Epson AcuLaser C1700, Fuji Xerox DocuPrint CP105b).
 *
 * macOS renders each page to an 8-bit sRGB or sGray CUPS raster at
 * 600 dpi; this filter feeds it to Dave Coffin's foo2hbpl1 encoder,
 * which is compiled in unchanged apart from its command-line main().
 *
 * Usage (as called by cupsd):
 *   rastertohbpl1 job user title copies options [file]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define main foo2hbpl1_main
#include "foo2hbpl1.c"
#undef main

#include <fcntl.h>
#include <signal.h>
#include <cups/cups.h>
#include <cups/raster.h>

static volatile sig_atomic_t Canceled = 0;

static void
cancel_job(int sig)
{
    (void)sig;
    Canceled = 1;
}

/* Copy a job string into PJL safely: no quotes, no control chars. */
static char *
pjl_safe(const char *in, char *out, size_t outlen)
{
    size_t i;

    for (i = 0; in && in[i] && i < outlen - 1; i++)
	out[i] = (in[i] == '"' || (unsigned char)in[i] < 0x20) ? '_' : in[i];
    out[i] = 0;
    return out;
}

/* Map the PPD MediaType choice to foo2hbpl1's media codes. */
static int
media_code(const char *type)
{
    static const struct { const char *name; int code; } map[] =
    {
	{ "Plain", 1 },     { "Bond", 2 },     { "LwCard", 3 },
	{ "LwGCard", 4 },   { "Labels", 5 },   { "Envelope", 6 },
	{ "Recycled", 7 },  { "PlainSide2", 8 },
    };
    size_t i;

    for (i = 0; i < sizeof map / sizeof *map; i++)
	if (!strcmp(type, map[i].name))
	    return map[i].code;
    return 0;	/* auto: plain, or envelope for envelope sizes */
}

/*
 * Build foo2hbpl1's page buffer from the raster, mirroring do_file():
 * row 0 is a guard row, pixel (r,c) lives at image + r*byte + (c+1)*deep,
 * colour pixels are stored K,C,M,Y and grey pixels as K.
 */
static int
encode_raster_page(cups_raster_t *ras, cups_page_header2_t *h)
{
    int rwide = h->cupsWidth, rhigh = h->cupsHeight;
    int iwide, ihigh, x0, y0;
    int nchan, deep, wide, byte, row, col, i, k;
    unsigned bpl = h->cupsBytesPerLine;
    unsigned char *image, *line, *sp, *dp;

    switch (h->cupsColorSpace)
    {
    case CUPS_CSPACE_W:
    case CUPS_CSPACE_SW:
    case CUPS_CSPACE_K:
	nchan = 1;
	deep = 1;
	break;
    case CUPS_CSPACE_RGB:
    case CUPS_CSPACE_SRGB:
	nchan = 3;
	deep = 4;
	break;
    case CUPS_CSPACE_CMYK:
	nchan = 4;
	deep = 4;
	break;
    default:
	fprintf(stderr, "ERROR: Unsupported colour space %d\n",
		h->cupsColorSpace);
	return -1;
    }
    if (h->cupsBitsPerColor != 8 || h->cupsColorOrder != CUPS_ORDER_CHUNKED
	|| h->HWResolution[0] != 600 || h->HWResolution[1] != 600
	|| rwide <= 0 || rhigh <= 0 || bpl < (unsigned)(rwide * nchan))
    {
	fprintf(stderr, "ERROR: Unsupported raster: %dx%d at %ux%u dpi, "
		"%d bits, order %d\n", rwide, rhigh, h->HWResolution[0],
		h->HWResolution[1], h->cupsBitsPerColor, h->cupsColorOrder);
	return -1;
    }

    /*
     * macOS rasterises only the imageable area; the printer wants the
     * whole sheet, so place the raster at its margin offset on a page.
     */
    iwide = (int)(h->cupsPageSize[0] * 600 / 72 + 0.5);
    ihigh = (int)(h->cupsPageSize[1] * 600 / 72 + 0.5);
    x0 = (int)(h->cupsImagingBBox[0] * 600 / 72 + 0.5);
    y0 = (int)((h->cupsPageSize[1] - h->cupsImagingBBox[3]) * 600 / 72 + 0.5);
    if (x0 < 0 || y0 < 0 || x0 + rwide > iwide || y0 + rhigh > ihigh)
    {
	iwide = rwide;
	ihigh = rhigh;
	x0 = y0 = 0;
    }

    wide = -(-iwide & -8);
    byte = wide * deep;
    image = calloc(ihigh + 2, byte);
    line = malloc(bpl);
    if (!image || !line)
    {
	fprintf(stderr, "ERROR: Out of memory for %dx%d page\n", iwide, ihigh);
	free(image);
	free(line);
	return -1;
    }

    for (row = 1; row <= ihigh; row++)
    {
	if (row <= y0 || row > y0 + rhigh)
	    continue;	/* outside the imageable area: left blank */
	if (Canceled || cupsRasterReadPixels(ras, line, bpl) != bpl)
	    memset(line, h->cupsColorSpace == CUPS_CSPACE_K ? 0 : 255, bpl);
	sp = line;
	dp = image + row*byte + x0*deep;
	for (col = 0; col < rwide; col++)
	{
	    dp += deep;
	    switch (h->cupsColorSpace)
	    {
	    case CUPS_CSPACE_W:
	    case CUPS_CSPACE_SW:
		*dp = ~*sp;
		break;
	    case CUPS_CSPACE_K:
		*dp = *sp;
		break;
	    case CUPS_CSPACE_CMYK:
		for (i = 0; i < 4; i++)
		    dp[i] = sp[(i-1) & 3];
		break;
	    default:	/* RGB: full-GCR separation as in foo2hbpl1, except */
		for (k = sp[2], i = 0; i < 2; i++)	/* pure black is K only: */
		    if (k < sp[i]) k = sp[i];		/* upstream laid 400% */
		*dp = ~k;				/* toner, smearing text */
		for (i = 0; i < 3; i++)
		    dp[i+1] = k ? (k - sp[i]) * 255 / k : 0;
		break;
	    }
	    sp += nchan;
	}
	for (i = 0; i < deep*Clip[0]; i++)
	    image[row*byte + deep+i] = 0;
	for (i = deep*(iwide-Clip[2]); i < byte; i++)
	    image[row*byte + deep+i] = 0;
    }
    memset(image+deep, 0, byte*(Clip[1]+1));
    memset(image+deep + byte*(ihigh-Clip[3]+1), 0, byte*Clip[3]);

    encode_page(deep > 1, iwide, ihigh, (char *) image);
    free(image);
    free(line);
    return 0;
}

int
main(int argc, char *argv[])
{
    cups_raster_t *ras;
    cups_page_header2_t header;
    char user[64], title[128];
    int fd = 0, page = 0, encoded = 0;

    if (argc < 6 || argc > 7)
    {
	fputs("Usage: rastertohbpl1 job user title copies options [file]\n",
	      stderr);
	return 1;
    }
    if (argc == 7 && (fd = open(argv[6], O_RDONLY)) < 0)
    {
	perror("ERROR: Unable to open raster file");
	return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, cancel_job);

    Username = pjl_safe(argv[2], user, sizeof user);
    Filename = pjl_safe(argv[3], title, sizeof title);

    ras = cupsRasterOpen(fd, CUPS_RASTER_READ);
    while (!Canceled && cupsRasterReadHeader2(ras, &header))
    {
	page++;
	fprintf(stderr, "PAGE: %d %d\n", page, header.NumCopies);
	fprintf(stderr, "INFO: Printing page %d\n", page);
	if (page == 1)
	    MediaCode = media_code(header.MediaType);
	if (encode_raster_page(ras, &header))
	    break;
	encoded++;
	fflush(stdout);
    }
    cupsRasterClose(ras);
    if (fd)
	close(fd);

    /* Trailer from foo2hbpl1's main(), once a job header has been sent */
    if (encoded)
    {
	puts("B\033%-12345X@PJL EOJ");
	fflush(stdout);
    }

    if (encoded == 0)
    {
	fputs("ERROR: No pages found\n", stderr);
	return 1;
    }
    fputs("INFO: Ready to print.\n", stderr);
    return Canceled ? 1 : 0;
}
