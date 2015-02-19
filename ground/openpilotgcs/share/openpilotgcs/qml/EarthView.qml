import QtQuick 2.4
import osgQtQuick 1.0
import PfdQmlEnums 1.0

Item {
    OSGViewport {
        anchors.fill: parent
        focus: true
        sceneData: skyNode
        camera: camera

        OSGSkyNode {
            id: skyNode
            sceneData: terrainNode
            dateTime: new Date();
        }

        OSGFileNode {
            id: terrainNode
            source: qmlWidget.terrainFile
            async: false
        }

        OSGCamera {
            id: camera
            fieldOfView: 90
            manipulatorMode: OSGCamera.Earth
        }
    }
}
