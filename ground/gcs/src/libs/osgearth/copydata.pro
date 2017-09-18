#
# copy osg and osgearth libraries and data to build dir
#

OSG_VERSION = $$system(osgversion --version-number)

contains(QT_ARCH, x86_64)  {
    LIB_DIR_NAME = lib64
} else {
    LIB_DIR_NAME = lib
}

# set debug suffix if needed
win32:CONFIG(debug, debug|release):DS = "d"

linux:osg {
    # copy osg libraries
    data_copy.commands += $(MKDIR) $$GCS_LIBRARY_PATH/osg $$addNewline()
    data_copy.commands += $(COPY_DIR) $$shell_quote($$OSG_SDK_DIR/$$LIB_DIR_NAME/)* $$shell_quote($$GCS_LIBRARY_PATH/osg/) $$addNewline()
}

linux:osgearth {
    # copy osgearth libraries
    data_copy.commands += $(MKDIR) $$GCS_LIBRARY_PATH/osg $$addNewline()
    data_copy.commands += $(COPY_DIR) $$shell_quote($$OSGEARTH_SDK_DIR/$$LIB_DIR_NAME/)* $$shell_quote($$GCS_LIBRARY_PATH/osg/) $$addNewline()
}

macx:osg {
    # copy osg libraries
    data_copy.commands += $(COPY_DIR) $$shell_quote($$OSG_SDK_DIR/lib/)* $$shell_quote($$GCS_LIBRARY_PATH/) $$addNewline()
}

macx:osgearth {
    # copy osgearth libraries
    data_copy.commands += $(COPY_DIR) $$shell_quote($$OSGEARTH_SDK_DIR/lib/)* $$shell_quote($$GCS_LIBRARY_PATH/) $$addNewline()
}

linux|macx {
    # add make target
    POST_TARGETDEPS += copydata

    data_copy.target = copydata
    QMAKE_EXTRA_TARGETS += data_copy
}

win32:osg {
    OSG_PLUGINS_DIR = $${OSG_SDK_DIR}/bin/osgPlugins-$${OSG_VERSION}

    # osg libraries
    OSG_LIBS += \
        libOpenThreads$${DS}.dll \
        libosg$${DS}.dll \
        libosgAnimation$${DS}.dll \
        libosgDB$${DS}.dll \
        libosgFX$${DS}.dll \
        libosgGA$${DS}.dll \
        libosgManipulator$${DS}.dll \
        libosgParticle$${DS}.dll \
        libosgPresentation$${DS}.dll \
        libosgShadow$${DS}.dll \
        libosgSim$${DS}.dll \
        libosgTerrain$${DS}.dll \
        libosgText$${DS}.dll \
        libosgUtil$${DS}.dll \
        libosgViewer$${DS}.dll \
        libosgVolume$${DS}.dll \
        libosgWidget$${DS}.dll

    osgQt:OSG_LIBS += \
        libosgQt$${DS}.dll

    for(lib, OSG_LIBS) {
        addCopyFileTarget($${lib},$${OSG_SDK_DIR}/bin,$${GCS_APP_PATH})
        addCopyDependenciesTarget($${lib},$${OSG_SDK_DIR}/bin,$${GCS_APP_PATH})
    }

    # osg plugins
    OSG_PLUGINS = \
        mingw_osgdb_3ds$${DS}.dll \
        mingw_osgdb_freetype$${DS}.dll \
        mingw_osgdb_jpeg$${DS}.dll \
        mingw_osgdb_osg$${DS}.dll \
        mingw_osgdb_png$${DS}.dll \
        mingw_osgdb_tiff$${DS}.dll \
        mingw_osgdb_zip$${DS}.dll \
        mingw_osgdb_serializers_osg$${DS}.dll

    # more osg plugins
    osg_more_plugins:OSG_PLUGINS = \
        mingw_osgdb_3dc$${DS}.dll \
        mingw_osgdb_ac$${DS}.dll \
        mingw_osgdb_bmp$${DS}.dll \
        mingw_osgdb_bsp$${DS}.dll \
        mingw_osgdb_bvh$${DS}.dll \
        mingw_osgdb_cfg$${DS}.dll \
        mingw_osgdb_curl$${DS}.dll \
        mingw_osgdb_dds$${DS}.dll \
        mingw_osgdb_dot$${DS}.dll \
        mingw_osgdb_dw$${DS}.dll \
        mingw_osgdb_dxf$${DS}.dll \
        mingw_osgdb_gdal$${DS}.dll \
        mingw_osgdb_glsl$${DS}.dll \
        mingw_osgdb_gz$${DS}.dll \
        mingw_osgdb_hdr$${DS}.dll \
        mingw_osgdb_ive$${DS}.dll \
        mingw_osgdb_ktx$${DS}.dll \
        mingw_osgdb_logo$${DS}.dll \
        mingw_osgdb_lwo$${DS}.dll \
        mingw_osgdb_lws$${DS}.dll \
        mingw_osgdb_md2$${DS}.dll \
        mingw_osgdb_mdl$${DS}.dll \
        mingw_osgdb_normals$${DS}.dll \
        mingw_osgdb_obj$${DS}.dll \
        mingw_osgdb_ogr$${DS}.dll \
        mingw_osgdb_openflight$${DS}.dll \
        mingw_osgdb_osc$${DS}.dll \
        mingw_osgdb_osga$${DS}.dll \
        mingw_osgdb_osgshadow$${DS}.dll \
        mingw_osgdb_osgterrain$${DS}.dll \
        mingw_osgdb_osgtgz$${DS}.dll \
        mingw_osgdb_osgviewer$${DS}.dll \
        mingw_osgdb_p3d$${DS}.dll \
        mingw_osgdb_pic$${DS}.dll \
        mingw_osgdb_ply$${DS}.dll \
        mingw_osgdb_pnm$${DS}.dll \
        mingw_osgdb_pov$${DS}.dll \
        mingw_osgdb_pvr$${DS}.dll \
        mingw_osgdb_revisions$${DS}.dll \
        mingw_osgdb_rgb$${DS}.dll \
        mingw_osgdb_rot$${DS}.dll \
        mingw_osgdb_scale$${DS}.dll \
        mingw_osgdb_shp$${DS}.dll \
        mingw_osgdb_stl$${DS}.dll \
        mingw_osgdb_tga$${DS}.dll \
        mingw_osgdb_tgz$${DS}.dll \
        mingw_osgdb_trans$${DS}.dll \
        mingw_osgdb_trk$${DS}.dll \
        mingw_osgdb_txf$${DS}.dll \
        mingw_osgdb_txp$${DS}.dll \
        mingw_osgdb_vtf$${DS}.dll \
        mingw_osgdb_x$${DS}.dll \
        mingw_osgdb_serializers_osganimation$${DS}.dll \
        mingw_osgdb_serializers_osgfx$${DS}.dll \
        mingw_osgdb_serializers_osgga$${DS}.dll \
        mingw_osgdb_serializers_osgmanipulator$${DS}.dll \
        mingw_osgdb_serializers_osgparticle$${DS}.dll \
        mingw_osgdb_serializers_osgshadow$${DS}.dll \
        mingw_osgdb_serializers_osgsim$${DS}.dll \
        mingw_osgdb_serializers_osgterrain$${DS}.dll \
        mingw_osgdb_serializers_osgtext$${DS}.dll \
        mingw_osgdb_serializers_osgviewer$${DS}.dll \
        mingw_osgdb_serializers_osgvolume$${DS}.dll

    for(lib, OSG_PLUGINS) {
        addCopyFileTarget($${lib},$${OSG_PLUGINS_DIR},$${GCS_LIBRARY_PATH}/osg/osgPlugins-$${OSG_VERSION})
        addCopyDependenciesTarget($${lib},$${OSG_PLUGINS_DIR},$${GCS_APP_PATH})
    }
}

win32:osgearth {
    # osgearth libraries
    OSGEARTH_LIBS = \
        libosgEarth$${DS}.dll \
        libosgEarthAnnotation$${DS}.dll \
        libosgEarthFeatures$${DS}.dll \
        libosgEarthSymbology$${DS}.dll \
        libosgEarthUtil$${DS}.dll

    # loaded dynamically (probably by an osg plugin, need to find by which)
    OSGEARTH_LIBS += \
        libopenjp2-7.dll

    osgearthQt:OSGEARTH_LIBS += \
        libosgEarthQt$${DS}.dll

    for(lib, OSGEARTH_LIBS) {
        addCopyFileTarget($${lib},$${OSGEARTH_SDK_DIR}/bin,$${GCS_APP_PATH})
        addCopyDependenciesTarget($${lib},$${OSGEARTH_SDK_DIR}/bin,$${GCS_APP_PATH})
    }

    # osgearth plugins
    OSGEARTH_PLUGINS += \
        mingw_osgdb_earth$${DS}.dll \
        mingw_osgdb_osgearth_arcgis$${DS}.dll \
        mingw_osgdb_osgearth_engine_mp$${DS}.dll \
        mingw_osgdb_osgearth_sky_simple$${DS}.dll \
        mingw_osgdb_osgearth_tms$${DS}.dll \
        mingw_osgdb_osgearth_xyz$${DS}.dll \
        mingw_osgdb_osgearth_cache_filesystem$${DS}.dll

    # more osgearth plugins
    more_osgearth_plugins:OSGEARTH_PLUGINS += \
        mingw_osgdb_kml$${DS}.dll \
        mingw_osgdb_osgearth_agglite$${DS}.dll \
        mingw_osgdb_osgearth_arcgis_map_cache$${DS}.dll \
        mingw_osgdb_osgearth_bing$${DS}.dll \
        mingw_osgdb_osgearth_colorramp$${DS}.dll \
        mingw_osgdb_osgearth_debug$${DS}.dll \
        mingw_osgdb_osgearth_engine_byo$${DS}.dll \
        mingw_osgdb_osgearth_feature_ogr$${DS}.dll \
        mingw_osgdb_osgearth_feature_tfs$${DS}.dll \
        mingw_osgdb_osgearth_feature_wfs$${DS}.dll \
        mingw_osgdb_osgearth_gdal$${DS}.dll \
        mingw_osgdb_osgearth_label_annotation$${DS}.dll \
        mingw_osgdb_osgearth_mask_feature$${DS}.dll \
        mingw_osgdb_osgearth_model_feature_geom$${DS}.dll \
        mingw_osgdb_osgearth_model_feature_stencil$${DS}.dll \
        mingw_osgdb_osgearth_model_simple$${DS}.dll \
        mingw_osgdb_osgearth_noise$${DS}.dll \
        mingw_osgdb_osgearth_ocean_simple$${DS}.dll \
        mingw_osgdb_osgearth_osg$${DS}.dll \
        mingw_osgdb_osgearth_refresh$${DS}.dll \
        mingw_osgdb_osgearth_scriptengine_javascript$${DS}.dll \
        mingw_osgdb_osgearth_sky_gl$${DS}.dll \
        mingw_osgdb_osgearth_splat_mask$${DS}.dll \
        mingw_osgdb_osgearth_template_matclass$${DS}.dll \
        mingw_osgdb_osgearth_tilecache$${DS}.dll \
        mingw_osgdb_osgearth_tileindex$${DS}.dll \
        mingw_osgdb_osgearth_tileservice$${DS}.dll \
        mingw_osgdb_osgearth_vdatum_egm2008$${DS}.dll \
        mingw_osgdb_osgearth_vdatum_egm84$${DS}.dll \
        mingw_osgdb_osgearth_vdatum_egm96$${DS}.dll \
        mingw_osgdb_osgearth_vpb$${DS}.dll \
        mingw_osgdb_osgearth_wcs$${DS}.dll \
        mingw_osgdb_osgearth_wms$${DS}.dll \
        mingw_osgdb_osgearth_xyz$${DS}.dll \
        mingw_osgdb_osgearth_yahoo$${DS}.dll

    for(lib, OSGEARTH_PLUGINS) {
        addCopyFileTarget($${lib},$${OSG_PLUGINS_DIR},$${GCS_LIBRARY_PATH}/osg/osgPlugins-$${OSG_VERSION})
        addCopyDependenciesTarget($${lib},$${OSG_PLUGINS_DIR},$${GCS_APP_PATH})
    }
}
