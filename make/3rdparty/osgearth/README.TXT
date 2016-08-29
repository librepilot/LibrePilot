This document describes how to build osg and osgEarth.

Building osgEarth requires:
* osgEarth (http://osgearth.org/)
* OpenSceneGraph (http://www.openscenegraph.org/)
* GDAL (http://www.gdal.org/)
* zlib, png, etc...

Make sure all LibrePilot SDKs are up to date as the osgEarth build relies on them.

More details can be found in osgearth.mk.


Linux prerequisites
----------------------------------

$ sudo apt-get install libzip-dev libpng-dev lipjpeg-dev libtiff5-dev libcurl4-openssl-dev
$ sudo apt-get install libgeos++-dev libgdal-dev

Alternative (not tested recently but could work):
$ sudo apt-get build-dep openscenegraph

Tested with:

$ curl --version
curl 7.35.0 (i686-pc-linux-gnu) libcurl/7.35.0 OpenSSL/1.0.1f zlib/1.2.8 libidn/1.28 librtmp/2.3

$ gdal-config --version
1.10.1


OSX prerequisites
----------------------------------

brew install cmake
brew install gdal

Windows prerequisites
----------------------------------

pacman -S mingw-w64-i686-cmake mingw-w64-i686-gdal-minimal

To build minimal gdal

From a MSYS shell:

$ pacman -S base-devel

Follow these instructions : http://sourceforge.net/p/msys2/wiki/Contributing%20to%20MSYS2/
With :
- LibrePilot package repository : ???

$ pacman -U mingw-w64-i686-gdal-minimal-2.0.1-2-any.pkg.tar.xz

Notes:
- uninstall any previously installed gdal package.

Todo
- declare provides=<original pkgname> and conflicts with origina gdal in PKGBUILD file
  (allows to substitute the minimal package when anything depends on the original package)

Building
----------------------------------

$ make all_osg

This will:
- clone osg and osgearth git repositories in ./3rdparty
- build osg and osgEarth libraries in ./build/3rdparty
- create distribution files (tar & md5) in ./build/3rdparty/install

Installing
----------------------------------

Copy the built osg and osgearth libraries into the tools directory:

cp -R build/3rdparty/install/osg*[.0-9][0-9] tools/

Configure LibrePilot GCS to use osg and osgearth:

echo "override GCS_EXTRA_CONF=osg osgearth copy_osg" > config

You'll then need to do a full rebuild of LibrePilot GCS.

Building LibrePilot GCS
----------------------------------

Clean and build all

make clean
make -j8 all

Replace the 8 in "-j8" with the number of CPU cores your PC has (maybe one more or less than your PC has).

Running LibrePilot GCS
----------------------------------

Start LibrePilot GCS with:

./build/librepilot-gcs_release/bin/librepilot-gcs -reset &

Only use the -reset option once (if at all).
It will reset your LibrePilot GCS setup to factory defaults which is necessary to see new gadget configuration options.
Your setup for workspaces, gadgets, scopes, etc.  will be lost.

In LibrePilot GCS, go to the Flight data workspace and use menu Window/Edit Gadgets Mode,
Select PFD gadget in one of the gadget panes and choose the PFD (ArcGis) or PFD (ReadyMap) configuration.
