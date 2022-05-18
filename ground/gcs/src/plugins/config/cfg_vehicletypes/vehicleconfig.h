/**
 ******************************************************************************
 *
 * @file       vehicleconfig.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief bit storage of config ui settings
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
#ifndef VEHICLECONFIG_H
#define VEHICLECONFIG_H

#include "../uavobjectwidgetutils/configtaskwidget.h"

#include "actuatorcommand.h"

class UAVDataObject;

typedef struct {
    uint    VTOLMotorN : 4;
    uint    VTOLMotorS : 4;
    uint    VTOLMotorE : 4;
    uint    VTOLMotorW : 4;
    uint    VTOLMotorNW : 4;
    uint    VTOLMotorNE : 4;
    uint    VTOLMotorSW : 4;
    uint    VTOLMotorSE : 4; // 32 bits
    // OctoX
    uint    VTOLMotorNNE : 4;
    uint    VTOLMotorENE : 4;
    uint    VTOLMotorESE : 4;
    uint    VTOLMotorSSE : 4;
    uint    VTOLMotorSSW : 4;
    uint    VTOLMotorWSW : 4;
    uint    VTOLMotorWNW : 4;
    uint    VTOLMotorNNW : 4; // 64 bits
    uint    TRIYaw : 4;
    uint    Accessory0 : 4;
    uint    Accessory1 : 4;
    uint    Accessory2 : 4;
    uint    Accessory3 : 4;
    quint32 padding : 12; // 96 bits
    quint32 padding1; // 128 bits
} __attribute__((packed))  multiGUISettingsStruct;

typedef struct {
    uint    SwashplateType : 4;
    uint    FirstServoIndex : 2;
    uint    CorrectionAngle : 9;
    uint    ccpmCollectivePassthroughState : 1;
    uint    ccpmLinkCyclicState : 1;
    uint    ccpmLinkRollState : 1;
    uint    SliderValue0 : 7;
    uint    SliderValue1 : 7;
    uint    SliderValue2 : 7; // 41 bits
    uint    ServoIndexW : 4;
    uint    ServoIndexX : 4;
    uint    ServoIndexY : 4;
    uint    ServoIndexZ : 4; // 57 bits
    uint    Throttle : 4;
    uint    Tail : 4; // 65bits
    quint32 padding : 30; // 96 bits
    quint32 padding1; // 128 bits
} __attribute__((packed))  heliGUISettingsStruct;

typedef struct {
    uint    FixedWingThrottle : 4;
    uint    FixedWingRoll1 : 4;
    uint    FixedWingRoll2 : 4;
    uint    FixedWingPitch1 : 4;
    uint    FixedWingPitch2 : 4;
    uint    FixedWingYaw1 : 4;
    uint    FixedWingYaw2 : 4;
    uint    Accessory0 : 4;  // 32 bits
    uint    Accessory1 : 4;
    uint    Accessory2 : 4;
    uint    Accessory3 : 4;
    uint    Accessory0_2 : 4;
    uint    Accessory1_2 : 4;
    uint    Accessory2_2 : 4;
    uint    Accessory3_2 : 4;
    quint32 padding : 4; // 64bits
    quint32 padding1;
    quint32 padding2; // 128 bits
} __attribute__((packed))  fixedGUISettingsStruct;

typedef struct {
    uint    GroundVehicleThrottle1 : 4;
    uint    GroundVehicleThrottle2 : 4;
    uint    GroundVehicleSteering1 : 4;
    uint    GroundVehicleSteering2 : 4;
    uint    padding : 16; // 32 bits
    quint32 padding1;
    quint32 padding2;
    quint32 padding3; // 128 bits
} __attribute__((packed))  groundGUISettingsStruct;

typedef struct {
    uint Motor1 : 4;
    uint Motor2 : 4;
    uint Motor3 : 4;
    uint Motor4 : 4;
    uint Motor5 : 4;
    uint Motor6 : 4;
    uint Motor7 : 4;
    uint Motor8 : 4; // 32 bits
    uint Servo1 : 4;
    uint Servo2 : 4;
    uint Servo3 : 4;
    uint Servo4 : 4;
    uint Servo5 : 4;
    uint Servo6 : 4;
    uint Servo7 : 4;
    uint Servo8 : 4; // 64 bits
    uint RevMotor1 : 4;
    uint RevMotor2 : 4;
    uint RevMotor3 : 4;
    uint RevMotor4 : 4;
    uint RevMotor5 : 4;
    uint RevMotor6 : 4;
    uint RevMotor7 : 4;
    uint RevMotor8 : 4; // 96 bits
    uint Accessory0 : 4;
    uint Accessory1 : 4;
    uint Accessory2 : 4;
    uint Accessory3 : 4;
    uint Accessory4 : 4;
    uint Accessory5 : 4;
    uint padding : 8; // 128 bits
} __attribute__((packed))  customGUISettingsStruct;


typedef union {
    uint UAVObject[4]; // 32 bits * 4
    heliGUISettingsStruct   heli; // 128 bits
    fixedGUISettingsStruct  fixedwing;
    multiGUISettingsStruct  multi;
    groundGUISettingsStruct ground;
    customGUISettingsStruct custom;
} GUIConfigDataUnion;

class ConfigVehicleTypeWidget;

/*
 * This class handles vehicle specific configuration UI and associated logic.
 *
 * VehicleConfig derives from ConfigTaskWidget but is not a top level ConfigTaskWidget.
 * VehicleConfig objects are nested within the ConfigVehicleTypeWidget and have particularities:
 * - bindings are added to the parent (i.e. ConfigVehicleConfigWidget)
 * - auto bindings are not supported
 * - as a consequence things like dirty state management are bypassed and delegated to the parent class.
 */
class VehicleConfig : public ConfigTaskWidget {
    Q_OBJECT

    friend ConfigVehicleTypeWidget;

public:

    /* Enumeration options for ThrottleCurves */
    typedef enum {
        MIXER_THROTTLECURVE1 = 0, MIXER_THROTTLECURVE2 = 1
    } MixerThrottleCurveElem;

    /* Enumeration options for field MixerType */
    typedef enum {
        MIXERTYPE_DISABLED        = 0,
        MIXERTYPE_MOTOR           = 1,
        MIXERTYPE_REVERSABLEMOTOR = 2,
        MIXERTYPE_SERVO           = 3,
        MIXERTYPE_CAMERAROLL      = 4,
        MIXERTYPE_CAMERAPITCH     = 5,
        MIXERTYPE_CAMERAYAW       = 6,
        MIXERTYPE_ACCESSORY0      = 7,
        MIXERTYPE_ACCESSORY1      = 8,
        MIXERTYPE_ACCESSORY2      = 9,
        MIXERTYPE_ACCESSORY3      = 10,
        MIXERTYPE_ACCESSORY4      = 11,
        MIXERTYPE_ACCESSORY5      = 12
    } MixerTypeElem;

    /* Array element names for field MixerVector */
    typedef enum {
        MIXERVECTOR_THROTTLECURVE1 = 0,
        MIXERVECTOR_THROTTLECURVE2 = 1,
        MIXERVECTOR_ROLL  = 2,
        MIXERVECTOR_PITCH = 3,
        MIXERVECTOR_YAW   = 4
    } MixerVectorElem;

    static const quint32 CHANNEL_NUMELEM = ActuatorCommand::CHANNEL_NUMELEM;;

    static GUIConfigDataUnion getConfigData();
    static void setConfigData(GUIConfigDataUnion configData);

    static void resetField(UAVObjectField *field);

    static void setComboCurrentIndex(QComboBox *box, int index);
    static void enableComboBoxes(QWidget *owner, QString boxName, int boxCount, bool enable);

    // VehicleConfig class
    VehicleConfig(QWidget *parent = 0);
    ~VehicleConfig();

    virtual QString getFrameType();

    double getMixerValue(UAVDataObject *mixer, QString elementName);
    void setMixerValue(UAVDataObject *mixer, QString elementName, double value);

protected:
    QStringList channelNames;
    QStringList mixerTypes;
    QStringList mixerVectors;
    QStringList mixerTypeDescriptions;

    void populateChannelComboBoxes();

    double  getMixerVectorValue(UAVDataObject *mixer, int channel, MixerVectorElem elementName);
    void    setMixerVectorValue(UAVDataObject *mixer, int channel, MixerVectorElem elementName, double value);
    void    resetMixerVector(UAVDataObject *mixer, int channel);
    void    resetMotorAndServoMixers(UAVDataObject *mixer);
    void    resetAllMixersType(UAVDataObject *mixer);
    QString getMixerType(UAVDataObject *mixer, int channel);
    void    setMixerType(UAVDataObject *mixer, int channel, MixerTypeElem mixerType);
    void    setThrottleCurve(UAVDataObject *mixer, MixerThrottleCurveElem curveType, QList<double> curve);
    void    getThrottleCurve(UAVDataObject *mixer, MixerThrottleCurveElem curveType, QList<double> *curve);
    bool    isValidThrottleCurve(QList<double> *curve);
    double  getCurveMin(QList<double> *curve);
    double  getCurveMax(QList<double> *curve);

    virtual void enableControls(bool enable);
    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

    virtual void registerWidgets(ConfigTaskWidget &parent);
    virtual void setupUI(QString frameType);

protected slots:
    void frameTypeChanged(QString frameType);

private:
    static UAVObjectManager *getUAVObjectManager();
};

#endif // VEHICLECONFIG_H
