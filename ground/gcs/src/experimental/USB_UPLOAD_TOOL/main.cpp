/**
 ******************************************************************************
 *
 * @file       main.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Uploader Serial and USB Uploader tool
 * @{
 * @brief The USB and Serial protocol uploader tool
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
#include "dfu.h"

#include <QCoreApplication>
#include <QFile>
#include <QByteArray>
#include <QThread>
#include <QStringList>
#include <QDebug>

#include <iostream>

void showProgress(QString status);
void progressUpdated(int percent);
void usage(QTextStream *standardOutput);

QString label;

// Command Line Options
#define DOWNLOAD        "-dn"
#define DEVICE          "-d"
#define DOWNDESCRIPTION "-dd"
#define PROGRAMFW       "-p"
#define PROGRAMDESC     "-w"
#define VERIFY          "-v"
#define COMPARECRC      "-cc"
#define COMPAREALL      "-ca"
#define STATUSREQUEST   "-s"
#define LISTDEVICES     "-ls"
#define RESET           "-r"
#define JUMP            "-j"
#define USE_SERIAL      "-t"
#define NO_COUNTDOWN    "-i"
#define HELP            "-h"
#define DEBUG           "-debug"
#define USERMODERESET   "-ur"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QTextStream standardOutput(stdout);
    int argumentCount = QCoreApplication::arguments().size();
    bool use_serial   = false;
    bool verify;
    bool debug = false;
    // bool umodereset   = false;
    DFU::Actions action = DFU::actionNone;
    QString file;
    QString serialport;
    QString description;
    int device = -1;
    QStringList args = QCoreApplication::arguments();

    if (args.contains(DEBUG)) {
        debug = true;
    }
    if (args.contains(USERMODERESET)) {
        // umodereset = true;
    }
    standardOutput << "Serial firmware uploader tool." << endl;
    if (args.contains(DEVICE)) {
        if (args.indexOf(DEVICE) + 1 < args.length()) {
            device = (args[args.indexOf(DEVICE) + 1]).toInt();
        }
    } else {
        device = 0;
    }

    if (argumentCount == 0 || args.contains(HELP) || !args.contains(USE_SERIAL)) {
        usage(&standardOutput);
        return -1;
    } else if (args.contains(PROGRAMFW)) {
        if (args.contains(VERIFY)) {
            verify = true;
        } else {
            verify = false;
        }
        if (args.contains(PROGRAMDESC)) {
            if (args.indexOf(PROGRAMDESC) + 1 < args.length()) {
                description = (args[args.indexOf(PROGRAMDESC) + 1]);
            }
        }
        if (args.indexOf(PROGRAMFW) + 1 < args.length()) {
            file = args[args.indexOf(PROGRAMFW) + 1];
        } else {
            // error
        }
        action = DFU::actionProgram;
    } else if (args.contains(COMPARECRC) || args.contains(COMPAREALL)) {
        if (args.contains(COMPARECRC)) {
            if (args.indexOf(COMPARECRC) + 1 < args.length()) {
                file = args[args.indexOf(COMPARECRC) + 1];
            } else {
                // error
            }
            action = DFU::actionCompareCrc;
        } else {
            if (args.indexOf(COMPAREALL) + 1 < args.length()) {
                file = args[args.indexOf(COMPAREALL) + 1];
            } else {
                // error
            }
            action = DFU::actionCompareAll;
        }
    } else if (args.contains(DOWNLOAD)) {
        if (args.indexOf(DOWNLOAD) + 1 < args.length()) {
            file = args[args.indexOf(DOWNLOAD) + 1];
        } else {
            // error
        }
        action = DFU::actionDownload;
    } else if (args.contains(STATUSREQUEST)) {
        action = DFU::actionStatusReq;
    } else if (args.contains(RESET)) {
        action = DFU::actionReset;
    } else if (args.contains(JUMP)) {
        action = DFU::actionJump;
    } else if (args.contains(LISTDEVICES)) {
        action = DFU::actionListDevs;
    }
    if ((file.isEmpty() || device == -1) && action != DFU::actionReset && action != DFU::actionStatusReq && action != DFU::actionListDevs && action != DFU::actionJump) {
        usage(&standardOutput);
        return -1;
    }
    if (args.contains(USE_SERIAL)) {
        if (args.indexOf(USE_SERIAL) + 1 < args.length()) {
            serialport = (args[args.indexOf(USE_SERIAL) + 1]);
            use_serial = true;
        }
    }
    if (debug) {
        qDebug() << "Action=" << (int)action;
        qDebug() << "File=" << file;
        qDebug() << "Device=" << device;
        qDebug() << "Action=" << action;
        qDebug() << "Description" << description;
        qDebug() << "Use Serial port" << use_serial;
        if (use_serial) {
            qDebug() << "Port Name" << serialport;
        }
    }
    if (use_serial) {
        if (args.contains(NO_COUNTDOWN)) {} else {
            showProgress("Connect the board");
            for (int i = 0; i < 6; i++) {
                progressUpdated(i * 100 / 5);
                QThread::msleep(500);
            }
        }
        standardOutput << endl << "Connect the board NOW" << endl;
        QThread::msleep(1000);
    }

    ///////////////////////////////////ACTIONS START///////////////////////////////////////////////////

    DFU::DFUObject dfu(debug, use_serial, serialport);

    QObject::connect(&dfu, &DFU::DFUObject::operationProgress, showProgress);
    QObject::connect(&dfu, &DFU::DFUObject::progressUpdated, progressUpdated);

    if (!dfu.ready()) {
        return -1;
    }
    dfu.AbortOperation();
    if (!dfu.enterDFU(0)) {
        standardOutput << "Could not enter DFU mode\n" << endl;
        return -1;
    }
    if (debug) {
        DFU::Status ret = dfu.StatusRequest();
        qDebug() << dfu.StatusToString(ret);
    }
    if (!(action == DFU::actionStatusReq || action == DFU::actionReset || action == DFU::actionJump)) {
        dfu.findDevices();
        if (action == DFU::actionListDevs) {
            standardOutput << "Found " << dfu.numberOfDevices << "\n";
            for (int x = 0; x < dfu.numberOfDevices; ++x) {
                standardOutput << "Device #" << x << "\n";
                standardOutput << "Device ID=" << dfu.devices[x].ID << "\n";
                standardOutput << "Device Readable=" << dfu.devices[x].Readable << "\n";
                standardOutput << "Device Writable=" << dfu.devices[x].Writable << "\n";
                standardOutput << "Device SizeOfCode=" << dfu.devices[x].SizeOfCode << "\n";
                standardOutput << "BL Version=" << dfu.devices[x].BL_Version << "\n";
                standardOutput << "Device SizeOfDesc=" << dfu.devices[x].SizeOfDesc << "\n";
                standardOutput << "FW CRC=" << dfu.devices[x].FW_CRC << "\n";

                int size = ((DFU::device)dfu.devices[x]).SizeOfDesc;
                dfu.enterDFU(x);
                standardOutput << "Description:" << dfu.DownloadDescription(size).toLatin1().data() << "\n";
                standardOutput << "\n";
            }
            return 0;
        }
        if (device > dfu.numberOfDevices - 1) {
            standardOutput << "Error:Invalid Device" << endl;
            return -1;
        }
        if (dfu.numberOfDevices == 1) {
            dfu.use_delay = false;
        }
        if (!dfu.enterDFU(device)) {
            standardOutput << "Error:Could not enter DFU mode\n" << endl;
            return -1;
        }
        if (action == DFU::actionProgram) {
            if (((DFU::device)dfu.devices[device]).Writable == false) {
                standardOutput << "ERROR device not Writable\n" << endl;
                return false;
            }
            standardOutput << "Uploading..." << endl;

            // this call is asynchronous so the only false status it will report
            // is when it is already running...
            bool retstatus = dfu.UploadFirmware(file.toLatin1(), verify, device);

            if (!retstatus) {
                standardOutput << "Upload failed with code:" << retstatus << endl;
                return -1;
            }
            while (!dfu.isFinished()) {
                QThread::msleep(500);
            }
            // TODO check if upload went well...
            if (file.endsWith("opfw")) {
                QFile fwfile(file);
                if (!fwfile.open(QIODevice::ReadOnly)) {
                    standardOutput << "Cannot open file " << file << endl;
                    return -1;
                }
                QByteArray firmware = fwfile.readAll();
                QByteArray desc     = firmware.right(100);
                DFU::Status status  = dfu.UploadDescription(desc);
                if (status != DFU::Last_operation_Success) {
                    standardOutput << "Upload failed with code:" << retstatus << endl;
                    return -1;
                }
            } else if (!description.isEmpty()) {
                DFU::Status status = dfu.UploadDescription(description);
                if (status != DFU::Last_operation_Success) {
                    standardOutput << "Upload failed with code:" << status << endl;
                    return -1;
                }
            }
            while (!dfu.isFinished()) {
                QThread::msleep(500);
            }
            standardOutput << "Uploading Succeeded!\n" << endl;
        } else if (action == DFU::actionDownload) {
            if (((DFU::device)dfu.devices[device]).Readable == false) {
                standardOutput << "ERROR device not readable\n" << endl;
                return false;
            }
            QByteArray fw;
            dfu.DownloadFirmware(&fw, 0);
            while (!dfu.isFinished()) {
                QThread::msleep(500);
            }
            bool ret = dfu.SaveByteArrayToFile(file.toLatin1(), fw);
            return ret;
        } else if (action == DFU::actionCompareCrc) {
            dfu.CompareFirmware(file.toLatin1(), DFU::crccompare, device);
            return 1;
        } else if (action == DFU::actionCompareAll) {
            if (((DFU::device)dfu.devices[device]).Readable == false) {
                standardOutput << "ERROR device not readable\n" << endl;
                return false;
            }
            dfu.CompareFirmware(file.toLatin1(), DFU::bytetobytecompare, device);
            return 1;
        }
    } else if (action == DFU::actionStatusReq) {
        standardOutput << "Current device status=" << dfu.StatusToString(dfu.StatusRequest()).toLatin1().data() << "\n" << endl;
    } else if (action == DFU::actionReset) {
        dfu.ResetDevice();
    } else if (action == DFU::actionJump) {
        dfu.JumpToApp(false, false);
    }
    return 0;

    // return a.exec();
}

void showProgress(QString status)
{
    QTextStream standardOutput(stdout);

    standardOutput << status << endl;
    label = status;
}

void progressUpdated(int percent)
{
    std::string bar;

    for (int i = 0; i < 50; i++) {
        if (i < (percent / 2)) {
            bar.replace(i, 1, "=");
        } else if (i == (percent / 2)) {
            bar.replace(i, 1, ">");
        } else {
            bar.replace(i, 1, " ");
        }
    }
    std::cout << "\r" << label.toLatin1().data() << " [" << bar << "] ";
    std::cout.width(3);
    std::cout << percent << "%     " << std::flush;
}

void usage(QTextStream *standardOutput)
{
    *standardOutput << "Options:\n";
    *standardOutput << "-ls                  : lists available devices\n";
    *standardOutput << "-p <file>            : program hw (requires:-d - optionals:-v,-w)\n";
    *standardOutput << "-v                   : verify (requires:-d)\n";
    *standardOutput << "-dn <file>           : download firmware to file\n";
    // *standardOutput  << "-dd <file>           : download discription (requires:-d)\n";
    *standardOutput << "-d <Device Number>   : target device number (default 0, first device)\n";
    // *standardOutput  << "-w <description>     : (requires: -p)\n";
    *standardOutput << "-ca <file>           : compares byte by byte current firmware with file\n";
    *standardOutput << "-cc <file>           : compares CRC  of current firmware with file\n";
    *standardOutput << "-s                   : requests status of device\n";
    *standardOutput << "-r                   : resets the device\n";
    *standardOutput << "-j                   : exits bootloader and jumps to user FW\n";
    *standardOutput << "-debug               : prints debug information\n";
    *standardOutput << "-t <port>            : uses serial port\n";
    *standardOutput << "-i                   : immediate, doesn't show the connection countdown\n";
    *standardOutput << "-h                   : print usage\n";
    // *standardOutput  << "-ur <port>           : user mode reset*\n";
    // *standardOutput  << "  *requires valid user space firmwares already running\n";
    *standardOutput << "\n";
    *standardOutput << "Program and verify the first device connected to COM1\n";
    *standardOutput << "  UploadTool -p c:/gpsp.opfw -v -t COM1\n";
    *standardOutput << "\n";
    *standardOutput << "Perform a quick compare of FW in file with FW on device #1\n";
    *standardOutput << "  UploadTool -ca /home/user1/gpsp.opfw -t ttyUSB0\n";
}

void howToUsage(QTextStream *standardOutput)
{
    *standardOutput << "run the tool with -h for more informations" << endl;
}
