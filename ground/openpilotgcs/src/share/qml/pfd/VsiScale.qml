import QtQuick 2.4

Item {
    id: sceneItem
    property variant sceneSize
    property real vert_velocity

    Timer {
         interval: 100; running: true; repeat: true
         onTriggered: vert_velocity = (0.9 * vert_velocity) + (0.1 * VelocityState.Down)
     }

    SvgElementImage {
        id: vsi_waypoint
        elementName: "vsi-waypoint"
        sceneSize: sceneItem.sceneSize

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        smooth: true
        visible: VelocityDesired.Down !== 0.0 && FlightStatus.FlightMode > 7

        //rotate it around the center
        transform: Rotation {
            angle: -VelocityDesired.Down * 5
            origin.y : vsi_waypoint.height / 2
            origin.x : vsi_waypoint.width * 33
        }
    }

    SvgElementImage {
        id: vsi_scale_meter

        visible: qmlWidget.altitudeUnit == "m"
        elementName: "vsi-scale-meter"
        sceneSize: sceneItem.sceneSize

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

    }

    SvgElementImage {
        id: vsi_scale_ft

        visible: qmlWidget.altitudeUnit == "ft"
        elementName: "vsi-scale-ft"
        sceneSize: sceneItem.sceneSize

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

    }

    SvgElementImage {
        id: vsi_arrow
        elementName: "vsi-arrow"
        sceneSize: sceneItem.sceneSize

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        smooth: true

        //rotate it around the center : limit value to +/-6m/s
        transform: Rotation {
            angle: vert_velocity > 6 ? -30 : vert_velocity < -6 ? 30 : -vert_velocity * 5
            origin.y : vsi_arrow.height / 2
            origin.x : vsi_arrow.width * 3.15
        }
    }

    SvgElementPositionItem {
        id: vsi_unit_text
        elementName: "vsi-unit-text"
        sceneSize: sceneItem.sceneSize

        Text {
            text: qmlWidget.altitudeUnit == "m" ? "m/s" : "ft/s"
            color: "cyan"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 1.7
                weight: Font.DemiBold
            }
            anchors.centerIn: parent
        }
    }

}
