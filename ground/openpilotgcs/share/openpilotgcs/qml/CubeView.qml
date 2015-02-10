import QtQuick 2.0
import osgQtQuick 1.0

OSGViewport {
    id: fullview
    anchors.fill: parent
    focus: true
    //color: "transparent"
    //opacity: 0.999
    updateMode: OSGViewport.Discrete
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
