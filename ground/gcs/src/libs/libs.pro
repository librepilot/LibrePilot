TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    version_info \
    qscispinbox\
    aggregation \
    extensionsystem \
    utils \
    opmapcontrol \
    qwt \
    sdlgamepad

gstreamer:SUBDIRS += gstreamer

osg:SUBDIRS += osgearth
