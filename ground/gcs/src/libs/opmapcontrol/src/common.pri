DESTDIR = ../build

QT += network
QT += sql
CONFIG += staticlib
TEMPLATE = lib
UI_DIR = uics
MOC_DIR = mocs
OBJECTS_DIR = objs
INCLUDEPATH +=../../../../libs/

# Stricter warnings turned on for OS X.
macx {
    CONFIG += warn_on
    QMAKE_CXXFLAGS_WARN_ON += -Werror
    QMAKE_CFLAGS_WARN_ON   += -Werror
}
