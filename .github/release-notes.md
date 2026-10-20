**One installer for every Mac.** It's a universal package that runs natively on both Apple Silicon and Intel, macOS 10.13 or later.

### Install
1. Download `Dell1250c-Driver-*.pkg` below.
2. The package is unsigned, so macOS will block the first double-click. Go to System Settings → Privacy & Security → **Open Anyway**. Or run this in Terminal:
   `sudo installer -pkg ~/Downloads/Dell1250c-Driver-*.pkg -target /`
3. Plug in the printer by USB and open System Settings → Printers & Scanners → Add Printer. Select the Dell. Under **Use**, choose **Select Software…** and pick **Dell 1250c Color Laser (HBPL v1)**.

This also works with the Xerox Phaser 6000. It's an unofficial driver, not made or supported by Dell or Xerox.
