exists( $(OSG_SDK_DIR) ) {
	CONFIG += osg
	DEFINES += USE_OSG
	LIBS *= -l$$qtLibraryName(GCSOsgEarth)
}
