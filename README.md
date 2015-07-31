About the LibrePilot Project
============================

### Open - Collaborative - Free

The LibrePilot open source project was founded in July 2015. It focuses on
research and development of software and hardware to be used in a variety of
applications including vehicle control and stabilization, unmanned autonomous
vehicles and robotics. One of the project’s primary goals is to provide an open
and collaborative environment making it the ideal home for development of
innovative ideas.

LibrePilot welcomes and encourages exchange and collaboration with other
projects, like adding support for existing hardware or software in
collaboration under the spirit of open source.

LibrePilot finds its roots in the OpenPilot project and the founding members
are all long-standing contributors in that project.

The LibrePilot project will be governed by a board of members using consensual
methods to make important decisions and to set the overall direction of the
project.

The LibrePilot source code is released under the OSI approved GPLv3 license.
Integral text of the license can be found at [www.gnu.org](http://www.gnu.org/licenses/gpl-3.0.en.html)


Links for the LibrePilot Project
--------------------------------

- [Main project web site](https://www.librepilot.org)
- [Project forums](https://forum.librepilot.org)
- [Source code repository](https://bitbucket.org/librepilot)
- [Mirror](https://github.com/librepilot)
- [Issue tracker](https://librepilot.atlassian.net)
- [Gitter Chat](https://gitter.im/librepilot/LibrePilot)
- IRC: #LibrePilot on FreeNode

How to build from source?
-------------------------
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
