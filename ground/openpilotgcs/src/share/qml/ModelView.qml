import QtQuick 2.4

Item {
    Loader {
        anchors.fill: parent
        focus: true
        source: qmlWidget.terrainEnabled ? "model/ModelTerrainView.qml" : "model/ModelView.qml"
    }
}