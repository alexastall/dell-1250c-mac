#!/bin/sh
# Remove the Dell 1250c driver. Delete the printer in System Settings first.
sudo rm -rf /Library/Printers/HBPL1 \
	"/Library/Printers/PPDs/Contents/Resources/Dell 1250c.ppd.gz"
sudo pkgutil --forget local.hbpl1.dell1250c 2>/dev/null
echo "Dell 1250c driver removed."
