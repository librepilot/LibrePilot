import QtQuick 2.4

import UAVTalk.SystemSettings 1.0
import UAVTalk.SystemAlarms 1.0
import UAVTalk.FlightStatus 1.0
import UAVTalk.VtolPathFollowerSettings 1.0

import "../common.js" as Utils

Item {
    id: warnings

    property variant sceneSize

    //  Uninitialised, OK,    Warning, Error, Critical
    property variant statusColors : ["gray", "green", "red", "red", "red"]

    //  DisArmed , Arming, Armed
    property variant armColors : ["gray", "orange", "green"]

    // All 'manual modes' are green, 'assisted' modes in cyan
    // "MANUAL","STAB 1","STAB 2", "STAB 3", "STAB 4", "STAB 5", "STAB 6",
    // "POS HOLD", "COURSELOCK","VEL ROAM", "HOME LEASH", "ABS POS", "RTB", "LAND", "PATHPLAN", "POI", "AUTOCRUISE", "AUTOTAKEOFF"
    property variant flightmodeColors : ["gray", "green", "green", "green", "green", "green", "green",
                                         "cyan", "cyan", "cyan", "cyan", "cyan", "cyan", "cyan", "cyan", "cyan", "cyan", "cyan"]

    // Manual,Rate,RateTrainer,Attitude,AxisLock,WeakLeveling,VirtualBar,Acro+,Rattitude,
    // AltitudeHold,AltitudeVario,CruiseControl" + Auto mode (VTOL/Wing pathfollower)
    // grey : 'disabled' modes
    property variant thrustmodeColors : ["green", "grey", "grey", "grey", "grey", "grey", "grey", "grey", "grey",
                                         "green", "green", "green", "cyan"]

    // systemSettings.airframeType 3 - 17 : VtolPathFollower, check ThrustControl
    property var thrust_mode: (flightStatus.flightMode < FlightMode.PositionHold) ? stabilizationDesired.stabilizationModeThrust :
                              (flightStatus.flightMode >= FlightMode.PositionHold) && (systemSettings.airframeType > AirframeType.FixedWingVtail) &&
                              (systemSettings.airframeType < AirframeType.GroundVehicleCar) && (vtolPathFollowerSettings.thrustControl == ThrustControl.Auto) ? 12 :
                              (flightStatus.flightMode >= FlightMode.PositionHold) && (systemSettings.airframeType < AirframeType.VTOL) ? 12 : 0

    property real flight_time: Math.round(systemStats.flightTime / 1000)
    property real time_h: (flight_time > 0 ? Math.floor(flight_time / 3600) : 0 )
    property real time_m: (flight_time > 0 ? Math.floor((flight_time - time_h*3600)/60) : 0)
    property real time_s: (flight_time > 0 ? Math.floor(flight_time - time_h*3600 - time_m*60) : 0)

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
            color: (systemStats.flightTime > 0) ? "green" : "grey"

            Text {
                anchors.centerIn: parent
                text: Utils.formatTime(time_h) + ":" + Utils.formatTime(time_m) + ":" + Utils.formatTime(time_s)
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
            color: warnings.armColors[flightStatus.armed]

            Text {
                anchors.centerIn: parent
                text: ["DISARMED","ARMING","ARMED"][flightStatus.armed]
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
            color: warnings.statusColors[systemAlarms.alarmManualControl]

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

        property bool warningActive: ((systemAlarms.alarmBootFault > Alarm.OK) ||
                                      (systemAlarms.alarmOutOfMemory > Alarm.OK) ||
                                      (systemAlarms.alarmStackOverflow > Alarm.OK) ||
                                      (systemAlarms.alarmCPUOverload > Alarm.OK) ||
                                      (systemAlarms.alarmEventSystem > Alarm.OK))
        Rectangle {
            anchors.fill: parent
            color: parent.warningActive ? "red" : "red"
            opacity: parent.warningActive ? 1.0 : 0.4

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
            color: warnings.statusColors[systemAlarms.alarmGuidance]

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
            color: warnings.flightmodeColors[flightStatus.flightMode]
            // Manual,Stabilized1,Stabilized2,Stabilized3,Stabilized4,Stabilized5,Stabilized6,PositionHold,CourseLock,
            // VelocityRoam,HomeLeash,AbsolutePosition,ReturnToBase,Land,PathPlanner,POI,AutoCruise,AutoTakeoff

            Text {
                anchors.centerIn: parent
                text: ["MANUAL","STAB 1","STAB 2", "STAB 3", "STAB 4", "STAB 5", "STAB 6", "POS HOLD", "COURSELOCK",
                       "VEL ROAM", "HOME LEASH", "ABS POS", "RTB", "LAND", "PATHPLAN", "POI", "AUTOCRUISE", "AUTOTAKEOFF"][flightStatus.flightMode]
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
            color: (flightStatus.flightMode == FlightMode.Manual) ? "grey" : warnings.thrustmodeColors[thrust_mode]

            // grey : 'disabled' modes
            Text {
                anchors.centerIn: parent
                text: ["MANUAL"," "," ", " ", " ", " ", " ", " ", " ", "ALT HOLD", "ALT VARIO", "CRUISECTRL", "AUTO"][thrust_mode]
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

        visible: (systemAlarms.alarmGPS > Alarm.OK)
    }

    SvgElementImage {
        id: warning_attitude
        elementName: "warning-attitude"
        sceneSize: warnings.sceneSize
        anchors.centerIn: background.centerIn
        visible: (systemAlarms.alarmAttitude > Alarm.OK)
    }
}
