win32 {
    # copy SDL DLL
    SDL_DLLS = \
        SDL.dll

    for(dll, SDL_DLLS) {
        addCopyFileTarget($${dll},$$[QT_INSTALL_BINS],$${GCS_APP_PATH})
    }
}
