###############################################################################
# General
###############################################################################

To enable the compilation of this library, use the following command:
    make config_append GCS_WITH_GSTREAMER=1

This will add the required options in the project config.

###############################################################################
# Windows (msys2)
###############################################################################

i686:
$ pacman -S mingw-w64-i686-gst-plugins-base mingw-w64-i686-gst-plugins-good mingw-w64-i686-gst-plugins-bad mingw-w64-i686-gst-plugins-ugly  mingw-w64-i686-gst-libav

x86_64:
$ pacman -S mingw-w64-x86_64-gst-plugins-base mingw-w64-x86_64-gst-plugins-good mingw-w64-x86_64-gst-plugins-bad mingw-w64-x86_64-gst-plugins-ugly mingw-w64-x86_64-gst-libav

###############################################################################
# Linux
###############################################################################

Get all the gstreamer libraries.

This might work:

Add the repository ppa:gstreamer-developers/ppa using Synaptic Package Manager or CLI
> sudo add-apt-repository ppa:gstreamer-developers/ppa
> sudo apt-get update

Upgrade to latest version of the packages using Synaptic Package Manager or CLI
> sudo apt-get install gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav
> sudo apt-get install gstreamer1.0-dev gstreamer-plugins-base1.0-dev

###############################################################################
# Mac
###############################################################################

brew install gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav libav x264

###############################################################################
# How to find required libraries (for copydata.pro)
###############################################################################

use gst-inspect with an element or plugin and look at Filename.

$ gst-inspect-1.0.exe ksvideosrc
Factory Details:
  Rank                     none (0)
  Long-name                KsVideoSrc
  Klass                    Source/Video
  Description              Stream data from a video capture device through Windows kernel streaming
  Author                   Ole André Vadla Ravnås <ole.andre.ravnas@tandberg.com>
Haakon Sporsheim <hakon.sporsheim@tandberg.com>
Andres Colubri <andres.colubri@gmail.com>

Plugin Details:
  Name                     winks
  Description              Windows kernel streaming plugin
  Filename                 C:\msys64\mingw64\lib\gstreamer-1.0\libgstwinks.dll
  Version                  1.6.3
  License                  LGPL
  Source module            gst-plugins-bad
  Source release date      2016-01-20
  Binary package           GStreamer
  Origin URL               http://gstreamer.net/
