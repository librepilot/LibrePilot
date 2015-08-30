/**
 ******************************************************************************
 *
 * @file       main.cpp
 * @author     The LibrePilot Team http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      UAVObjectGenerator main.
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
#include <QtCore/QCoreApplication>
#include <QFile>
#include <QString>
#include <QStringList>
#include <iostream>

#include "generators/java/uavobjectgeneratorjava.h"
#include "generators/flight/uavobjectgeneratorflight.h"
#include "generators/gcs/uavobjectgeneratorgcs.h"
#include "generators/matlab/uavobjectgeneratormatlab.h"
#include "generators/python/uavobjectgeneratorpython.h"
#include "generators/wireshark/uavobjectgeneratorwireshark.h"

#define RETURN_ERR_USAGE 1
#define RETURN_ERR_XML   2
#define RETURN_OK        0

using namespace std;

/**
 * print usage info
 */
void usage()
{
    cout << "Usage: uavobjectgenerator [language] [-v] xml_path template_base [UAVObj1] ... [UAVObjN]" << endl;
    cout << "Languages: " << endl;
    cout << "\t-gcs           build groundstation code" << endl;
    cout << "\t-flight        build flight code" << endl;
    cout << "\t-java          build java code" << endl;
    cout << "\t-python        build python code" << endl;
    cout << "\t-matlab        build matlab code" << endl;
    cout << "\t-wireshark     build wireshark plugin" << endl;
    cout << "\tIf no language is specified none are built - just parse xmls." << endl;
    cout << "Misc: " << endl;
    cout << "\t-h or --help   this help" << endl;
    cout << "\t-v             verbose" << endl;
    cout << "\tinput_path     path to UAVObject definition (.xml) files." << endl;
    cout << "\ttemplate_path  path to the root of the source tree." << endl;
    cout << "\tUAVObjXY       name of a specific UAVObject to be built." << endl;
    cout << "\tIf any specific UAVObjects are given only these will be built." << endl;
    cout << "\tIf no UAVObject is specified -> all are built." << endl;
}

/**
 * inform user of invalid usage
 */
int usage_err()
{
    cout << "Invalid usage!" << endl;
    usage();
    return RETURN_ERR_USAGE;
}

/**
 * entrance
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    cout << "- LibrePilot UAVObject Generator -" << endl;

    QString inputpath;
    QString templatepath;
    QString outputpath;
    QStringList arguments_stringlist;
    QStringList objects_stringlist;

    // process arguments
    for (int argi = 1; argi < argc; argi++) {
        arguments_stringlist << argv[argi];
    }

    if ((arguments_stringlist.removeAll("-h") > 0) || (arguments_stringlist.removeAll("--help") > 0)) {
        usage();
        return RETURN_OK;
    }

    bool verbose       = (arguments_stringlist.removeAll("-v") > 0);
    bool do_gcs        = (arguments_stringlist.removeAll("-gcs") > 0);
    bool do_flight     = (arguments_stringlist.removeAll("-flight") > 0);
    bool do_java       = (arguments_stringlist.removeAll("-java") > 0);
    bool do_python     = (arguments_stringlist.removeAll("-python") > 0);
    bool do_matlab     = (arguments_stringlist.removeAll("-matlab") > 0);
    bool do_wireshark  = (arguments_stringlist.removeAll("-wireshark") > 0);

    bool do_allObjects = true;

    if (arguments_stringlist.length() >= 2) {
        inputpath    = arguments_stringlist.at(0);
        templatepath = arguments_stringlist.at(1);
    } else {
        // wrong number of arguments
        return usage_err();
    }
    if (arguments_stringlist.length() > 2) {
        do_allObjects = false;
        for (int argi = 2; argi < arguments_stringlist.length(); argi++) {
            objects_stringlist << (arguments_stringlist.at(argi).toLower() + ".xml");
        }
    }

    if (!inputpath.endsWith("/")) {
        inputpath.append("/"); // append a slash if it is not there
    }
    if (!templatepath.endsWith("/")) {
        templatepath.append("/"); // append a slash if it is not there
    }
    // put all output files in the current directory
    outputpath = QString("./");

    QDir xmlPath = QDir(inputpath);
    UAVObjectParser *parser = new UAVObjectParser();

    QStringList filters     = QStringList("*.xml");

    xmlPath.setNameFilters(filters);
    QFileInfoList xmlList   = xmlPath.entryInfoList();

    // Read in each XML file and parse object(s) in them

    for (int n = 0; n < xmlList.length(); ++n) {
        QFileInfo fileinfo = xmlList[n];
        if (!do_allObjects) {
            if (!objects_stringlist.removeAll(fileinfo.fileName().toLower())) {
                if (verbose) {
                    cout << "Skipping XML file: " << fileinfo.fileName().toStdString() << endl;
                }
                continue;
            }
        }
        if (verbose) {
            cout << "Parsing XML file: " << fileinfo.fileName().toStdString() << endl;
        }
        QString filename = fileinfo.fileName();
        QString xmlstr   = readFile(fileinfo.absoluteFilePath());

        QString res = parser->parseXML(xmlstr, filename);

        if (!res.isNull()) {
            if (!verbose) {
                cout << "Error in XML file: " << fileinfo.fileName().toStdString() << endl;
            }
            cout << "Error parsing " << res.toStdString() << endl;
            return RETURN_ERR_XML;
        }
    }

    if (objects_stringlist.length() > 0) {
        cout << "required UAVObject definitions not found! " << objects_stringlist.join(",").toStdString() << endl;
        return RETURN_ERR_XML;
    }

    // check for duplicate object ID's
    QList<quint32> objIDList;
    int numBytesTotal = 0;
    for (int objidx = 0; objidx < parser->getNumObjects(); ++objidx) {
        quint32 id = parser->getObjectID(objidx);
        numBytesTotal += parser->getNumBytes(objidx);
        if (verbose) {
            cout << "Checking object " << parser->getObjectName(objidx).toStdString() << " (" << parser->getNumBytes(objidx) << " bytes)" << endl;
        }
        if (objIDList.contains(id) || id == 0) {
            cout << "Error: Object ID collision found in object " << parser->getObjectName(objidx).toStdString() << ", modify object name" << endl;
            return RETURN_ERR_XML;
        }

        objIDList.append(id);
    }

    // done parsing and checking
    cout << "Done: processed " << xmlList.length() << " XML files and generated "
         << objIDList.length() << " objects with no ID collisions. Total size of the data fields is " << numBytesTotal << " bytes." << endl;


    if (verbose) {
        cout << "used units: " << parser->all_units.join(",").toStdString() << endl;
    }

    if (do_flight) {
        // generate flight code if wanted
        cout << "generating flight code" << endl;
        UAVObjectGeneratorFlight flightgen;
        flightgen.generate(parser, templatepath, outputpath);
    } else if (do_gcs) {
        // generate gcs code if wanted
        cout << "generating gcs code" << endl;
        UAVObjectGeneratorGCS gcsgen;
        gcsgen.generate(parser, templatepath, outputpath);
    } else if (do_java) {
        // generate java code if wanted
        cout << "generating java code" << endl;
        UAVObjectGeneratorJava javagen;
        javagen.generate(parser, templatepath, outputpath);
    } else if (do_python) {
        // generate python code if wanted
        cout << "generating python code" << endl;
        UAVObjectGeneratorPython pygen;
        pygen.generate(parser, templatepath, outputpath);
    } else if (do_matlab) {
        // generate matlab code if wanted
        cout << "generating matlab code" << endl;
        UAVObjectGeneratorMatlab matlabgen;
        matlabgen.generate(parser, templatepath, outputpath);
    } else if (do_wireshark) {
        // generate wireshark plugin if wanted
        cout << "generating wireshark code" << endl;
        UAVObjectGeneratorWireshark wiresharkgen;
        wiresharkgen.generate(parser, templatepath, outputpath);
    }

    return RETURN_OK;
}
