include(../../openpilotgcs.pri)

TEMPLATE = aux

DATACOLLECTIONS = cloudconfig configurations dials models qml sounds backgrounds diagrams mapicons stylesheets osgearth

equals(copydata, 1) {
    for(dir, DATACOLLECTIONS) {
        exists($$GCS_SOURCE_TREE/src/share/$$dir) {
            addCopyDirTarget($$dir, $$GCS_SOURCE_TREE/src/share, $$GCS_DATA_PATH)
        }
    }
}
