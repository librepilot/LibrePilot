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

osg {
    SUBDIRS += osgearth
}
