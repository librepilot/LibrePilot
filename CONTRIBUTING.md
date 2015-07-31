How to build from source?
=========================

Both development environment and GCS are supported on Windows, Linux and Mac OS X

The first step is to Install all OS specific prerequisites.
###Mac OS X
Install XCode and its relatated command line tools (follow Apple documentation).
Install git, curl and p7zip. You can use brew `brew install git curl p7zip` or macport: `sudo port install git curl p7zip`
###Ubuntu

    sudo apt-get install git build-essentials curl gdb wget debhelper p7zip-full unzip flex bison libsdl1.2-dev  libudev-dev  libusb-1.0-0-dev libc6-i386 mesa-common-dev


###Windows
Install [msysGIT](https://msysgit.github.io/) under `C:\git`

Clone LibrePilot Git repository.
Open Git Bash and run

    cd /path/to/LibrePilot_root
    ./make/scripts/win_sdk_install.sh

You can build using the `/path/to/LibrePilot_root/make/winx86/bin/make` wrapper to call `mingw32-make.exe` as:

    ./make/winx86/bin/make  all_sdk_install
or call `mingw32-make` directly

    mingw32-make  all_sdk_install

##Setup the build environment and build
The `all_sdk_install` target will automatically retrieve and install all needed tools (qt, arm gcc etc.) in a local folder `/path/to/LibrePilot_root/tools`


    make all_sdk_install
    make package

The `package` target will build the complete installable package for the current platform.

Run make with no arguments to show the complete list of supported targets.
