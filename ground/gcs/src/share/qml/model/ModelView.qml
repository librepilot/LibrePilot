import QtQuick 2.4
import OsgQtQuick 1.0

import "../uav.js" as UAV

Item {

    OSGViewport {
        anchors.fill: parent
        focus: true
        sceneData: sceneNode
        camera: camera

        OSGGroup {
            id: sceneNode
            children: [
                transformNode,
                backgroundNode
            ]
        }

        OSGBackgroundNode {
            id: backgroundNode
            imageFile: qmlWidget.backgroundImageFile
        }

        OSGTransformNode {
            id: transformNode
            modelData: fileNode
            rotate: Qt.vector3d(UAV.attitudePitch(), UAV.attitudeRoll(), -UAV.attitudeYaw())
            //scale: Qt.vector3d(0.001, 0.001, 0.001)
        }

        OSGFileNode {
            id: fileNode
            source: qmlWidget.modelFile
            async: false
            optimizeMode: OptimizeMode.OptimizeAndCheck
        }

        OSGCamera {
            id: camera
            fieldOfView: 90
            node: transformNode
        }
    }

}
