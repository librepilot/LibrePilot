import QtQuick 2.0
import osgQtQuick 1.0
import PfdQmlEnums 1.0

OSGViewport {
    anchors.fill: parent
    focus: true
    sceneData: fileNode
    camera: camera

    OSGNodeFile {
        id: fileNode
        source: qmlWidget.modelFile
        async: false
        optimizeMode: OSGNodeFile.OptimizeAndCheck
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        //manipulatorMode: OSGCamera.Track
        //trackNode: fileNode
    }
}
