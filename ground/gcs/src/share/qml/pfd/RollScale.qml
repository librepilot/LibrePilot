import QtQuick 2.4

import UAVTalk.AttitudeState 1.0

import "../uav.js" as UAV

Item {
    id: sceneItem

    property variant sceneSize
    property real horizontCenter

    //onHorizontCenterChanged: console.log("horizont center:" + horizontCenter)

    SvgElementImage {
        id: rollscale
        elementName: "roll-scale"
        sceneSize: sceneItem.sceneSize

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        smooth: true

        // rotate it around the center of horizon
        transform: Rotation {
            angle: -UAV.attitudeRoll()
            origin.y : rollscale.height * 2.4
            origin.x : rollscale.width / 2
        }
    }

}
