import QtQuick 2.4
import Pfd 1.0
import OsgQtQuick 1.0

Item {
    OSGViewport {
        anchors.fill: parent
        focus: true
        sceneData: skyNode
        camera: camera

        OSGSkyNode {
            id: skyNode
            sceneData: terrainNode
            dateTime: getDateTime()
            minimumAmbientLight: qmlWidget.minimumAmbientLight

            function getDateTime() {
                switch(qmlWidget.timeMode) {
                case TimeMode.Local:
                    return new Date();
                case TimeMode.Predefined:
                    return qmlWidget.dateTime;
                }
            }

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
