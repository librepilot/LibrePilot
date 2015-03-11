import QtQuick 2.4
import osgQtQuick 1.0
import PfdQmlEnums 1.0

Item {

// unfortunately it is currently not possible to place a background image behind the osg scene
// the OSGViewport will completely overwrite the background image
// currently the background image is handled by OSGViewport directly (which offers, if needed, "real" anti-aliasing of scene and background) 
//Image {
//    anchors.fill: parent
//    source: "backgrounds/default_backgrounds.png"
//}

    OSGViewport {
        anchors.fill: parent
        focus: true
        sceneData: transformNode
        camera: camera

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