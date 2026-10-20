#!/bin/sh
# Build the universal (Apple Silicon + Intel) filter and the installer package.
set -e
cd "$(dirname "$0")"
VERSION=${VERSION:-1.0.1}
rm -rf build && mkdir -p build/root/Library/Printers/HBPL1/Filter \
	"build/root/Library/Printers/PPDs/Contents/Resources"

clang -O2 -arch arm64 -arch x86_64 -mmacosx-version-min=10.13 \
	-Wall -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-result \
	-Wno-unused-function -Wno-unused-but-set-variable -Wno-sign-compare \
	-o build/root/Library/Printers/HBPL1/Filter/rastertohbpl1 \
	src/rastertohbpl1.c -lcups

cp "ppd/Dell 1250c.ppd" build/root/Library/Printers/PPDs/Contents/Resources/
gzip -9 -n "build/root/Library/Printers/PPDs/Contents/Resources/Dell 1250c.ppd"
cp COPYING build/root/Library/Printers/HBPL1/
chmod -R go-w build/root

pkgbuild --root build/root --identifier local.hbpl1.dell1250c \
	--version "$VERSION" --install-location / \
	--scripts pkg/scripts --ownership recommended \
	"build/Dell1250c-Driver-$VERSION.pkg"
