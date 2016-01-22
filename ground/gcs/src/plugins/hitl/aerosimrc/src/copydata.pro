# Windows only

# set debug suffix if needed
win32:CONFIG(debug, debug|release):DS = "d"

win32 {
    # resources and sample configuration
    PLUGIN_RESOURCES = \
        cc_off.tga \
        cc_off_hover.tga \
        cc_on.tga \
        cc_on_hover.tga \
        cc_plugin.ini \
        plugin.txt

    for(res, PLUGIN_RESOURCES) {
        addCopyFileTarget($${res},$${RES_DIR},$${PLUGIN_DIR})
    }

    # Qt DLLs
    QT_DLLS = \
        Qt5Core$${DS}.dll \
        Qt5Network$${DS}.dll

    for(dll, QT_DLLS) {
        addCopyFileTarget($${dll},$$[QT_INSTALL_BINS],$${SIM_DIR})
    }

    # MinGW DLLs
    #MINGW_DLLS = \
    #             libgcc_s_dw2-1.dll \
    #             mingwm10.dll
    #for(dll, MINGW_DLLS) {
    #    addCopyFileTarget($${dll},$$(QTMINGW),$${SIM_DIR})
    #}
}
