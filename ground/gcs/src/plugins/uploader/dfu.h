/**
 ******************************************************************************
 *
 * @file       dfu.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Uploader Uploader Plugin
 * @{
 * @brief The uploader plugin
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
#ifndef DFU_H
#define DFU_H

#include <QByteArray>
#include <QThread>
#include <QMutex>
#include <QList>
#include <QVariant>

#define MAX_PACKET_DATA_LEN 255
#define MAX_PACKET_BUF_SIZE (1 + 1 + MAX_PACKET_DATA_LEN + 2)

#define BUF_LEN             64

// serial
class qsspt;

// usb
class opHID_hidapi;

namespace DFU {
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
Q_NAMESPACE
#endif
enum TransferTypes {
    FW,
    Descript
};

enum CompareType {
    crccompare,
    bytetobytecompare
};

Q_ENUMS(Status)
enum Status {
    DFUidle, // 0
    uploading, // 1
    wrong_packet_received, // 2
    too_many_packets, // 3
    too_few_packets, // 4
    Last_operation_Success, // 5
    downloading, // 6
    idle, // 7
    Last_operation_failed, // 8
    uploadingStarting, // 9
    outsideDevCapabilities, // 10
    CRC_Fail, // 11
    failed_jump, // 12
    abort // 13
};

enum Actions {
    actionNone,
    actionProgram,
    actionProgramAndVerify,
    actionDownload,
    actionCompareAll,
    actionCompareCrc,
    actionListDevs,
    actionStatusReq,
    actionReset,
    actionJump
};

enum Commands {
    Reserved, // 0
    Req_Capabilities, // 1
    Rep_Capabilities, // 2
    EnterDFU, // 3
    JumpFW, // 4
    Reset, // 5
    Abort_Operation, // 6
    Upload, // 7
    END, // 8
    Download_Req, // 9
    Download, // 10
    Status_Request, // 11
    Status_Rep, // 12
};

enum eBoardType {
    eBoardUnkwn   = 0,
    eBoardMainbrd = 1,
    eBoardINS,
    eBoardPip     = 3,
    eBoardCC = 4,
    eBoardRevo    = 9,
    eBoardSparky2 = 0x92,
};

struct device {
    quint16 ID;
    quint32 FW_CRC;
    quint8  BL_Version;
    int     SizeOfDesc;
    quint32 SizeOfCode;
    bool    Readable;
    bool    Writable;
};

class DFUObject : public QThread {
    Q_OBJECT;

public:
    static quint32 CRCFromQBArray(QByteArray array, quint32 Size);

    DFUObject(bool debug, bool use_serial, QString port);

    virtual ~DFUObject();

    // Service commands:
    bool enterDFU(int const &devNumber);
    bool findDevices();
    int JumpToApp(bool safeboot, bool erase);
    int ResetDevice(void);
    DFU::Status StatusRequest();
    bool EndOperation();
    int AbortOperation(void);
    bool ready()
    {
        return mready;
    }

    // Upload (send to device) commands
    DFU::Status UploadDescription(QVariant description);
    bool UploadFirmware(const QString &sfile, const bool &verify, int device);

    // Download (get from device) commands:
    // DownloadDescription is synchronous
    QString DownloadDescription(int const & numberOfChars);
    QByteArray DownloadDescriptionAsBA(int const & numberOfChars);
    // Asynchronous firmware download: initiates fw download,
    // and a downloadFinished signal is emitted when download
    // if finished:
    bool DownloadFirmware(QByteArray *byteArray, int device);

    // Comparison functions (is this needed?)
    DFU::Status CompareFirmware(const QString &sfile, const CompareType &type, int device);

    bool SaveByteArrayToFile(QString const & file, QByteArray const &array);

    // Variables:
    QList<device> devices;
    int numberOfDevices;
    int send_delay;
    bool use_delay;

    // Helper functions:
    QString StatusToString(DFU::Status const & status);
    static quint32 CRC32WideFast(quint32 Crc, quint32 Size, quint32 *Buffer);
    DFU::eBoardType GetBoardType(int boardNum);

signals:
    void progressUpdated(int);
    void downloadFinished();
    void uploadFinished(DFU::Status);
    void operationProgress(QString status);

private:
    // Generic variables:
    bool debug;
    bool use_serial;
    bool mready;
    int RWFlags;

    // Serial
    qsspt *serialhandle;

    // USB
    opHID_hidapi *hidHandle;

    int sendData(void *, int);
    int receiveData(void *data, int size);
    uint8_t sspTxBuf[MAX_PACKET_BUF_SIZE];
    uint8_t sspRxBuf[MAX_PACKET_BUF_SIZE];

    int setStartBit(int command)
    {
        return command | 0x20;
    }

    void CopyWords(char *source, char *destination, int count);
    void printProgBar(int const & percent, QString const & label);
    bool StartUpload(qint32 const &numberOfBytes, TransferTypes const & type, quint32 crc);
    bool UploadData(qint32 const & numberOfPackets, QByteArray & data);

    // Thread management:
    // Same as startDownload except that we store in an external array:
    bool StartDownloadT(QByteArray *fw, qint32 const & numberOfBytes, TransferTypes const & type);
    DFU::Status UploadFirmwareT(const QString &sfile, const bool &verify, int device);
    QMutex mutex;
    DFU::Commands requestedOperation;
    qint32 requestSize;
    DFU::TransferTypes requestTransferType;
    QByteArray *requestStorage;
    QString requestFilename;
    bool requestVerify;
    int requestDevice;

protected:
    void run(); // Executes the upload or download operations
};
}

Q_DECLARE_METATYPE(DFU::Status)


#endif // DFU_H
