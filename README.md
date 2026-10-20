# Dell 1250c driver for macOS

A native macOS (Apple Silicon + Intel) print driver for the Dell 1250c and
its twin, the Xerox Phaser 6000. These printers speak **HBPL v1**, a
host-based language: the computer renders every page and the printer only
halftones and prints it, so there's no PostScript or PCL to fall back on.

How it works:

```
App → macOS (PDF → 600 dpi sRGB/grey raster) → rastertohbpl1 → USB → printer
```

`rastertohbpl1` is a small CUPS filter wrapped around Dave Coffin's
`foo2hbpl1` encoder (GPL v2+, from <https://dechifro.org/hbpl/>), which was
reverse-engineered from the same Windows driver family. No Ghostscript or
Homebrew is needed at print time.

This is an unofficial driver, not made or supported by Dell or Xerox.
It's been tested on a real Dell 1250c.

## Install

Download the latest `Dell1250c-Driver-*.pkg` from the
[Releases](../../releases/latest) page. It's one universal installer for
both Apple Silicon and Intel Macs. The package is unsigned, so macOS will
block the first double-click. Allow it under System Settings → Privacy &
Security → **Open Anyway**, or install it from Terminal:

```bash
sudo installer -pkg ~/Downloads/Dell1250c-Driver-1.0.1.pkg -target /
```

To build it yourself instead (needs the Xcode command-line tools):

```bash
./build.sh
```

Releases are built by GitHub Actions whenever a `v*` tag is pushed.

Then plug the printer in by USB and open System Settings → Printers &
Scanners → Add Printer. Select the Dell. Under **Use**, choose **Select
Software…** and pick **Dell 1250c Color Laser (HBPL v1)**.

### Printing from the other Macs

Install the driver on the Mac the printer is plugged into. Then turn on
System Settings → General → Sharing → Printer Sharing for it. The other
Macs will see the shared printer and need no driver at all.

## Options in the print dialog

- **Paper Size:** A4 (default), Letter, Legal, A5, B5 (JIS), Executive,
  Folio, Envelope #10, Monarch, C5, DL
- **Colour:** Colour or Black and White
- **Paper Type:** Automatic, Plain, Plain – Side 2, Recycled, Bond,
  Lightweight (Glossy) Cardstock, Labels, Envelope

## Uninstall

```bash
./uninstall.sh
```

## Changes to upstream foo2hbpl1.c

`src/foo2hbpl1.c` is upstream v1.3 with three small changes:

1. The paper-size table lookup stepped by 1 instead of 3 and read past the
   end of the table. It crashed on macOS.
2. The run-length search could read up to 17,057 pixels past the end of the
   page buffer. It's now bounded by the pixels left on the page.
3. It sends the Mac's hostname in the job header, as it did on Linux.

`src/rastertohbpl1.c` also changes upstream's RGB-to-CMYK conversion: pure
black prints with black toner only. Upstream added full cyan, magenta and
yellow under it (400% toner), which smeared black text and solid blacks.

## Testing without paper

`test/make-testpage.swift` draws a test PDF. Run it through
`cupsfilter -m application/vnd.cups-raster` and then `rastertohbpl1`. Then
decode the result with `hbpldecode` from the foo2zjs package to see exactly
what the printer would receive.

## Licence

GPL v2 or later. See `COPYING`.
