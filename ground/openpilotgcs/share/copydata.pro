include(../openpilotgcs.pri)

TEMPLATE = aux

DATACOLLECTIONS = cloudconfig default_configurations dials models qml sounds backgrounds diagrams mapicons stylesheets osgearth

equals(copydata, 1) {
    for(dir, DATACOLLECTIONS) {
        exists($$GCS_SOURCE_TREE/share/openpilotgcs/$$dir) {
            addCopyDirTarget($$dir, $$GCS_SOURCE_TREE/share/openpilotgcs, $$GCS_DATA_PATH)
        }
    }
}
