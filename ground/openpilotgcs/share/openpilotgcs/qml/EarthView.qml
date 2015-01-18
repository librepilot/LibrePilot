import QtQuick 2.0
import osgQtQuick 1.0
import PfdQmlEnums 1.0

OSGViewport {
    id: fullview
    anchors.fill: parent
    focus: true
    //color: "transparent"
    //opacity: 0.999
    updateMode: OSGViewport.Discrete
    camera: camera
    sceneData: skyNode

    OSGSkyNode {
        id: skyNode
        sceneData: terrainNode
    }

    OSGNodeFile {
        id: terrainNode
        source: qmlWidget.terrainFile
        async: false
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        manipulatorMode: OSGCamera.Earth
    }
}
