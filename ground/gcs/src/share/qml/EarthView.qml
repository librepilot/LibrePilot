import QtQuick 2.4
import Pfd 1.0
import OsgQtQuick 1.0

import "common.js" as Utils

Item {
    OSGViewport {
        anchors.fill: parent
        focus: true
        sceneData: skyNode
        camera: camera

        OSGSkyNode {
            id: skyNode
            sceneData: terrainNode
            dateTime: Utils.getDateTime()
            minimumAmbientLight: qmlWidget.minimumAmbientLight
        }

        OSGFileNode {
            id: terrainNode
            source: qmlWidget.terrainFile
            async: false
        }

        OSGCamera {
            id: camera
            fieldOfView: 90
            manipulatorMode: ManipulatorMode.Earth
        }

    }
}
