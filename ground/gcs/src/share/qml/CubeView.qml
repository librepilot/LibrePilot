import QtQuick 2.4
import OsgQtQuick 1.0

Item {
    OSGViewport {
        anchors.fill: parent
        focus: true
        sceneData: cubeNode
        camera: camera

        OSGCubeNode {
            id: cubeNode
        }

        OSGCamera {
            id: camera
            fieldOfView: 90
        }
    }
}
