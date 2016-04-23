/*
 * Copyright (C) 2016 The LibrePilot Project
 * Contact: http://www.librepilot.org
 *
 * This file is part of LibrePilot GCS.
 *
 * LibrePilot GCS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LibrePilot GCS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LibrePilot GCS.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.4

import "../js/uav.js" as UAV

Item {
    id: sceneItem
    property variant sceneSize
    property real vert_velocity

    Timer {
         interval: 100; running: true; repeat: true
         onTriggered: vert_velocity = (0.9 * vert_velocity) + (0.1 * UAV.velocityStateDown())
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
        visible: (UAV.isFlightModeAssisted() && (UAV.velocityDesiredDown() != 0.0))

        // rotate it around the center
        transform: Rotation {
            angle: -UAV.velocityDesiredDown() * 5
            origin.y : vsi_waypoint.height / 2
            origin.x : vsi_waypoint.width * 33
        }
    }

    SvgElementImage {
        id: vsi_scale_meter

        visible: (pfdContext.altitudeUnit == "m")
        elementName: "vsi-scale-meter"
        sceneSize: sceneItem.sceneSize

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

    }

    SvgElementImage {
        id: vsi_scale_ft

        visible: (pfdContext.altitudeUnit == "ft")
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

        // rotate it around the center, limit value to +/-6m/s
        transform: Rotation {
            angle: (vert_velocity > 6) ? -30 : (vert_velocity < -6) ? 30 : -vert_velocity * 5
            origin.y : vsi_arrow.height / 2
            origin.x : vsi_arrow.width * 3.15
        }
    }

    SvgElementPositionItem {
        id: vsi_unit_text
        elementName: "vsi-unit-text"
        sceneSize: sceneItem.sceneSize

        Text {
            text: (pfdContext.altitudeUnit == "m") ? "m/s" : "ft/s"
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
