import QtQuick 2.4
import Pfd 1.0
import OsgQtQuick 1.0

import "../common.js" as Utils
import "../uav.js" as UAV

OSGViewport {
    anchors.fill: parent
    focus: true
    sceneData: skyNode
    camera: camera

    OSGSkyNode {
        id: skyNode
        sceneData: sceneGroup
        dateTime: Utils.getDateTime()
        minimumAmbientLight: qmlWidget.minimumAmbientLight
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
