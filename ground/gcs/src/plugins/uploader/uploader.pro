TEMPLATE = lib
TARGET = Uploader

DEFINES += UPLOADER_LIBRARY

QT += svg serialport

include(uploader_dependencies.pri)
include(../../libs/version_info/version_info.pri)

macx {
    QMAKE_CXXFLAGS  += -fpermissive
}

!macx {
    QMAKE_CXXFLAGS += -Wno-enum-compare
}

HEADERS += \
    uploadergadget.h \
    uploadergadgetconfiguration.h \
    uploadergadgetfactory.h \
    uploadergadgetoptionspage.h \
    uploadergadgetwidget.h \
    uploaderplugin.h \
    dfu.h \
    devicewidget.h \
    SSP/port.h \
    SSP/qssp.h \
    SSP/qsspt.h \
    SSP/common.h \
    runningdevicewidget.h \
    uploader_global.h \
    enums.h \
    rebootdialog.h

SOURCES += \
    uploadergadget.cpp \
    uploadergadgetconfiguration.cpp \
    uploadergadgetfactory.cpp \
    uploadergadgetoptionspage.cpp \
    uploadergadgetwidget.cpp \
    uploaderplugin.cpp \
    dfu.cpp \
    devicewidget.cpp \
    SSP/port.cpp \
    SSP/qssp.cpp \
    SSP/qsspt.cpp \
    runningdevicewidget.cpp \
    rebootdialog.cpp

OTHER_FILES += Uploader.pluginspec

FORMS += \
    uploader.ui \
    devicewidget.ui \
    runningdevicewidget.ui \
    rebootdialog.ui

RESOURCES += uploader.qrc

# TODO should use GCS_SYNTH_DIR... but that will break QtCreator  
exists( ../../../../../build/gcs-synthetics/fw_resource.qrc ) {
    RESOURCES += ../../../../../build/gcs-synthetics/fw_resource.qrc
} else {
    message("fw_resource.qrc is not available, automatic firmware upgrades are disabled")
}
