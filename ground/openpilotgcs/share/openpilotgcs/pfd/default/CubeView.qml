import QtQuick 2.0
import osgQtQuick 1.0

OSGViewport {
    id: fullview
    anchors.fill: parent
    focus: true
    sceneData: cubeNode
    camera: camera
    //color: "transparent"
    //opacity: 0.999
    //drawingMode: OSGViewport.Buffer

    OSGCubeNode {
        id: cubeNode
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        //manipulatorMode: OSGCamera.Earth
    }
}
