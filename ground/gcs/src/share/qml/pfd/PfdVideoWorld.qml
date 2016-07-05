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

import Pfd 1.0
import OsgQtQuick 1.0

import "../js/common.js" as Utils
import "../js/uav.js" as UAV

OSGViewport {
    id: osgViewport

    anchors.fill: parent
    focus: true

    readonly property real horizontCenter : horizontCenterItem.horizontCenter

    // Factor for OSGViewer vertical offset
    readonly property double factor: 0.04

    // Stretch height and apply offset
    //height: height * (1 + factor)
    y: -height * factor

    sceneNode: billboardNode
    camera: camera
    updateMode: UpdateMode.Discrete

    OSGCamera {
        id: camera
        fieldOfView: 90
    }

    OSGBillboardNode {
        id: billboardNode
        children: [ videoNode ]
    }

    OSGImageNode {
       id: videoNode
       imageUrl: "gst://localhost/play?" + encodeURIComponent(pfdContext.gstPipeline)
    }

    Rectangle {
        // using rectangle instead of svg rendered to pixmap
        // as it's much more memory efficient
        id: world
        smooth: true
        opacity: 0

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "horizon")
        width: Math.round(sceneItem.width * scaledBounds.width / 2) * 2
        height: Math.round(sceneItem.height * scaledBounds.height / 2) * 3

        property double pitch1DegScaledHeight: (svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-90").y -
                                                svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch90").y) / 180.0

        property double pitch1DegHeight: sceneItem.height * pitch1DegScaledHeight

        transform: [
            Translate {
                id: pitchTranslate
                x: Math.round((world.parent.width - world.width)/2)
                // y is centered around world_center element
                y: Math.round(horizontCenter - world.height / 2 + UAV.attitudePitch() * world.pitch1DegHeight)
            },
            Rotation {
                angle: -UAV.attitudeRoll()
                origin.x : world.parent.width / 2
                origin.y : horizontCenter
            }
        ]

    }

    Item {
        id: pitch_window
        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-window-terrain")

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height) - osgViewport.y
        width: Math.floor(scaledBounds.width * sceneItem.width)
        height: Math.floor(scaledBounds.height * sceneItem.height)

        rotation: -UAV.attitudeRoll()
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
            anchors.verticalCenterOffset: attitudeState.pitch * world.pitch1DegHeight
            border: 64 // sometimes numbers are excluded from bounding rect

            smooth: true
        }

        SvgElementImage {
            id: horizont_line
            elementName: "center-line"

            opacity: 0.5

            // worldView is loaded with Loader, so background element is visible
            sceneSize: background.sceneSize
            anchors.centerIn: parent

            anchors.verticalCenterOffset: UAV.attitudePitch() * world.pitch1DegHeight
            border: 1
            smooth: true
       }

       SvgElementImage {
            id: pitch_0
            elementName: "pitch0"

            sceneSize: background.sceneSize
            anchors.centerIn: parent
            anchors.verticalCenterOffset: UAV.attitudePitch() * world.pitch1DegHeight
            border: 1
            smooth: true
       }
    }
}
