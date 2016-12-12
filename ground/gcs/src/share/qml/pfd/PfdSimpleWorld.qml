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
    id: worldView
    property real horizontCenter : horizontCenterItem.horizontCenter

    Rectangle {
        // using rectange instead of svg rendered to pixmap
        // as it's much more memory efficient
        id: world
        smooth: true

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "horizon")
        width: Math.round(sceneItem.width * scaledBounds.width / 2) * 2
        height: Math.round(sceneItem.height * scaledBounds.height / 2) * 3

        property double pitch1DegScaledHeight: ((svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-90").y -
                                                 svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch90").y) * 1.03) / 180.0

        property double pitch1DegHeight: sceneItem.height*pitch1DegScaledHeight

        gradient: Gradient {
            GradientStop { position: 0.4000; color: "#013163" }
            GradientStop { position: 0.4999; color: "#0164CC" }
            GradientStop { position: 0.5001; color: "#653300" }
            GradientStop { position: 0.6000; color: "#3C1E00" }
        }

        transform: [
            Translate {
                id: pitchTranslate
                x: Math.round((world.parent.width - world.width) / 2)
                // y is centered around world_center element
                y: Math.round(horizontCenter - world.height / 2 + UAV.attitudePitch() * world.pitch1DegHeight)
            },
            Rotation {
                angle: -UAV.attitudeRoll()
                origin.x : world.parent.width / 2
                origin.y : horizontCenter
            }
        ]

        SvgElementImage {
            id: horizont_line
            elementName: "center-line"

            // worldView is loaded with Loader, so background element is visible
            sceneSize: background.sceneSize
            anchors.centerIn: parent
            border: 1
            smooth: true
        }

        SvgElementImage {
            id: pitch_0
            elementName: "pitch0"

            sceneSize: background.sceneSize
            anchors.centerIn: parent
            border: 1
            smooth: true
        }
    }

    Item {
        id: pitch_window
        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-window")

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)
        width: Math.floor(scaledBounds.width * sceneItem.width)
        height: Math.floor(scaledBounds.height * sceneItem.height)

        rotation: -UAV.attitudeRoll()
        transformOrigin: Item.Center

        smooth: true
        clip: true

        SvgElementImage {
            id: pitch_scale
            elementName: "pitch-scale"
            // worldView is loaded with Loader, so background element is visible
            sceneSize: background.sceneSize
            anchors.centerIn: parent
            // see comment for world transform
            anchors.verticalCenterOffset: UAV.attitudePitch() * world.pitch1DegHeight
            border: 64 //sometimes numbers are excluded from bounding rect

            smooth: true
        }
    }

}
