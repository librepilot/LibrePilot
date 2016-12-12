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
import QtQuick.Controls 1.4

import Pfd 1.0
import OsgQtQuick 1.0

import "../js/common.js" as Utils
import "../js/uav.js" as UAV

Item {
    OSGViewport {
        id: osgViewport

        anchors.fill: parent
        focus: true

        sceneNode: skyNode
        camera: camera
        manipulator: nodeTrackerManipulator
        //incrementalCompile: true

        OSGCamera {
            id: camera
            fieldOfView: 90
            logarithmicDepthBuffer: true
        }

        OSGNodeTrackerManipulator {
            id: nodeTrackerManipulator
            // use model to compute camera home position
            sceneNode: modelTransformNode
            // model will be tracked
            trackNode: modelTransformNode
        }

        OSGSkyNode {
            id: skyNode
            sceneNode: sceneGroup
            viewport: osgViewport
            dateTime: Utils.getDateTime()
            minimumAmbientLight: pfdContext.minimumAmbientLight
        }

        OSGGroup {
            id: sceneGroup
            children: [ terrainFileNode, modelNode ]
        }

        OSGGeoTransformNode {
            id: modelNode

            sceneNode: terrainFileNode
            children: [ modelTransformNode ]

            clampToTerrain: true

            position: UAV.position()
        }

        OSGTransformNode {
            id: modelTransformNode

            children: [ modelFileNode ]

            // model dimensions are in mm, scale to meters
            scale: Qt.vector3d(0.001, 0.001, 0.001)
            attitude: UAV.attitude()
        }

        OSGFileNode {
            id: terrainFileNode
            source: pfdContext.terrainFile
        }

        OSGFileNode {
            id: modelFileNode

            // use ShaderGen pseudoloader to generate the shaders expected by osgEarth
            // see http://docs.osgearth.org/en/latest/faq.html#i-added-a-node-but-it-has-no-texture-lighting-etc-in-osgearth-why
            source: pfdContext.modelFile + ".osgearth_shadergen"

            optimizeMode: OptimizeMode.OptimizeAndCheck
        }

        Keys.onUpPressed: {
            pfdContext.nextModel();
        }

        Keys.onDownPressed: {
            pfdContext.previousModel();
        }

        BusyIndicator {
            width: 24
            height: 24
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 4

            running: osgViewport.busy
        }

    }
}
