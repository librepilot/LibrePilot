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

import "../js/common.js" as Utils
import "../js/uav.js" as UAV

Item {
    id: info
    property variant sceneSize

    //
    // Waypoint functions
    //

    property real posEast_old
    property real posNorth_old
    property real total_distance
    property real total_distance_km

    property bool init_dist: false

    function reset_distance() {
        total_distance = 0;
    }

    function compute_distance(posEast,posNorth) {
        if (total_distance == 0 && !init_dist) { init_dist = "true"; posEast_old = posEast; posNorth_old = posNorth; }
        if (posEast > posEast_old+3 || posEast < posEast_old - 3 || posNorth > posNorth_old + 3 || posNorth < posNorth_old - 3) {
           total_distance += Math.sqrt(Math.pow((posEast - posEast_old ), 2) + Math.pow((posNorth - posNorth_old), 2));
           total_distance_km = total_distance / 1000;

           posEast_old = posEast;
           posNorth_old = posNorth;
           return total_distance;
        }
    }

    // End Functions
    //
    // Start Drawing

    SvgElementImage {
        id: info_bg
        sceneSize: info.sceneSize
        elementName: "info-bg"
        width: parent.width
        opacity: opaque ? 1 : 0.3
    }

    //
    // GPS Info (Top)
    //

    property real bar_width: (info_bg.height + info_bg.width) / 110

    property variant gps_tooltip: "Altitude : " + UAV.gpsAltitude() + "m\n" +
                                  "H/V/P DOP : " + UAV.gpsHdopInfo() +
                                  [UAV.gpsSensorType() == "DJI" ? "" :  "\n" + UAV.gpsSatsInView() + " Sats in view"]

    Repeater {
        id: satNumberBar

        model: 13
        Rectangle {
            property int minSatNumber : index
            width: Math.round(bar_width)
            radius: width / 4

            TooltipArea {
               text: gps_tooltip
            }

            x: Math.round((bar_width * 4.5) + (bar_width * 1.6 * index))
            height: bar_width * index * 0.6
            y: (bar_width * 8) - height
            color: "green"
            opacity: UAV.gpsNumSat() >= minSatNumber ? 1 : 0.4
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "gps-mode-text"

        TooltipArea {
            text: gps_tooltip
        }

        Text {
            text: [UAV.gpsNumSat() > 5 ? " " + UAV.gpsNumSat().toString() + " sats - " : ""] + UAV.gpsStatus()
            anchors.centerIn: parent
            font.pixelSize: parent.height*1.3
            font.family: pt_bold.name
            font.weight: Font.DemiBold
            color: "white"
        }
    }

    SvgElementImage {
        sceneSize: info.sceneSize
        elementName: "gps-icon"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        TooltipArea {
            text: gps_tooltip
        }
    }

    // Waypoint Info (Top)
    // Only visible when PathPlan is active (WP loaded)

    SvgElementImage {
        sceneSize: info.sceneSize
        elementName: "waypoint-labels"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: UAV.isPathPlanEnabled()
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "waypoint-heading-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: UAV.isPathPlanEnabled()

        Text {
            text: UAV.isPathPlanValid() ? "   " + UAV.waypointHeading().toFixed(1) + "°" : "   0°"
            anchors.centerIn: parent
            color: "cyan"

            font {
                family: pt_bold.name
                pixelSize: Math.floor(parent.height * 1.5)
                weight: Font.DemiBold
            }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "waypoint-distance-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: UAV.isPathPlanEnabled()

        Text {
            text: UAV.isPathPlanValid() ? "  " + UAV.waypointDistance().toFixed(0) + " m" : "  0 m"
            anchors.centerIn: parent
            color: "cyan"

            font {
                family: pt_bold.name
                pixelSize: Math.floor(parent.height * 1.5)
                weight: Font.DemiBold
            }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "waypoint-total-distance-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: UAV.isPathPlanEnabled()

        MouseArea { id: total_dist_mouseArea; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: reset_distance()}

        Text {
            text: "  "+total_distance.toFixed(0)+" m"
            anchors.centerIn: parent
            color: "cyan"

            font {
                family: pt_bold.name
                pixelSize: Math.floor(parent.height * 1.5)
                weight: Font.DemiBold
            }
        }

        Timer {
            interval: 1000; running: true; repeat: true;
            onTriggered: { if (UAV.isGpsStatusFix3D()) compute_distance(positionState.east, positionState.north) }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "waypoint-eta-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: UAV.isPathPlanEnabled()

        Text {
            text: UAV.isPathPlanValid() ? Utils.estimatedTimeOfArrival(UAV.waypointDistance(), UAV.currentVelocity()) : "00:00:00"
            anchors.centerIn: parent
            color: "cyan"

            font {
                family: pt_bold.name
                pixelSize: Math.floor(parent.height * 1.5)
                weight: Font.DemiBold
            }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "waypoint-number-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: UAV.isPathPlanEnabled()

        Text {
            text: UAV.isPathPlanValid() ? UAV.currentWaypointActive() + " / " + UAV.waypointCount() : "0 / 0"
            anchors.centerIn: parent
            color: "cyan"

            font {
                family: pt_bold.name
                pixelSize: Math.floor(parent.height * 1.5)
                weight: Font.DemiBold
            }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "waypoint-mode-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: UAV.isPathPlanEnabled()

        Text {
            text: UAV.isPathPlanValid() ? UAV.pathModeDesired() : ""
            anchors.centerIn: parent
            color: "cyan"

            font {
                family: pt_bold.name
                pixelSize: Math.floor(parent.height * 1.5)
                weight: Font.DemiBold
            }
        }
    }

    // Battery Info (Top)
    // Only visible when PathPlan not active and Battery module enabled and ADC input configured

    SvgElementPositionItem {
        id: topbattery_voltamp_bg
        sceneSize: info.sceneSize
        elementName: "topbattery-label-voltamp-bg"

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height
        visible: (!UAV.isPathPlanEnabled() && UAV.batteryModuleEnabled() && UAV.batteryADCConfigured())

        Rectangle {
            anchors.fill: parent
            color: (UAV.batteryNbCells() > 0) ? UAV.batteryAlarmColor() : "black"

        }
    }

    SvgElementImage {
        sceneSize: info.sceneSize
        elementName: "topbattery-labels"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: (!UAV.isPathPlanEnabled() && UAV.batteryModuleEnabled() && UAV.batteryADCConfigured())
    }

    SvgElementPositionItem {
        id: topbattery_volt
        sceneSize: info.sceneSize
        elementName: "topbattery-volt-text"

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height
        visible: (!UAV.isPathPlanEnabled() && UAV.batteryModuleEnabled() && UAV.batteryADCConfigured())

        Rectangle {
            anchors.fill: parent
            color: "transparent"

            Text {
               text: UAV.batteryVoltage()
               anchors.centerIn: parent
               color: "white"
               font {
                   family: pt_bold.name
                   pixelSize: Math.floor(parent.height * 0.6)
                   weight: Font.DemiBold
               }
            }
        }
    }

    SvgElementPositionItem {
        id: topbattery_amp
        sceneSize: info.sceneSize
        elementName: "topbattery-amp-text"

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height
        visible: (!UAV.isPathPlanEnabled() && UAV.batteryModuleEnabled() && UAV.batteryADCConfigured())

        Rectangle {
            anchors.fill: parent
            color: "transparent"

            Text {
               text: UAV.batteryCurrent()
               anchors.centerIn: parent
               color: "white"
               font {
                   family: pt_bold.name
                   pixelSize: Math.floor(parent.height * 0.6)
                   weight: Font.DemiBold
               }
            }
        }
    }

    SvgElementPositionItem {
        id: topbattery_milliamp
        sceneSize: info.sceneSize
        elementName: "topbattery-milliamp-text"

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height
        visible: (!UAV.isPathPlanEnabled() && UAV.batteryModuleEnabled() && UAV.batteryADCConfigured())

        Rectangle {
            anchors.fill: parent

            TooltipArea {
                  text: "Reset consumed energy"
            }

            MouseArea {
               id: reset_consumed_energy_mouseArea;
               anchors.fill: parent;
               cursorShape: Qt.PointingHandCursor;
               onClicked: pfdContext.resetConsumedEnergy();
            }

            // Alarm based on estimatedFlightTime < 120s orange, < 60s red
            color: UAV.estimatedTimeAlarmColor()

            border.color: "white"
            border.width: topbattery_volt.width * 0.01
            radius: border.width * 4

            Text {
               text: UAV.batteryConsumedEnergy()
               anchors.centerIn: parent
               color: "white"
               font {
                   family: pt_bold.name
                   pixelSize: Math.floor(parent.height * 0.6)
                   weight: Font.DemiBold
               }
            }
        }
    }

    // Default counter
    // Only visible when PathPlan not active

    SvgElementImage {
        sceneSize: info.sceneSize
        elementName: "topbattery-total-distance-label"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: !UAV.isPathPlanEnabled()
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        elementName: "topbattery-total-distance-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)
        visible: !UAV.isPathPlanEnabled()

        TooltipArea {
            text: "Reset distance counter"
        }

        MouseArea { id: total_dist_mouseArea2; anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: reset_distance()}

        Text {
            text:  total_distance > 1000 ? total_distance_km.toFixed(2) +" Km" : total_distance.toFixed(0)+" m"
            anchors.right: parent.right
            color: "cyan"

            font {
                family: pt_bold.name
                pixelSize: Math.floor(parent.height * 1)
                weight: Font.DemiBold
            }
        }

        Timer {
            interval: 1000; running: true; repeat: true;
            onTriggered: { if (UAV.isGpsStatusFix3D()) compute_distance(positionState.east, positionState.north) }
        }
    }

    //
    // Home info (visible after arming)
    //

    SvgElementImage {
        id: home_bg
        elementName: "home-bg"
        sceneSize: info.sceneSize
        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        opacity: opaque ? 1 : 0.6

        states: State {
             name: "fading"
             when: UAV.isTakeOffLocationValid()
             PropertyChanges { target: home_bg; x: Math.floor(scaledBounds.x * sceneItem.width) - home_bg.width; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; duration: 800 }
            }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        id: home_heading_text
        elementName: "home-heading-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)

        states: State {
             name: "fading_heading"
             when: UAV.isTakeOffLocationValid()
             PropertyChanges { target: home_heading_text; x: Math.floor(scaledBounds.x * sceneItem.width) - home_bg.width; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; duration: 800 }
            }
        }

        Text {
            text: "  " + UAV.homeHeading().toFixed(1) + "°"
            anchors.centerIn: parent
            color: "cyan"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 1.2
            }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        id: home_distance_text
        elementName: "home-distance-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)

        states: State {
             name: "fading_distance"
             when: UAV.isTakeOffLocationValid()
             PropertyChanges { target: home_distance_text; x: Math.floor(scaledBounds.x * sceneItem.width) - home_bg.width; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; duration: 800 }
            }
        }

        Text {
            text: UAV.homeDistance().toFixed(0) + " m"
            anchors.centerIn: parent
            color: "cyan"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 1.2
            }
        }
    }

    SvgElementPositionItem {
        sceneSize: info.sceneSize
        id: home_eta_text
        elementName: "home-eta-text"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: Math.floor(scaledBounds.y * sceneItem.height)

        states: State {
             name: "fading_distance"
             when: UAV.isTakeOffLocationValid()
             PropertyChanges { target: home_eta_text; x: Math.floor(scaledBounds.x * sceneItem.width) - home_bg.width; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; duration: 800 }
            }
        }

        Text {
            text: Utils.estimatedTimeOfArrival(UAV.homeDistance(), UAV.currentVelocity())
            anchors.centerIn: parent
            color: "cyan"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 1.2
            }
        }
    }

    SvgElementImage {
        id: info_border
        elementName: "info-border"
        sceneSize: info.sceneSize
        width: Math.floor(parent.width * 1.009)
    }
}
