TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    version_info \
    qscispinbox\
    qtconcurrent \
    aggregation \
    extensionsystem \
    glc_lib \
    utils \
    opmapcontrol \
    qwt \
    sdlgamepad

exists( $(OSG_SDK_DIR) ) {
    SUBDIRS += osgearth
}
