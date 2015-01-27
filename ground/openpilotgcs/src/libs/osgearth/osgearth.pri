OSG_SDK_DIR = $$(OSG_SDK_DIR)
!isEmpty(OSG_SDK_DIR) {
	CONFIG += osg
	DEFINES += USE_OSG
	LIBS *= -l$$qtLibraryName(GCSOsgEarth)
}
