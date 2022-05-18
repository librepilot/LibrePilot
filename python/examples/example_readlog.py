##
##############################################################################
#
# @file       example_readlog.py
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



import logging
import serial
import traceback
import sys

from librepilot.uavtalk.uavobject import *
from librepilot.uavtalk.uavtalk import *
from librepilot.uavtalk.objectManager import *
from librepilot.uavtalk.connectionManager import *
from example import UavtalkDemo


def _hex02(value):
    return "%02X" % value
    
class UavtalkLogDemo(UavtalkDemo):
    
    def __init__(self):
        self.nbUpdates = 0
        self.lastRateCalc = time.time()
        self.updateRate = 0
        self.objMan = None
        self.connMan = None
        
    def setup(self, filename):
        print "Opening File \"%s\"" % filename
        file = open(filename,"rb")
        if file == None:
            raise IOError("Failed to open file")
        
        print "Creating UavTalk"
        self.uavTalk = UavTalk(None, filename)

        print "Starting ObjectManager"
        self.objMan = ObjManager(self.uavTalk)
        self.objMan.importDefinitions()

        print "Starting UavTalk"
        self.uavTalk.start()

def printUsage():
    appName = os.path.basename(sys.argv[0])
    print
    print "usage:"
    print "  %s filename " % appName
    print
    print "  for example: %s /tmp/OP-2015-04-28_23-16-33.opl" % appName
    print 
    
if __name__ == '__main__':
    
    if len(sys.argv) !=2:
        print "ERROR: Incorrect number of arguments"
        print len(sys.argv)
        printUsage()
        sys.exit(2)

    script, filename = sys.argv
    if not os.path.exists(sys.argv[1]):
        sys.exit('ERROR: Database %s was not found!' % sys.argv[1])

    # Log everything, and send it to stderr.
    logging.basicConfig(level=logging.INFO)

    try:
        demo = UavtalkLogDemo()
        demo.setup(filename)

        while True:
            demo.objMan.AttitudeState.waitUpdate()
            demo._onAttitudeUpdate(demo.objMan.AttitudeState)
            
    except KeyboardInterrupt:
        pass
    except Exception,e:
        print
        print "An error occured: ", e
        print
        traceback.print_exc()
    
    print
    


