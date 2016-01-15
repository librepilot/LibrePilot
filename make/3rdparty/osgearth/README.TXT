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
- clone git repositories in ./3rdparty
- build osg and osgEarth libraries in ./build/3rdparty
- install osg in ./build/3rdparty/install
- create distribution files (tar & md5) in ./build/3rdparty/install
