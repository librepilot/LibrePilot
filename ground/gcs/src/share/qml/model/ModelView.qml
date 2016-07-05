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

import OsgQtQuick 1.0

import "../js/uav.js" as UAV

Item {

    OSGViewport {
        anchors.fill: parent
        focus: true

        sceneNode: sceneNode
        camera: camera
        manipulator: trackballManipulator

        OSGCamera {
            id: camera
            fieldOfView: 90
        }

        OSGTrackballManipulator {
            id: trackballManipulator
            sceneNode: transformNode
        }

        OSGGroup {
            id: sceneNode
            children: [ transformNode, backgroundNode ]
        }

        OSGBillboardNode {
            id: backgroundNode
            children: [ backgroundImageNode ]
        }

        OSGImageNode {
            id: backgroundImageNode
            imageUrl: pfdContext.backgroundImageFile
        }

        OSGTransformNode {
            id: transformNode

            children: [ fileNode ]

            attitude: UAV.attitude()
        }

        OSGFileNode {
            id: fileNode
            source: pfdContext.modelFile
            async: false
            optimizeMode: OptimizeMode.OptimizeAndCheck
        }

        Keys.onUpPressed: {
            pfdContext.nextModel();
        }

        Keys.onDownPressed: {
            pfdContext.previousModel();
        }
    }

}
