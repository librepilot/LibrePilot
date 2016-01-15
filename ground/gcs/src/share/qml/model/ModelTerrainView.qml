import QtQuick 2.4
import Pfd 1.0
import OsgQtQuick 1.0

import UAVTalk.AttitudeState 1.0
import UAVTalk.HomeLocation 1.0
import UAVTalk.GPSPositionSensor 1.0

import "../uav.js" as UAV

OSGViewport {
    anchors.fill: parent
    focus: true
    sceneData: skyNode
    camera: camera

    OSGSkyNode {
        id: skyNode
        sceneData: sceneGroup
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

    OSGGroup {
        id: sceneGroup
        children: [ terrainNode, modelNode ]
    }

    OSGFileNode {
        id: terrainNode
        source: qmlWidget.terrainFile
        async: false
    }

    OSGModelNode {
        id: modelNode
        clampToTerrain: true
        modelData: modelTransformNode
        sceneData: terrainNode

        attitude: UAV.attitude()
        position: UAV.position()
    }

    OSGTransformNode {
        id: modelTransformNode
        // model dimensions are in mm, scale to meters
        scale: Qt.vector3d(0.001, 0.001, 0.001)
        modelData: modelFileNode
    }

    OSGFileNode {
        id: modelFileNode
        source: qmlWidget.modelFile
        async: false
        optimizeMode: OptimizeMode.OptimizeAndCheck
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        logarithmicDepthBuffer: true
        manipulatorMode: ManipulatorMode.Track
        // use model to compute camera home position
        node: modelTransformNode
        // model will be tracked
        trackNode: modelTransformNode
    }
}
