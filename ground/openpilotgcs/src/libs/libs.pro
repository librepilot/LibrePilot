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

OSG_DIR = $$(OSG_DIR)
!isEmpty(OSG_DIR) {
	SUBDIRS += osgearth
}
