defineReplace(cleanPath) {
    win32:1 ~= s|\\\\|/|g
    contains(1, ^/.*):pfx = /
    else:pfx =
    segs = $$split(1, /)
    out =
    for(seg, segs) {
        equals(seg, ..):out = $$member(out, 0, -2)
        else:!equals(seg, .):out += $$seg
    }
    return($$join(out, /, $$pfx))
}

defineReplace(addNewline) { 
    return($$escape_expand(\\n\\t))
}

defineReplace(qtLibraryName) {
   unset(LIBRARY_NAME)
   LIBRARY_NAME = $$1
   CONFIG(debug, debug|release) {
      !debug_and_release|build_pass {
          mac:RET = $$member(LIBRARY_NAME, 0)_debug
              else:win32:RET = $$member(LIBRARY_NAME, 0)d
      }
   }
   isEmpty(RET):RET = $$LIBRARY_NAME
   return($$RET)
}

defineTest(addCopyFileTarget) {
    file = $$1
    src  = $$2/$$1
    dest = $$3/$$1

    target = $${file}
    $${target}.target    = $$dest
    $${target}.depends   = $$src

    # create directory. Better would be an order only dependency
    $${target}.commands  = -@$(MKDIR) \"$$dirname(dest)\" $$addNewline()
    $${target}.commands += $(COPY_FILE) \"$$src\" \"$$dest\"

    QMAKE_EXTRA_TARGETS += $$target
    POST_TARGETDEPS += $$eval($${target}.target)

    export($${target}.target)
    export($${target}.depends)
    export($${target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(POST_TARGETDEPS)

    return(true)
}

defineTest(addCopyDependenciesTarget) {
    file = $$1
    src  = $$2/$$1
    dest = $$3

    target_file = $${OUT_PWD}/deps/$${file}.deps

    target = $${file}_deps
    $${target}.target    = $$target_file
    $${target}.depends   = $$src

    $${target}.commands  = -@$(MKDIR) \"$$dirname(target_file)\" $$addNewline()
    $${target}.commands  += $$(PYTHON) $$(ROOT_DIR)/make/copy_dependencies.py --dest \"$$dest\" --files \"$$src\" --excludes OPENGL32.DLL > \"$$target_file\"

    QMAKE_EXTRA_TARGETS += $$target
    POST_TARGETDEPS += $$eval($${target}.target)

    export($${target}.target)
    export($${target}.depends)
    export($${target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(POST_TARGETDEPS)

    return(true)
}

defineTest(addCopyDirTarget) {
    dir  = $$1
    src  = $$2/$$1
    dest = $$3/$$1

    target = $${dir}
    $${target}.target    = $$dest
    $${target}.depends   = $$src
    # Windows does not update directory timestamp if files are modified
    win32:$${target}depends += FORCE

    $${target}.commands  = @rm -rf \"$$dest\" $$addNewline()
    # create directory. Better would be an order only dependency
    $${target}.commands += -@$(MKDIR) \"$$dirname(dest)\" $$addNewline()
    $${target}.commands += $(COPY_DIR) \"$$src\" \"$$dest\"

    QMAKE_EXTRA_TARGETS += $$target
    POST_TARGETDEPS += $$eval($${target}.target)

    export($${target}.target)
    export($${target}.depends)
    export($${target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(POST_TARGETDEPS)

    return(true)
}

# For use in custom compilers which just copy files
win32:i_flag = i
defineReplace(stripSrcDir) {
    win32 {
        !contains(1, ^.:.*):1 = $$OUT_PWD/$$1
    } else {
        !contains(1, ^/.*):1 = $$OUT_PWD/$$1
    }
    out = $$cleanPath($$1)
    out ~= s|^$$re_escape($$_PRO_FILE_PWD_/)||$$i_flag
    return($$out)
}

isEmpty(TEST):CONFIG(debug, debug|release) {
    !debug_and_release|build_pass {
        TEST = 1
    }
}

equals(TEST, 1) {
    QT +=testlib
    DEFINES += WITH_TESTS
}

# don't build both debug and release
CONFIG -= debug_and_release

#ideally, we would want a qmake.conf patch, but this does the trick...
win32:!isEmpty(QMAKE_SH):QMAKE_COPY_DIR = cp -r -f

GCS_SOURCE_TREE = $$PWD
ROOT_DIR = $$GCS_SOURCE_TREE/../..

isEmpty(GCS_BUILD_TREE) {
    sub_dir = $$_PRO_FILE_PWD_
    sub_dir ~= s,^$$re_escape($$PWD),,
    GCS_BUILD_TREE = $$cleanPath($$OUT_PWD)
    GCS_BUILD_TREE ~= s,$$re_escape($$sub_dir)$,,
}

# Find the tools directory,
# try from Makefile (not run by Qt Creator),
isEmpty(TOOLS_DIR):TOOLS_DIR = $$(TOOLS_DIR)
isEmpty(TOOLS_DIR):TOOLS_DIR = $$clean_path($$ROOT_DIR/tools)

# Set the default name of the application
isEmpty(GCS_SMALL_NAME):GCS_SMALL_NAME = gcs

isEmpty(GCS_BIG_NAME) {
    GCS_BIG_NAME = GCS
} else {
    # Requote for safety and because of QTBUG-46224
    GCS_BIG_NAME = "$$GCS_BIG_NAME"
}

isEmpty(ORG_SMALL_NAME):ORG_SMALL_NAME = unknown

isEmpty(ORG_BIG_NAME) {
    ORG_BIG_NAME = Unknown
} else {
    # Requote for safety and because of QTBUG-46224
    ORG_BIG_NAME = "$$ORG_BIG_NAME"
}

isEmpty(WIKI_URL_ROOT) {
    WIKI_URL_ROOT = Unknown
} else {
    WIKI_URL_ROOT = "$$WIKI_URL_ROOT"
}

isEmpty(GCS_LIBRARY_BASENAME):GCS_LIBRARY_BASENAME = lib

macx {
    GCS_APP_TARGET   = $$GCS_BIG_NAME
    GCS_PATH = $$GCS_BUILD_TREE/$${GCS_APP_TARGET}.app/Contents
    GCS_APP_PATH = $$GCS_PATH/MacOS
    GCS_LIBRARY_PATH = $$GCS_PATH/Plugins
    GCS_PLUGIN_PATH  = $$GCS_LIBRARY_PATH
    GCS_QT_QML_PATH = $$GCS_PATH/Imports
    GCS_DATA_PATH    = $$GCS_PATH/Resources
    GCS_DOC_PATH     = $$GCS_DATA_PATH/doc
    copydata = 1
    copyqt = 1
} else {
    GCS_APP_TARGET = $$GCS_SMALL_NAME
    GCS_PATH         = $$GCS_BUILD_TREE
    GCS_APP_PATH     = $$GCS_PATH/bin
    GCS_LIBRARY_PATH = $$GCS_PATH/$$GCS_LIBRARY_BASENAME/$$GCS_SMALL_NAME
    GCS_PLUGIN_PATH  = $$GCS_LIBRARY_PATH/plugins
    GCS_DATA_PATH    = $$GCS_PATH/share/$$GCS_SMALL_NAME
    GCS_DOC_PATH     = $$GCS_PATH/share/doc

    !isEqual(GCS_SOURCE_TREE, $$GCS_BUILD_TREE):copydata = 1

    win32 {
        GCS_QT_PLUGINS_PATH = $$GCS_APP_PATH
        GCS_QT_QML_PATH = $$GCS_APP_PATH

        copyqt = $$copydata
    } else {
        GCS_QT_BASEPATH = $$GCS_LIBRARY_PATH/qt5
        GCS_QT_LIBRARY_PATH = $$GCS_QT_BASEPATH/lib
        GCS_QT_PLUGINS_PATH = $$GCS_QT_BASEPATH/plugins
        GCS_QT_QML_PATH = $$GCS_QT_BASEPATH/qml

        QT_INSTALL_DIR = $$clean_path($$[QT_INSTALL_LIBS]/../../../..)
        equals(QT_INSTALL_DIR, $$TOOLS_DIR) {
            copyqt = 1
        } else {
            copyqt = 0
        }
    }
}

INCLUDEPATH += \
    $$GCS_SOURCE_TREE/src/libs

DEPENDPATH += \
    $$GCS_SOURCE_TREE/src/libs

LIBS += -L$$GCS_LIBRARY_PATH

DEFINES += ORG_BIG_NAME=$$shell_quote(\"$$ORG_BIG_NAME\")
DEFINES += GCS_BIG_NAME=$$shell_quote(\"$$GCS_BIG_NAME\")
DEFINES += ORG_SMALL_NAME=$$shell_quote(\"$$ORG_SMALL_NAME\")
DEFINES += GCS_SMALL_NAME=$$shell_quote(\"$$GCS_SMALL_NAME\")
DEFINES += WIKI_URL_ROOT=$$shell_quote(\"$$WIKI_URL_ROOT\")

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
#DEFINES += QT_USE_FAST_OPERATOR_PLUS
#DEFINES += QT_USE_FAST_CONCATENATION

unix {
    CONFIG(debug, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    CONFIG(release, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    CONFIG(debug, debug|release):MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    CONFIG(release, debug|release):MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc
    UI_DIR = $${OUT_PWD}/.uic
}

linux-g++-* {
    # Bail out on non-selfcontained libraries. Just a security measure
    # to prevent checking in code that does not compile on other platforms.
    QMAKE_LFLAGS += -Wl,--allow-shlib-undefined -Wl,--no-undefined
}

win32 {
    # Fix ((packed)) pragma handling issue introduced when upgrading MinGW from 4.4 to 4.8
    # See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52991
    # The ((packet)) pragma is used in uav metadata struct and other places
    QMAKE_CXXFLAGS += -mno-ms-bitfields
}

# Explicit setting of C++11
CONFIG += c++11

address_sanitizer {
    # enable asan by adding "address_sanitizer" to your root config file
    # see https://github.com/google/sanitizers
    # see https://blog.qt.io/blog/2013/04/17/using-gccs-4-8-0-address-sanitizer-with-qt/
    #
    # to use, simply compile and run, asan will crash with a report if an error is found.
    # if you don't see symbols, try this: ./build/librepilot-gcs_debug/bin/librepilot-gcs 2>&1 | ./make/scripts/asan_symbolize.py
    #
    # Note: asan will apply only to GCS and not to third party libraries (Qt, osg, ...).

    QMAKE_CXXFLAGS += -fsanitize=address -g -fno-omit-frame-pointer
    QMAKE_CFLAGS += -fsanitize=address -g -fno-omit-frame-pointer
    QMAKE_LFLAGS += -fsanitize=address -g
}

# Stricter warnings turned on for OS X.
macx {
    CONFIG += warn_on
    !warn_off {
        QMAKE_CXXFLAGS_WARN_ON += -Werror
        QMAKE_CFLAGS_WARN_ON   += -Werror
        QMAKE_CXXFLAGS_WARN_ON += -Wno-gnu-static-float-init
    }
    # building with libc++ is needed when linking with osg/gdal
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++
}

# use ccache when available
QMAKE_CC = $$(CCACHE) $$QMAKE_CC
QMAKE_CXX = $$(CCACHE) $$QMAKE_CXX
