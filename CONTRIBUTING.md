# How to build from source?

Both development environment and GCS are supported on Windows, Linux and Mac OS X.

# Install prerequisites

The first step is to Install all OS specific prerequisites.

## Mac OS X

Install XCode and its relatated command line tools (follow Apple documentation). Install git, curl and p7zip.

You can use brew with `brew install git curl p7zip` or macport with `sudo port install git curl p7zip`.


## Ubuntu

    sudo apt-get install git build-essential curl gdb wget debhelper p7zip-full unzip flex bison libsdl1.2-dev libudev-dev libusb-1.0-0-dev libc6-i386 mesa-common-dev


## Windows

### Install [Msys2](https://msys2.github.io/)

Follow the instructions on the web site. You can either install the i686 (Win32) or x86_64 (Win64) version.
It is recommended to go for the Win64 Shell if possible.
The x86_64 version has both Win32 and Win64 shells and it is possible to build both i686 and x86_64 applications.

### Add the LibrePilot MinGW package repository

Add the following lines at the end of your /etc/pacman.conf file:

    [librepilot-mingw]
    SigLevel = Optional TrustAll
    Server = http://download.librepilot.org/repo/mingw

### Install required packages

#### For i686 applications

Start a MinGW-w64 Win64 Shell or a MinGW-w64 Win32 Shell.

    pacman -Sy
    pacman -S --needed git unzip tar mingw-w64-i686-toolchain mingw-w64-i686-ccache mingw-w64-i686-ntldd mingw-w64-i686-qt5 mingw-w64-i686-SDL mingw-w64-i686-mesa mingw-w64-i686-openssl mingw-w64-i686-gdal-minimal mingw-w64-i686-OpenSceneGraph mingw-w64-i686-osgearth

Optionally install debug packages:

    pacman -S --needed mingw-w64-i686-OpenSceneGraph-debug mingw-w64-i686-osgearth-debug

#### For x86_64 applications

Start a MinGW-w64 Win64 Shell.

    pacman -Sy
    pacman -S --needed git unzip tar mingw-w64-x86_64-toolchain mingw-w64-x86_64-ccache mingw-w64-x86_64-ntldd mingw-w64-x86_64-qt5 mingw-w64-x86_64-SDL mingw-w64-x86_64-mesa mingw-w64-x86_64-openssl mingw-w64-x86_64-gdal-minimal mingw-w64-x86_64-OpenSceneGraph mingw-w64-x86_64-osgearth

Optionally install debug packages:

    pacman -S --needed mingw-w64-x86_64-OpenSceneGraph-debug mingw-w64-x86_64-osgearth-debug

**NOTE** On Windows you need to run the mingw version of make, which is `mingw32-make`


# Setup the build environment and build

The `all_sdk_install` target will automatically retrieve and install all needed tools (qt, arm gcc, etc.) in a local folder `/path/to/LibrePilot_root/tools`

## Ubuntu / Mac OS X

    make all_sdk_install
    make package

## Windows

    mingw32-make all_sdk_install
    mingw32-make package

The `package` target will build the complete installable package for the current platform.

You can build the `all` target to just build the software.

Run `make` with no arguments to show the complete list of supported targets.
