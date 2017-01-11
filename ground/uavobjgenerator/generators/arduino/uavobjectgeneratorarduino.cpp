/**
 ******************************************************************************
 *
 * @file       uavobjectgeneratorarduino.cpp
 * @author     The LibrePilot Project, https://www.librepilot.org, Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      produce arduino code for uavobjects
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "uavobjectgeneratorarduino.h"

using namespace std;

bool UAVObjectGeneratorArduino::generate(UAVObjectParser *parser, QString templatepath, QString outputpath)
{
    fieldTypeStrC << "int8_t" << "int16_t" << "int32_t" << "uint8_t"
                  << "uint16_t" << "uint32_t" << "float" << "uint8_t";

    arduinoCodePath   = QDir(templatepath + QString(ARDUINO_CODE_DIR));
    arduinoOutputPath = QDir(outputpath);
    arduinoOutputPath.mkpath(arduinoOutputPath.absolutePath());

    arduinoIncludeTemplate = readFile(arduinoCodePath.absoluteFilePath("inc/uavobject.h.template"));

    if (arduinoIncludeTemplate.isNull()) {
        cerr << "Error: Could not open arduino template files." << endl;
        return false;
    }

    for (int objidx = 0; objidx < parser->getNumObjects(); ++objidx) {
        ObjectInfo *info = parser->getObjectByIndex(objidx);
        process_object(info);
    }
    return true; // if we come here everything should be fine
}


/**
 * Generate the Arduino object files
 **/
bool UAVObjectGeneratorArduino::process_object(ObjectInfo *info)
{
    if (info == NULL) {
        return false;
    }

    // Prepare output strings
    QString outInclude = arduinoIncludeTemplate;

    // Replace common tags
    replaceCommonTags(outInclude, info);

    // Use the appropriate typedef for enums where we find them. Set up
    // that StringList here for use below.
    QStringList typeList;
    for (int n = 0; n < info->fields.length(); ++n) {
        if (info->fields[n]->type == FIELDTYPE_ENUM) {
            typeList << QString("%1%2Options").arg(info->name).arg(info->fields[n]->name);
        } else {
            typeList << fieldTypeStrC[info->fields[n]->type];
        }
    }

    // Replace the $(DATAFIELDS) tag
    QString fields;
    QString dataStructures;
    for (int n = 0; n < info->fields.length(); ++n) {
        // Determine type
        QString type = typeList[n];
        // Append field
        // Check if it is a named set and create structures accordingly
        if (info->fields[n]->numElements > 1) {
            if (info->fields[n]->elementNames[0].compare("0") != 0) {
                QString structTypeName = QString("%1%2Data").arg(info->name).arg(info->fields[n]->name);
                QString structType     = "typedef struct __attribute__ ((__packed__)) {\n";
                for (int f = 0; f < info->fields[n]->elementNames.count(); f++) {
                    structType.append(QString("    %1 %2;\n").arg(type).arg(info->fields[n]->elementNames[f]));
                }
                structType.append(QString("}  %1 ;\n").arg(structTypeName));
                structType.append("typedef struct __attribute__ ((__packed__)) {\n");
                structType.append(QString("    %1 array[%2];\n").arg(type).arg(info->fields[n]->elementNames.count()));
                structType.append(QString("}  %1Array ;\n").arg(structTypeName));
                structType.append(QString("#define %1%2ToArray( var ) UAVObjectFieldToArray( %3, var )\n\n").arg(info->name).arg(info->fields[n]->name).arg(structTypeName));

                dataStructures.append(structType);

                fields.append(QString("    %1 %2;\n").arg(structTypeName)
                              .arg(info->fields[n]->name));
            } else {
                fields.append(QString("    %1 %2[%3];\n").arg(type)
                              .arg(info->fields[n]->name).arg(info->fields[n]->numElements));
            }
        } else {
            fields.append(QString("    %1 %2;\n").arg(type).arg(info->fields[n]->name));
        }
    }
    outInclude.replace("$(DATAFIELDS)", fields);
    outInclude.replace("$(DATASTRUCTURES)", dataStructures);
    // Replace the $(DATAFIELDINFO) tag
    QString enums;
    for (int n = 0; n < info->fields.length(); ++n) {
        enums.append(QString("/* Field %1 information */\n").arg(info->fields[n]->name));
        // Only for enum types
        if (info->fields[n]->type == FIELDTYPE_ENUM) {
            enums.append(QString("\n// Enumeration options for field %1\n").arg(info->fields[n]->name));
            enums.append("typedef enum __attribute__ ((__packed__)) {\n");
            // Go through each option
            QStringList options = info->fields[n]->options;
            for (int m = 0; m < options.length(); ++m) {
                QString s = (m == (options.length() - 1)) ? "    %1_%2_%3=%4\n" : "    %1_%2_%3=%4,\n";
                enums.append(s
                             .arg(info->name.toUpper())
                             .arg(info->fields[n]->name.toUpper())
                             .arg(options[m].toUpper().replace(QRegExp(ENUM_SPECIAL_CHARS), ""))
                             .arg(m));
            }
            enums.append(QString("} %1%2Options;\n")
                         .arg(info->name)
                         .arg(info->fields[n]->name));
        }
        // Generate element names (only if field has more than one element)
        if (info->fields[n]->numElements > 1 && !info->fields[n]->defaultElementNames) {
            enums.append(QString("\n// Array element names for field %1\n").arg(info->fields[n]->name));
            enums.append("typedef enum {\n");
            // Go through the element names
            QStringList elemNames = info->fields[n]->elementNames;
            for (int m = 0; m < elemNames.length(); ++m) {
                QString s = (m != (elemNames.length() - 1)) ? "    %1_%2_%3=%4,\n" : "    %1_%2_%3=%4\n";
                enums.append(s
                             .arg(info->name.toUpper())
                             .arg(info->fields[n]->name.toUpper())
                             .arg(elemNames[m].toUpper())
                             .arg(m));
            }
            enums.append(QString("} %1%2Elem;\n")
                         .arg(info->name)
                         .arg(info->fields[n]->name));
        }
        // Generate array information
        if (info->fields[n]->numElements > 1) {
            enums.append(QString("\n// Number of elements for field %1\n").arg(info->fields[n]->name));
            enums.append(QString("#define %1_%2_NUMELEM %3\n")
                         .arg(info->name.toUpper())
                         .arg(info->fields[n]->name.toUpper())
                         .arg(info->fields[n]->numElements));
        }

        enums.append("\n");
    }
    outInclude.replace("$(DATAFIELDINFO)", enums);

    // Write the arduino code
    bool res = writeFileIfDifferent(arduinoOutputPath.absolutePath() + "/" + info->namelc + ".h", outInclude);
    if (!res) {
        cout << "Error: Could not write arduino include files" << endl;
        return false;
    }

    return true;
}
