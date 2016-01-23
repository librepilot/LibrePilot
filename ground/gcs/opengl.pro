# copy legacy OpenGL DLL

TEMPLATE = aux

include(gcs.pri)

MESAWIN_DIR = $$(MESAWIN_DIR)
isEmpty(MESAWIN_DIR):MESAWIN_DIR = $${TOOLS_DIR}/mesawin

# opengl32.dll will be copied to ./bin/opengl32/ for the installer to use
# the installer packages the dll and optionally allows to install it to ./bin/
# this implies that the opengl32.dll will not be used in a dev environment,
# unless it is copied to the ./bin/ ...

exists($${MESAWIN_DIR}) {
    contains(QT_ARCH, i386) {
        # take opengl32.dll from mesa (32 bit)
        OPENGL_DIR = $${MESAWIN_DIR}/opengl32_32
    }
    else {
        # take opengl32.dll from mesa (64 bit)
        OPENGL_DIR = $${MESAWIN_DIR}/opengl32_64
    }
}
else {
    # take opengl32.dll from mingw32/bin/
    # WARNING : doesn't currently work, GCS crashes at startup when using msys2 opengl32.dll
    warning("msys2 opengl32.dll breaks GCS, please install mesa with: make mesawin_install")
    # skip installing opengl32.dll, the package target will fail when creating the installer
    #OPENGL_DIR = $$[QT_INSTALL_BINS]
}

OPENGL_DLLS = \
    opengl32.dll

exists($${OPENGL_DIR}):for(dll, OPENGL_DLLS) {
    addCopyFileTarget($${dll},$${OPENGL_DIR},$${GCS_APP_PATH}/opengl32)
}
