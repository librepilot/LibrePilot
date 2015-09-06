################################################################################
#!/bin/bash
################################################################################
#
# This script compiles GDAL and other dependencies needed by OSG and OSGEarth
#
# A good source for building with mingw :
#    http://www.gaia-gis.it/gaia-sins/mingw_how_to.html
#
################################################################################

################################################################################
# Environment
################################################################################

WORKING_DIR=$PWD

# TODO this should not be hardwired
ROOT_DIR=/d/Projects/LibrePilot

DOWNLOAD_DIR=$ROOT_DIR/downloads/osgearth
SOURCE_DIR=$ROOT_DIR/3rdparty/osgearth_dependencies
BUILD_DIR=$ROOT_DIR/build/3rdparty/osgearth_dependencies

HOST=mingw32

# list of libraries to build
# other candidates include bzip2, libxml2, gif, geotiff, ssl, gl...
BUILD_PKGCONFIG=1
BUILD_ZLIB=1
BUILD_LIBJPEG=1
BUILD_LIBPNG=1
BUILD_LIBTIFF=1
BUILD_FREETYPE=1
BUILD_OPENSSL=2
BUILD_CURL=1
BUILD_PROJ4=1
BUILD_GEOS=1
BUILD_GDAL=1

# TODO
#  libcurl needs to be built with ssl support
#  gdal does not seem to link with shared proj4

mkdir -p $SOURCE_DIR/
mkdir -p $BUILD_DIR/bin/
mkdir -p $BUILD_DIR/include/
mkdir -p $BUILD_DIR/lib/

# make sure all libraries see each others
export PATH=$BUILD_DIR/bin:$PATH
export CPATH=$BUILD_DIR/include
export LIBRARY_PATH=$BUILD_DIR/lib
export PKG_CONFIG_PATH=$BUILD_DIR/lib/pkgconfig

# reduce binary sizes by removing the -g option (not picked up by all libraries)
export CFLAGS=-O2
export CXXFLAGS=-O2

################################################################################
# pkg-config
# required by libcurl, gdal, osg, osgearth
################################################################################

if [ "$BUILD_PKGCONFIG" -eq 1 ]; then

echo "****************************************"
echo "Building pkg-config..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/pkg-config-0.28.tar.gz -C .
cd pkg-config-0.28

./configure --prefix=$BUILD_DIR --build=$HOST \
  --with-internal-glib
make
make install

fi

################################################################################
# ZLIB
# required by libcurl, gdal, osg, osgearth
################################################################################

if [ "$BUILD_ZLIB" -eq 1 ]; then

echo "****************************************"
echo "Building zlib..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/zlib-1.2.8.tar.gz -C .
cd zlib-1.2.8

make -f win32/Makefile.gcc clean
make -f win32/Makefile.gcc

cp -f zlib1.dll $BUILD_DIR/bin/
cp -f zconf.h $BUILD_DIR/include/
cp -f zlib.h $BUILD_DIR/include/
cp -f libz.a $BUILD_DIR/lib/
cp -f libz.dll.a $BUILD_DIR/lib/

fi

################################################################################
# LIBJPEG
# required by gdal, osg, osgearth
################################################################################

if [ "$BUILD_LIBJPEG" -eq 1 ]; then

echo "****************************************"
echo "Building libjpeg..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/jpegsrc.v9a.tar.gz -C .
cd jpeg-9a

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install-strip

fi

################################################################################
# LIBPNG
# required by gdal, osg, osgearth
################################################################################

if [ "$BUILD_LIBPNG" -eq 1 ]; then

echo "****************************************"
echo "Building libpng..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/libpng-1.6.14.tar.gz -C .
cd libpng-1.6.14

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install-strip

fi

################################################################################
# LIBTIFF
# reqires zlib
# required by gdal, osg, osgearth
################################################################################

if [ "$BUILD_LIBTIFF" -eq 1 ]; then

echo "****************************************"
echo "Building libtiff..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/tiff-4.0.3.tar.gz -C .
cd tiff-4.0.3

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install-strip

fi

################################################################################
# FreeType
################################################################################

if [ "$BUILD_FREETYPE" -eq 1 ]; then

echo "****************************************"
echo "Building FreeType..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/freetype-2.5.3.tar.gz -C .
cd freetype-2.5.3

./configure --prefix=$BUILD_DIR
make
make install

fi

################################################################################
# OpenSSL
################################################################################

if [ "$BUILD_OPENSSL" -eq 1 ]; then

echo "****************************************"
echo "Building OpenSSL..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/curl-7.38.0.tar.gz -C .
cd curl-7.38.0

./configure --prefix=$BUILD_DIR --build=$HOST
make
make install

fi

################################################################################
# cURL
# required by gdal, osgearth
################################################################################

if [ "$BUILD_CURL" -eq 1 ]; then

echo "****************************************"
echo "Building cURL..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/curl-7.38.0.tar.gz -C .
cd curl-7.38.0

./configure --prefix=$BUILD_DIR --build=$HOST \
  --enable-shared=yes --with-zlib=$BUILD_DIR
make
make install-strip

fi

################################################################################
# PROJ.4
# required by osgearth
################################################################################

if [ "$BUILD_PROJ4" -eq 1 ]; then

echo "****************************************"
echo "Building PROJ.4..."
echo "****************************************"

cd $SOURCE_DIR
tar -xzf $DOWNLOAD_DIR/proj-4.8.0.tar.gz -C .
tar -xzf $DOWNLOAD_DIR/proj-datumgrid-1.5.tar.gz -C proj-4.8.0/nad/
cd proj-4.8.0

./configure --prefix=$BUILD_DIR --build=$HOST \
  --enable-static=no --enable-shared=yes
make
make install

fi

################################################################################
# GEOS
# required by gdal
################################################################################

if [ "$BUILD_GEOS" -eq 1 ]; then

echo "****************************************"
echo "Building GEOS..."
echo "****************************************"

cd $SOURCE_DIR
tar xjf $DOWNLOAD_DIR/geos-3.3.8.tar.bz2 -C .
cd geos-3.3.8

# TODO why --disable-inline?
./configure --prefix=$BUILD_DIR --build=$HOST \
  --enable-static=no --enable-shared=yes --disable-inline
make
make install

fi

################################################################################
# GDAL
# requires zlib, libcurl, libpng, libjpeg, libtiff, geos
# required by osgearth
################################################################################

if [ "$BUILD_GDAL" -eq 1 ]; then

echo "****************************************"
echo "Building GDAL..."
echo "****************************************"

cd $SOURCE_DIR
tar xzf $DOWNLOAD_DIR/gdal-1.10.1.tar.gz -C .
cd gdal-1.10.1

# fix GNUmakefile as described here http://trac.osgeo.org/gdal/wiki/BuildingWithMinGW
patch < $WORKING_DIR/gdal_GNUmakefile_fix.diff

./configure --prefix=$BUILD_DIR --build=$HOST \
  --without-python --without-libtool \
  --with-xerces=no \
  --with-libz=$BUILD_DIR --with-curl=$BUILD_DIR \
  --with-png=$BUILD_DIR --with-jpeg=$BUILD_DIR --with-libtiff=$BUILD_DIR \
  --with-geos=$BUILD_DIR/bin/geos-config
make
make install

fi

################################################################################
