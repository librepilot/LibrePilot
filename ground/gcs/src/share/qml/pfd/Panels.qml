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
import QtQuick 2.0

import "../js/common.js" as Utils
import "../js/uav.js" as UAV

Item {
    id: panels
    property variant sceneSize

    //
    // Panel functions
    //

    property bool show_panels: false
    property bool display_rc: false
    property bool display_bat: false
    property bool display_oplm: false
    property bool display_sys: false

    function close_panels(){
        if (show_panels == true)
            show_panels = false;
        else
            show_panels = true;
    }

    function hide_display_rcinput(){
        show_panels = true;
        display_oplm = false
        display_bat = false
        rc_input_bg.z = 10
        battery_bg.z = -1
        oplm_bg.z = -1
        system_bg.z = -1
    }

    function hide_display_battery(){
        show_panels = true;
        display_oplm = false
        display_bat = true
        rc_input_bg.z = 10
        battery_bg.z = 20
        oplm_bg.z = -1
        system_bg.z = -1
    }

    function hide_display_oplink(){
        show_panels = true;
        display_oplm = true
        display_bat = false
        rc_input_bg.z = 10
        battery_bg.z = 20
        oplm_bg.z = 30
        system_bg.z = -1
    }

    function hide_display_system(){
        show_panels = true;
        display_oplm = false
        display_bat = false
        rc_input_bg.z = 10
        battery_bg.z = 20
        oplm_bg.z = 30
        system_bg.z = 40
    }

    property real smeter_angle

    // Needed to get correctly int8 value
    property int oplm_rssi: telemetry_link ? UAV.oplmRSSI() : -127

    property real telemetry_sum
    property real telemetry_sum_old
    property bool telemetry_link

    // Hack : check if telemetry is active. Works with real link and log replay

    function telemetry_check() {
       telemetry_sum = opLinkStatus.rxRate + opLinkStatus.txRate

       if (telemetry_sum != telemetry_sum_old || UAV.isOplmConnected()) {
           telemetry_link = 1
       } else {
           telemetry_link = 0
       }
       telemetry_sum_old = telemetry_sum
    }

    Timer {
         id: telemetry_activity
         interval: 1200; running: true; repeat: true
         onTriggered: telemetry_check()
    }

    // Filtering for S-meter. Smeter range -127dB <--> -13dB = S9+60dB

    Timer {
         id: smeter_filter
         interval: 100; running: true; repeat: true
         onTriggered: smeter_angle = (0.90 * smeter_angle) + (0.1 * (oplm_rssi + 13))
    }

    // End Functions
    //
    // Start Drawing

    //
    // Animation properties
    //

    property double offset_value: close_bg.width * 0.85

    property int anim_type: Easing.OutExpo
    property int anim_duration: 1600

    //
    // Close - Open panel
    //

    SvgElementImage {
        id: close_bg
        elementName: "close-bg"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)


        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: close_bg; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                id: close_anim
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: panel_open_icon
        elementName: "panel-open-icon"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: close_bg.z + 1
        opacity: show_panels == true ? 0 : 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: panel_open_icon; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
             PropertyChanges { target: panel_open_icon; opacity: 0; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
                PropertyAnimation { property: "opacity"; duration: 500; }
            }
        }
    }

    SvgElementImage {
        id: close_mousearea
        elementName: "close-panel-mousearea"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: close_bg.z + 100

        TooltipArea {
            text: show_panels == true ? "Close panels" : "Open panels"
        }

        MouseArea {
             id: hidedisp_close;
             anchors.fill: parent;
             cursorShape: Qt.PointingHandCursor
             onClicked: close_panels()
        }

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: close_mousearea; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    //
    // Rc-Input panel
    //

    SvgElementImage {
        id: rc_input_bg
        elementName: "rc-input-bg"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: 10

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: rc_input_bg; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: rc_input_labels
        elementName: "rc-input-labels"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: rc_input_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: rc_input_labels; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: rc_input_mousearea
        elementName: "rc-input-panel-mousearea"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: rc_input_bg.z + 1

        TooltipArea {
            text: "RC panel"
        }

        MouseArea {
             id: hidedisp_rcinput;
             anchors.fill: parent;
             cursorShape: Qt.PointingHandCursor
             onClicked: hide_display_rcinput()
        }

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: rc_input_mousearea; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: rc_throttle
        elementName: "rc-throttle"
        sceneSize: panels.sceneSize
        z: rc_input_bg.z+2

        width: scaledBounds.width * sceneItem.width
        height: (scaledBounds.height * sceneItem.height) * (manualControlCommand.throttle)

        x: scaledBounds.x * sceneItem.width
        y: (scaledBounds.y * sceneItem.height) - rc_throttle.height + (scaledBounds.height * sceneItem.height)

        smooth: true

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: rc_throttle; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: rc_stick
        elementName: "rc-stick"
        sceneSize: panels.sceneSize
        z: rc_input_bg.z+3

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        y: (scaledBounds.y * sceneItem.height) + (manualControlCommand.pitch * rc_stick.width * 2.5)

        smooth: true

        //rotate it around his center
        transform: Rotation {
            angle: manualControlCommand.yaw * 90
            origin.y : rc_stick.height / 2
            origin.x : rc_stick.width / 2
        }

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: rc_stick; x: Math.floor(scaledBounds.x * sceneItem.width) + (manualControlCommand.roll * rc_stick.width * 2.5) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    //
    // Battery panel
    //

    SvgElementImage {
        id: battery_bg
        elementName: "battery-bg"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: 20

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: battery_bg; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementPositionItem {
        id: battery_volt
        sceneSize: panels.sceneSize
        elementName: "battery-volt-text"
        z: battery_bg.z + 1

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: battery_volt; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: UAV.batteryAlarmColor()
            border.color: "white"
            border.width: battery_volt.width * 0.01
            radius: border.width * 4

            Text {
               text: UAV.batteryVoltage()
               anchors.centerIn: parent
               color: "white"
               font {
                   family: pt_bold.name
                   pixelSize: Math.floor(parent.height * 0.6)
               }
            }
        }
    }

    SvgElementPositionItem {
        id: battery_amp
        sceneSize: panels.sceneSize
        elementName: "battery-amp-text"
        z: battery_bg.z+2

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: battery_amp; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: UAV.batteryAlarmColor()
            border.color: "white"
            border.width: battery_volt.width * 0.01
            radius: border.width * 4

            Text {
               text: UAV.batteryCurrent()
               anchors.centerIn: parent
               color: "white"
               font {
                   family: pt_bold.name
                   pixelSize: Math.floor(parent.height * 0.6)
               }
            }
        }
    }

    SvgElementPositionItem {
        id: battery_milliamp
        sceneSize: panels.sceneSize
        elementName: "battery-milliamp-text"
        z: battery_bg.z+3

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: battery_milliamp; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Rectangle {
            anchors.fill: parent

            TooltipArea {
               text: "Reset consumed energy"
               visible: display_bat == true ? 1 : 0
            }

            MouseArea {
               id: reset_panel_consumed_energy_mouseArea;
               anchors.fill: parent;
               cursorShape: Qt.PointingHandCursor;
               visible: display_bat == true ? 1 : 0
               onClicked: pfdContext.resetConsumedEnergy();
            }

            // Alarm based on estimatedFlightTime < 120s orange, < 60s red
            color: UAV.estimatedTimeAlarmColor()

            border.color: "white"
            border.width: battery_volt.width * 0.01
            radius: border.width * 4

            Text {
               text: UAV.batteryConsumedEnergy()
               anchors.centerIn: parent
               color: "white"
               font {
                   family: pt_bold.name
                   pixelSize: Math.floor(parent.height * 0.6)
               }
            }
        }
    }

    SvgElementPositionItem {
        id: battery_estimated_flight_time
        sceneSize: panels.sceneSize
        elementName: "battery-estimated-flight-time"
        z: battery_bg.z+4

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: battery_estimated_flight_time; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Rectangle {
            anchors.fill: parent

            TooltipArea {
               text: "Reset consumed energy"
               visible: display_bat == true ? 1 : 0
            }

            MouseArea {
               id: reset_panel_consumed_energy_mouseArea2;
               anchors.fill: parent;
               cursorShape: Qt.PointingHandCursor;
               visible: display_bat == true ? 1 : 0
               onClicked: pfdContext.resetConsumedEnergy();
            }

            // Alarm based on estimatedFlightTime < 120s orange, < 60s red
            color: UAV.estimatedTimeAlarmColor()

            border.color: "white"
            border.width: battery_volt.width * 0.01
            radius: border.width * 4

            Text {
               text: Utils.formatFlightTime(UAV.estimatedFlightTimeValue())
               anchors.centerIn: parent
               color: "white"
               font {
                   family: pt_bold.name
                   pixelSize: Math.floor(parent.height * 0.6)
               }
            }
        }
    }

    SvgElementImage {
        id: battery_labels
        elementName: "battery-labels"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: battery_bg.z+5

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: battery_labels; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: battery_mousearea
        elementName: "battery-panel-mousearea"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: battery_bg.z+6

        TooltipArea {
            text: "Battery panel"
        }

        MouseArea {
             id: hidedisp_battery;
             anchors.fill: parent;
             cursorShape: Qt.PointingHandCursor
             onClicked: hide_display_battery()
        }

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: battery_mousearea; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    //
    // OPLM panel
    //

    SvgElementImage {
        id: oplm_bg
        elementName: "oplm-bg"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: 30

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: oplm_bg; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: smeter_bg
        elementName: "smeter-bg"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: oplm_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: smeter_bg; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: smeter_scale
        elementName: "smeter-scale"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: oplm_bg.z + 2

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: smeter_scale; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: smeter_needle
        elementName: "smeter-needle"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: oplm_bg.z + 3

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: smeter_needle; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        transform: Rotation {
            angle: smeter_angle.toFixed(1)
            origin.y : smeter_needle.height
        }
    }

    SvgElementImage {
        id: smeter_mask
        elementName: "smeter-mask"
        sceneSize: panels.sceneSize
        //y: Math.floor(scaledBounds.y * sceneItem.height)
        width: smeter_scale.width * 1.09
        //anchors.horizontalCenter: smeter_scale

        z: oplm_bg.z + 4

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: smeter_mask; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementImage {
        id: oplm_id_label
        elementName: "oplm-id-label"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: oplm_bg.z + 6

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: oplm_id_label; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementPositionItem {
        id: oplm_id_text
        sceneSize: panels.sceneSize
        elementName: "oplm-id-text"
        z: oplm_bg.z + 7

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: oplm_id_text; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: (UAV.oplmDeviceID() > 0) ? UAV.oplmDeviceID().toString(16) : "--  --  --  --"
             anchors.centerIn: parent
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
                 capitalization: Font.AllUppercase
             }
        }
    }

    SvgElementImage {
        id: rx_quality_label
        elementName: "rx-quality-label"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: oplm_bg.z + 8

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: rx_quality_label; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementPositionItem {
        id: rx_quality_text
        sceneSize: panels.sceneSize
        elementName: "rx-quality-text"
        z: oplm_bg.z + 9

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: rx_quality_text; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: UAV.receiverQuality()
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementImage {
        id: cnx_state_label
        elementName: "cnx-state-label"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: oplm_bg.z + 8

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: cnx_state_label; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementPositionItem {
        id: cnx_state_text
        sceneSize: panels.sceneSize
        elementName: "cnx-state-text"
        z: oplm_bg.z + 9

        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        y: scaledBounds.y * sceneItem.height

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: cnx_state_text; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: UAV.oplmLinkState()
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementImage {
        id: oplm_mousearea
        elementName: "oplm-panel-mousearea"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: oplm_bg.z

        TooltipArea {
            text: "Link panel"
        }

        MouseArea {
             id: hidedisp_oplm;
             anchors.fill: parent;
             cursorShape: Qt.PointingHandCursor
             onClicked: hide_display_oplink()
        }

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: oplm_mousearea; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    //
    // System panel
    //

    SvgElementImage {
        id: system_bg
        elementName: "system-bg"
        sceneSize: panels.sceneSize
        y: scaledBounds.y * sceneItem.height
        z: 40

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_bg; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }

    SvgElementPositionItem {
        id: system_frametype
        elementName: "system-frame-type"
        sceneSize: panels.sceneSize
        y: scaledBounds.y * sceneItem.height
        z: system_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_frametype; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: UAV.frameType()
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementPositionItem {
        id: system_cpuloadtemp
        elementName: "system-cpu-load-temp"
        sceneSize: panels.sceneSize
        y: scaledBounds.y * sceneItem.height
        z: system_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_cpuloadtemp; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             // CC3D: Only display Cpu load, no temperature available.
             text: UAV.cpuLoad() + "%" + [UAV.isCC3D() ? "" : " | " + UAV.cpuTemp() + "°C"]
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementPositionItem {
        id: system_memfree
        elementName: "system-mem-free"
        sceneSize: panels.sceneSize
        y: scaledBounds.y * sceneItem.height
        z: system_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_memfree; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: UAV.freeMemory()
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementPositionItem {
        id: system_fusion_algo
        elementName: "system-attitude-estimation-algo"
        sceneSize: panels.sceneSize
        y: scaledBounds.y * sceneItem.height
        z: system_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_fusion_algo; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: UAV.fusionAlgorithm()
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.35)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementPositionItem {
        id: system_mag_used
        elementName: "system-mag-used"
        sceneSize: panels.sceneSize
        y: scaledBounds.y * sceneItem.height
        z: system_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_mag_used; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: UAV.magSourceName()
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementPositionItem {
        id: system_gpstype
        elementName: "system-gps-type"
        sceneSize: panels.sceneSize
        y: scaledBounds.y * sceneItem.height
        z: system_bg.z + 1

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_gpstype; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }

        Text {
             text: UAV.gpsSensorType()
             anchors.right: parent.right
             color: "white"
             font {
                 family: pt_bold.name
                 pixelSize: Math.floor(parent.height * 1.4)
                 weight: Font.DemiBold
             }
        }
    }

    SvgElementImage {
        id: system_mousearea
        elementName: "system-panel-mousearea"
        sceneSize: panels.sceneSize
        y: Math.floor(scaledBounds.y * sceneItem.height)
        z: system_bg.z + 1

        TooltipArea {
            text: "System panel"
        }

        MouseArea {
             id: hidedisp_system;
             anchors.fill: parent;
             cursorShape: Qt.PointingHandCursor
             onClicked: hide_display_system()
        }

        states: State {
             name: "fading"
             when: show_panels == true
             PropertyChanges { target: system_mousearea; x: Math.floor(scaledBounds.x * sceneItem.width) + offset_value; }
        }

        transitions: Transition {
            SequentialAnimation {
                PropertyAnimation { property: "x"; easing.type: anim_type; duration: anim_duration }
            }
        }
    }
}
