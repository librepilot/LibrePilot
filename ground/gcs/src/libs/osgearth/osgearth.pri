exists( $(OSG_SDK_DIR) ) {
	DEFINES += USE_OSG
	LIBS *= -l$$qtLibraryName(GCSOsgEarth)
}
exists( $(OSGEARTH_SDK_DIR) ) {
	DEFINES += USE_OSGEARTH
}
