#ifndef DEVICEDESCRIPTORSTRUCT_H
#define DEVICEDESCRIPTORSTRUCT_H

#include <QString>
struct deviceDescriptorStruct {
public:
    QString        gitHash;
    QString        gitDate;
    QString        gitTag;
    QByteArray     fwHash;
    QByteArray     uavoHash;
    quint8         boardType;
    quint8         boardRevision;
    static QString idToBoardName(quint16 id)
    {
        switch (id) {
        case 0x0101:
            // MB
            return QString("OpenPilot MainBoard");

            break;
        case 0x0201:
            // INS
            return QString("OpenPilot INS");

            break;
        case 0x0301:
            // OPLink Mini
            return QString("OPLinkMini");

            break;
        case 0x0401:
            // Coptercontrol
            return QString("CopterControl");

            break;
        case 0x0402:
            // Coptercontrol 3D
            // It would be nice to say CC3D here but since currently we use string comparisons
            // for firmware compatibility and the filename path that would break
            return QString("CopterControl");

            break;
        case 0x0901:
            // old unreleased Revolution prototype
            return QString("Revolution");

            break;
        case 0x0903:
            // Revo also known as Revo Mini
            return QString("Revolution");

            break;
        case 0x0904:
            return QString("DiscoveryF4");

            break;
        case 0x0905:
            // Nano
            return QString("RevoNano");

        case 0x9201:
            // Sparky 2.0
            return QString("Sparky2");

        case 0x1001:
            return QString("SPRacingF3");

        case 0x1002:
            return QString("SPRacingF3EVO");

        case 0x1003:
            return QString("NucleoF303RE");

        case 0x1005:
            return QString("PikoBLX");

        case 0x1006:
            return QString("tinyFISH");

        default:
            return QString("");

            break;
        }
    }
    deviceDescriptorStruct() {}
};

#endif // DEVICEDESCRIPTORSTRUCT_H
