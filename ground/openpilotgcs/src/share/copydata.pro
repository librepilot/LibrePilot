include(../../openpilotgcs.pri)

TEMPLATE = aux

DATACOLLECTIONS = cloudconfig configurations dials models qml sounds backgrounds diagrams mapicons stylesheets osgearth

equals(copydata, 1) {
    for(dir, DATACOLLECTIONS) {
        exists($$GCS_SOURCE_TREE/src/share/$$dir) {
            addCopyDirFilesTargets($$GCS_SOURCE_TREE/src/share/$$dir, $$GCS_DATA_PATH/$$dir)
        }
    }
}
