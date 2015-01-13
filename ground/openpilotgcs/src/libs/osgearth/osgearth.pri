OSG_DIR = $$(OSG_DIR)

message(Using osg from here: $$OSG_DIR)

!isEmpty(OSG_DIR) {
	CONFIG += osg
	DEFINES += USE_OSG
	LIBS *= -l$$qtLibraryName(GCSOsgEarth)
}
