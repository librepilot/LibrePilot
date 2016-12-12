#!/usr/bin/python

 ###############################################################################
 #
 # The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 # Script to update vehicle templates files to match current UAVO structure
 # and data.
 #
 ###############################################################################

import json
import re
import collections
import fnmatch
import os

class DecimalEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Decimal):
            return float(obj)
        return json.JSONEncoder.default(self, obj)

json.encoder.FLOAT_REPR = lambda o: format(o, '.17g')

files = []
for root, dirnames, filenames in os.walk('ground/gcs/src/share/vehicletemplates/'):
  for filename in fnmatch.filter(filenames, '*.optmpl'):
    files.append(os.path.join(root, filename))

for f in files:
    data = json.load(open(f, 'r'), object_pairs_hook=collections.OrderedDict)

    for item in data['objects']:
        fieldsToRemove = []
        for i in item['fields']:
            name = i['name']
            values = i['values']
            if re.compile('ThrustPIDScaleCurve').match(name):
                i['type'] = "int8"
                for j in values:
                    j['value'] = int(j['value'] * 100)
            elif re.compile('AcroInsanityFactor').match(name):
                i['type'] = "int8"
                value = 0
                for j in values:
                    value = int(j['value'] * 100)
                values.pop()
                values.append({'name': 'roll', 'value': value})
                values.append({'name': 'pitch', 'value': value})
                values.append({'name': 'yaw', 'value': value})
            elif re.compile('FeedForward').match(name):
                fieldsToRemove.append(i)
            elif re.compile('MaxAccel').match(name):
                fieldsToRemove.append(i)
            elif re.compile('AccelTime').match(name):
                fieldsToRemove.append(i)
            elif re.compile('DecelTime').match(name):
                fieldsToRemove.append(i)
        for field in fieldsToRemove:
            item['fields'].remove(field)
    with open(f, 'w') as outfile:
        json.dump(data, outfile, indent=4, separators=(',', ': '), cls=DecimalEncoder)
