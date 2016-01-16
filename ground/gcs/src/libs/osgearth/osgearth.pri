osg {
	DEFINES += USE_OSG
	LIBS *= -l$$qtLibraryName(GCSOsgEarth)
}

osgearth {
	DEFINES += USE_OSGEARTH
}
