TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    version_info \
    qscispinbox\
    qtconcurrent \
    aggregation \
    extensionsystem \
    utils \
    opmapcontrol \
    qwt \
    sdlgamepad

OSG_SKD_DIR = $$(OSG_SKD_DIR)
!isEmpty(OSG_SKD_DIR) {
	SUBDIRS += osgearth
}
