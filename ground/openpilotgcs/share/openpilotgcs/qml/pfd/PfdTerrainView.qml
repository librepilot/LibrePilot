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
        manipulatorMode: OSGCamera.User

        yaw: AttitudeState.Yaw
        pitch: AttitudeState.Pitch
        roll: AttitudeState.Roll

        latitude: lat()
        longitude: lon()
        altitude: alt()

        function lat() {
            switch(qmlWidget.positionMode) {
            case Pfd.GPS:
                return GPSPositionSensor.Latitude / 10000000.0;
            case Pfd.Home:
                return HomeLocation.Latitude / 10000000.0;
            case Pfd.Predefined:
                return qmlWidget.latitude;
            }
        }

        function lon() {
            switch(qmlWidget.positionMode) {
            case Pfd.GPS:
                return GPSPositionSensor.Longitude / 10000000.0;
            case Pfd.Home:
                return HomeLocation.Longitude / 10000000.0;
            case Pfd.Predefined:
                return qmlWidget.longitude;
            }
        }

        function alt() {
            switch(qmlWidget.positionMode) {
            case Pfd.GPS:
                return GPSPositionSensor.Altitude;
            case Pfd.Home:
                return HomeLocation.Altitude;
            case Pfd.Predefined:
                return qmlWidget.altitude;
            }
        }

    }

    Rectangle {
        // using rectange instead of svg rendered to pixmap
        // as it's much more memory efficient
        id: world
        smooth: true
        opacity: 0

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "horizon")
        width: Math.round(sceneItem.width*scaledBounds.width/2)*2
        height: Math.round(sceneItem.height*scaledBounds.height/2)*3

        property double pitch1DegScaledHeight: (svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-90").y -
                                                svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch90").y)/180.0

        property double pitch1DegHeight: sceneItem.height*pitch1DegScaledHeight

        color: "red"

        transform: [
            Translate {
                id: pitchTranslate
                x: Math.round((world.parent.width - world.width)/2)
                // y is centered around world_center element
                y: Math.round(horizontCenter - world.height/2 +
                              AttitudeState.Pitch*world.pitch1DegHeight)
            },
            Rotation {
                angle: -AttitudeState.Roll
                origin.x : world.parent.width/2
                origin.y : horizontCenter
            }
        ]

    }

    Item {
        id: pitch_window
        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-window-terrain")

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)
        width: Math.floor(scaledBounds.width * sceneItem.width)
        height: Math.floor(scaledBounds.height * sceneItem.height)

        rotation: -AttitudeState.Roll
        transformOrigin: Item.Center

        smooth: true
        clip: true

        SvgElementImage {
            id: pitch_scale
            elementName: "pitch-scale"

            //worldView is loaded with Loader, so background element is visible
            sceneSize: background.sceneSize
            anchors.centerIn: parent
            //see comment for world transform
            anchors.verticalCenterOffset: AttitudeState.Pitch*world.pitch1DegHeight
            border: 64 //sometimes numbers are excluded from bounding rect

            smooth: true
        }

        SvgElementImage {
            id: horizont_line
            elementName: "center-line"

            opacity: 0.5

            //worldView is loaded with Loader, so background element is visible
            sceneSize: background.sceneSize
            anchors.centerIn: parent

            anchors.verticalCenterOffset: AttitudeState.Pitch*world.pitch1DegHeight
            border: 1
            smooth: true
       }

       SvgElementImage {
            id: pitch_0
            elementName: "pitch0"

            sceneSize: background.sceneSize
            anchors.centerIn: parent
            anchors.verticalCenterOffset: AttitudeState.Pitch*world.pitch1DegHeight
            border: 1
            smooth: true
       }
    }
}
