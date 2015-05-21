import QtQuick 2.4
import osgQtQuick 1.0
import PfdQmlEnums 1.0

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
            rotate: Qt.vector3d(AttitudeState.Pitch, AttitudeState.Roll, -AttitudeState.Yaw)
            //scale: Qt.vector3d(0.001, 0.001, 0.001)
        }

        OSGFileNode {
            id: fileNode
            source: qmlWidget.modelFile
            async: false
            optimizeMode: OSGFileNode.OptimizeAndCheck
        }

        OSGCamera {
            id: camera
            fieldOfView: 90
            node: transformNode
        }
    }

}