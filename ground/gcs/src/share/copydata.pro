include(../../gcs.pri)

TEMPLATE = aux

DATACOLLECTIONS = vehicletemplates configurations dials models backgrounds qml sounds diagrams mapicons stylesheets osgearth

equals(copydata, 1) {
    for(dir, DATACOLLECTIONS) {
        exists($$GCS_SOURCE_TREE/src/share/$$dir) {
            addCopyDirTarget($$dir, $$GCS_SOURCE_TREE/src/share, $$GCS_DATA_PATH)
        }
    }
}
