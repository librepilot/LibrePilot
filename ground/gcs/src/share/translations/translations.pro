include(../../../gcs.pri)

# Commented languages with outdated translations
# Allow removing the 'C' language in default config files at first start.
# Need to be uncommented for update in all languages files (make ts)
LANGUAGES = fr zh_CN # de es ru

# var, prepend, append
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

XMLPATTERNS = $$[QT_INSTALL_BINS]/xmlpatterns
LUPDATE = $$[QT_INSTALL_BINS]/lupdate -locations relative -no-ui-lines -no-sort
LRELEASE = $$[QT_INSTALL_BINS]/lrelease
LCONVERT = $$[QT_INSTALL_BINS]/lconvert

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/gcs_,.ts)

MIME_TR_H = $$PWD/mime_tr.h

for(dir, $$list($$files($$GCS_SOURCE_TREE/src/plugins/*))):MIMETYPES_FILES += $$files($$dir/*.mimetypes.xml)
MIMETYPES_FILES = \"$$join(MIMETYPES_FILES, \", \")\"
QMAKE_SUBSTITUTES += extract-mimetypes.xq.in
ts.commands += \
    $$XMLPATTERNS -output $$MIME_TR_H $$PWD/extract-mimetypes.xq && \
    (cd $$GCS_SOURCE_TREE && $$LUPDATE src $$MIME_TR_H -ts $$TRANSLATIONS) && \
    $$QMAKE_DEL_FILE $$MIME_TR_H

QMAKE_EXTRA_TARGETS += ts

TEMPLATE = app
TARGET = phony_target2
CONFIG -= qt sdk separate_debug_info gdb_dwarf_index
QT =
LIBS =

updateqm.input = TRANSLATIONS
updateqm.output = $$GCS_DATA_PATH/translations/${QMAKE_FILE_BASE}.qm
updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

QMAKE_LINK = @: IGNORE THIS LINE
OBJECTS_DIR =
win32:CONFIG -= embed_manifest_exe

qmfiles.files = $$prependAll(LANGUAGES, $$OUT_PWD/gcs_,.qm)
qmfiles.path = /share/gcs/translations
qmfiles.CONFIG += no_check_exist
INSTALLS += qmfiles

#========= begin block copying qt_*.qm files ==========
defineReplace(QtQmExists) {
    for(lang,$$1) {
        qm_file = $$[QT_INSTALL_TRANSLATIONS]/qt_$${lang}.qm
        exists($$qm_file) : result += $$qm_file
    }
    return($$result)
}
QT_TRANSLATIONS = $$QtQmExists(LANGUAGES)

copyQT_QMs.input = QT_TRANSLATIONS
copyQT_QMs.output = $$GCS_DATA_PATH/translations/${QMAKE_FILE_BASE}.qm
copyQT_QMs.variable_out = PRE_TARGETDEPS
copyQT_QMs.commands = $(COPY_FILE) ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copyQT_QMs.name = Copy ${QMAKE_FILE_IN}
copyQT_QMs.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += copyQT_QMs
#========= end block copying qt_*.qm files ============
