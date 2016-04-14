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
    id: warnings
    property variant sceneSize

    SvgElementImage {
        id: warning_bg
        elementName: "warnings-bg"
        sceneSize: warnings.sceneSize
        width: background.width
        anchors.bottom: parent.bottom
    }

    SvgElementPositionItem {
        id: warning_time
        sceneSize: parent.sceneSize
        elementName: "warning-time"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        Rectangle {
            anchors.fill: parent
            color: (UAV.flightTimeValue() > 0) ? "green" : "grey"

            Text {
                anchors.centerIn: parent
                text: Utils.formatFlightTime(UAV.flightTimeValue())
                font {
                    family: pt_bold.name
                    pixelSize: Math.floor(parent.height * 0.8)
                    weight: Font.DemiBold
                }
            }
        }
    }

    SvgElementPositionItem {
        id: warning_arm
        sceneSize: parent.sceneSize
        elementName: "warning-arm"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        Rectangle {
            anchors.fill: parent
            color: UAV.armStatusColor()

            Text {
                anchors.centerIn: parent
                text: UAV.armStatusName()
                font {
                    family: pt_bold.name
                    pixelSize: Math.floor(parent.height * 0.74)
                    weight: Font.DemiBold
                }
            }
        }
    }

    SvgElementPositionItem {
        id: warning_rc_input
        sceneSize: parent.sceneSize
        elementName: "warning-rc-input"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        Rectangle {
            anchors.fill: parent
            color: UAV.rcInputStatusColor()

            Text {
                anchors.centerIn: parent
                text: "RC INPUT"
                font {
                    family: pt_bold.name
                    pixelSize: Math.floor(parent.height * 0.74)
                    weight: Font.DemiBold
                }
            }
        }
    }

    SvgElementPositionItem {
        id: warning_master_caution
        sceneSize: parent.sceneSize
        elementName: "warning-master-caution"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        Rectangle {
            anchors.fill: parent
            color: "red"
            opacity: UAV.masterCautionWarning() ? 1.0 : 0.4

            Text {
                anchors.centerIn: parent
                text: "MASTER CAUTION"
                color: "white"
                font {
                    family: pt_bold.name
                    pixelSize: Math.floor(parent.height * 0.74)
                    weight: Font.DemiBold
                }
            }
        }
    }

    SvgElementPositionItem {
        id: warning_autopilot
        sceneSize: parent.sceneSize
        elementName: "warning-autopilot"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        Rectangle {
            anchors.fill: parent
            color: UAV.autopilotStatusColor()

            Text {
                anchors.centerIn: parent
                text: "AUTOPILOT"
                font {
                    family: pt_bold.name
                    pixelSize: Math.floor(parent.height * 0.74)
                    weight: Font.DemiBold
                }
            }
        }
    }

    SvgElementPositionItem {
        id: warning_flightmode
        sceneSize: parent.sceneSize
        elementName: "warning-flightmode"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        Rectangle {
            anchors.fill: parent
            color: UAV.flightModeColor()

            Text {
                anchors.centerIn: parent
                text: UAV.flightModeName()
                font {
                    family: pt_bold.name
                    pixelSize: Math.floor(parent.height * 0.74)
                    weight: Font.DemiBold
                }
            }
        }
    }

    SvgElementPositionItem {
        id: warning_thrustmode
        sceneSize: parent.sceneSize
        elementName: "warning-thrustmode"
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height
        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height

        Rectangle {
            anchors.fill: parent
            color: UAV.isFlightModeManual() ? "grey" : UAV.thrustModeColor()

            // grey : 'disabled' modes
            Text {
                anchors.centerIn: parent
                text: UAV.thrustModeName()
                font {
                    family: pt_bold.name
                    pixelSize: Math.floor(parent.height * 0.74)
                    weight: Font.DemiBold
                }
            }
        }
    }

    SvgElementImage {
        id: warning_gps
        elementName: "warning-gps"
        sceneSize: warnings.sceneSize

        visible: UAV.isGpsNotInitialised() ? false : !UAV.isGpsValid()
    }

    SvgElementImage {
        id: warning_attitude
        elementName: "warning-attitude"
        sceneSize: warnings.sceneSize
        anchors.centerIn: background.centerIn
        visible: UAV.isAttitudeNotInitialised() ? false : !UAV.isAttitudeValid()
    }
}
