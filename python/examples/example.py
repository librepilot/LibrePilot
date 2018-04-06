##
##############################################################################
#
# @file       example.py
# @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
#             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
# @brief      Base classes for python UAVObject
#   
# @see        The GNU Public License (GPL) Version 3
#
#############################################################################/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# 


import logging
import serial
import traceback
import sys

from librepilot.uavtalk.uavobject import *
from librepilot.uavtalk.uavtalk import *
from librepilot.uavtalk.objectManager import *
from librepilot.uavtalk.connectionManager import *


def _hex02(value):
    return "%02X" % value


class UavtalkDemo():
    def __init__(self):
        self.nbUpdates = 0
        self.lastRateCalc = time.time()
        self.updateRate = 0
        self.objMan = None
        self.connMan = None

    def setup(self, port):
        print "Opening Port \"%s\"" % port
        if port[:3].upper() == "COM":
            _port = int(port[3:]) - 1
        else:
            _port = port
        serPort = serial.Serial(_port, 57600, timeout=.5)
        if not serPort.isOpen():
            raise IOError("Failed to open serial port")

        print "Creating UavTalk"
        self.uavTalk = UavTalk(serPort, None)

        print "Starting ObjectManager"
        self.objMan = ObjManager(self.uavTalk)
        self.objMan.importDefinitions()

        print "Starting UavTalk"
        self.uavTalk.start()

        print "Starting ConnectionManager"
        self.connMan = ConnectionManager(self.uavTalk, self.objMan)

        print "Connecting...",
        self.connMan.connect()
        print "Connected"

        print "Getting all Data"
        self.objMan.requestAllObjUpdate()

    # print "SN:",
    # sn = self.objMan.FirmwareIAPObj.CPUSerial.value
    # print "".join(map(_hex02, sn))

    def stop(self):
        if self.uavTalk:
            print "Stopping UavTalk"
            self.uavTalk.stop()

    def showAttitudeViaObserver(self):
        print "Request fast periodic updates for AttitudeState"
        self.objMan.AttitudeState.metadata.telemetryUpdateMode = UAVMetaDataObject.UpdateMode.PERIODIC
        self.objMan.AttitudeState.metadata.telemetryUpdatePeriod.value = 50
        self.objMan.AttitudeState.metadata.updated()

        print "Install Observer for AttitudeState updates\n"
        self.objMan.regObjectObserver(self.objMan.AttitudeState, self, "_onAttitudeUpdate")
        # Spin until we get interrupted
        while True:
            time.sleep(1)

    def showAttitudeViaWait(self):
        print "Request fast periodic updates for AttitudeState"
        self.objMan.AttitudeState.metadata.telemetryUpdateMode = UAVMetaDataObject.UpdateMode.PERIODIC
        self.objMan.AttitudeState.metadata.telemetryUpdatePeriod.value = 50
        self.objMan.AttitudeState.metadata.updated()

        while True:
            self.objMan.AttitudeState.waitUpdate()
            self._onAttitudeUpdate(self.objMan.AttitudeState)

    def showAttitudeViaGet(self):
        while True:
            self.objMan.AttitudeState.getUpdate()
            self._onAttitudeUpdate(self.objMan.AttitudeState)

    def _onAttitudeUpdate(self, args):
        self.nbUpdates += 1

        now = time.time()
        if now - self.lastRateCalc > 1:
            self.updateRate = self.nbUpdates / (now - self.lastRateCalc)
            self.lastRateCalc = now
            self.nbUpdates = 0

        if self.nbUpdates & 1:
            dot = "."
        else:
            dot = " "

        print " %s Rate: %02.1f Hz  " % (dot, self.updateRate),

        roll = self.objMan.AttitudeState.Roll.value
        print "RPY: %f %f %f " % (self.objMan.AttitudeState.Roll.value, self.objMan.AttitudeState.Pitch.value,
                                  self.objMan.AttitudeState.Yaw.value) + " "

    # return
    #    print "Roll: %f " % roll,
    #    i = roll/90
    #    if i<-1: i=-1
    #    if i>1: i= 1
    #    i = int((i+1)*15)
    #    print "-"*i+"*"+"-"*(30-i)+" \r",

    def driveServo(self):
        print "Taking control of self.actuatorCmd"
        self.objMan.ActuatorCommand.metadata.access = UAVMetaDataObject.Access.READONLY
        self.objMan.ActuatorCommand.metadata.updated()

        while True:
            self.objMan.ActuatorCommand.Channel.value[0] = 1000
            self.objMan.ActuatorCommand.updated()
            time.sleep(1)

            self.objMan.ActuatorCommand.Channel.value[0] = 2000
            self.objMan.ActuatorCommand.updated()
            time.sleep(1)


    def driveInput(self):
        print "Taking control of self.ManualControl"
        self.objMan.ManualControlCommand.metadata.access = UAVMetaDataObject.Access.READONLY
        self.objMan.ManualControlCommand.metadata.updated()
        self.objMan.ManualControlCommand.Connected.value = True
        self.objMan.ManualControlCommand.updated()

        print "Arming board using Yaw right"
        # FIXME: Seems there is a issue with ArmedField.ARMED, 2 equals to the ARMED state
        while (self.objMan.FlightStatus.Armed.value != 2):
            self.objMan.ManualControlCommand.Yaw.value = 1
            self.objMan.ManualControlCommand.updated()
            self.objMan.ManualControlCommand.Throttle.value = -1
            self.objMan.ManualControlCommand.updated()
            time.sleep(1)

        if (self.objMan.FlightStatus.Armed.value == 2):
            print "Board is armed !"
            self.objMan.ManualControlCommand.Yaw.value = 0
            self.objMan.ManualControlCommand.updated()        

        print "Applying Throttle"
        self.objMan.ManualControlCommand.Throttle.value = 0.01 # very small value for safety
        # Assuming board do not control a helicopter, Thrust value will be equal to Throttle value.
        # Because a 'high' value can be read from latest real RC input value, 
        # initial value is set now to zero for safety reasons.
        self.objMan.ManualControlCommand.Thrust.value = 0
        # self.objMan.ManualControlCommand.Throttle.value = self.objMan.ManualControlCommand.Thrust.value
        self.objMan.ManualControlCommand.updated()
        time.sleep(0.3)

        count = 60
        print "Moving Pitch input"
        while (count > 0):
            count-=1
            if self.objMan.ManualControlCommand.Pitch.value < 1:
                 self.objMan.ManualControlCommand.Pitch.value += 0.05
                 self.objMan.ManualControlCommand.updated()
                 time.sleep(0.1)
            if self.objMan.ManualControlCommand.Pitch.value > 1:
                self.objMan.ManualControlCommand.Pitch.value = 0
                self.objMan.ManualControlCommand.updated()
                time.sleep(0.1)

        self.objMan.ManualControlCommand.Pitch.value = 0
        self.objMan.ManualControlCommand.updated()
        time.sleep(0.5)

        count = 60
        print "Moving Roll input"
        while (count > 0):
            count-=1
            if self.objMan.ManualControlCommand.Roll.value < 1:
                 self.objMan.ManualControlCommand.Roll.value += 0.05
                 self.objMan.ManualControlCommand.updated()
                 time.sleep(0.1)

            if self.objMan.ManualControlCommand.Roll.value > 1:
                self.objMan.ManualControlCommand.Roll.value = 0
                self.objMan.ManualControlCommand.updated()
                time.sleep(0.1)

        self.objMan.ManualControlCommand.Roll.value = 0
        self.objMan.ManualControlCommand.updated()
        time.sleep(0.5)

        print "Setting Throttle to minimum"
        self.objMan.ManualControlCommand.Throttle.value = -1
        self.objMan.ManualControlCommand.updated()
        time.sleep(1)

        print "Disarming board using Yaw left"
        # FIXME: Seems there is a issue with ArmedField.DISARMED, 0 equals to the DISARMED state
        while (self.objMan.FlightStatus.Armed.value != 0):
             self.objMan.ManualControlCommand.Yaw.value = -1
             self.objMan.ManualControlCommand.updated()
             time.sleep(0.3)

        self.objMan.ManualControlCommand.Yaw.value = 0
        self.objMan.ManualControlCommand.updated()
        time.sleep(1)

        print "Back to self.ManualControl, controlled by RC radio"
        self.objMan.ManualControlCommand.metadata.access = UAVMetaDataObject.Access.READWRITE
        self.objMan.ManualControlCommand.metadata.updated()    
        self.objMan.ManualControlCommand.Connected.value = False
        self.objMan.ManualControlCommand.updated()


def printUsage():
    appName = os.path.basename(sys.argv[0])
    print
    print "usage:"
    print "  %s port o|w|g|s|i" % appName
    print "  o: Show Attitude using an \"observer\""
    print "  w: Show Attitude waiting for updates from flight"
    print "  g: Show Attitude performing get operations"
    print "  s: Drive Servo"
    print "  i: Take control over RC input"
    print
    print "  for example: %s COM30 o" % appName
    print


if __name__ == '__main__':

    if len(sys.argv) != 3:
        print "ERROR: Incorrect number of arguments"
        printUsage()
        sys.exit(2)

    port, option = sys.argv[1:]

    if option not in ["o", "w", "g", "s", "i"]:
        print "ERROR: Invalid option"
        printUsage()
        sys.exit(2)

    # Log everything, and send it to stderr.
    logging.basicConfig(level=logging.INFO)

    try:
        demo = UavtalkDemo()
        demo.setup(port)

        if option == "o":
            demo.showAttitudeViaObserver()  # will not return
        elif option == "w":
            demo.showAttitudeViaWait()  # will not return
        if option == "g":
            demo.showAttitudeViaGet()  # will not return
        if option == "s":
            demo.driveServo()  # will not return
        if option == "i":
            demo.driveInput()  # will not return

    except KeyboardInterrupt:
        pass
    except Exception, e:
        print
        print "An error occured: ", e
        print
        traceback.print_exc()

    print

    try:
        demo.stop()
    except Exception:
        pass
