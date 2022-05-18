/**
 ******************************************************************************
 *
 * @file       configoutputwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Servo output configuration panel for the config gadget
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
#ifndef CONFIGOUTPUTWIDGET_H
#define CONFIGOUTPUTWIDGET_H

#include "cfg_vehicletypes/vehicleconfig.h"

#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "uavobjectutilmanager.h"

#include "systemalarms.h"

#include <QList>
#include <QSlider>

class Ui_OutputWidget;
class OutputChannelForm;
class MixerSettings;

class QLabel;
class QCheckBox;
class QWidget;

class OutputBankControls {
public:
    OutputBankControls(MixerSettings *mixer, QLabel *label, QColor color, QComboBox *rateCombo, QComboBox *modeCombo);
    virtual ~OutputBankControls();

    QLabel *label() const
    {
        return m_label;
    }
    QColor color() const
    {
        return m_color;
    }
    QComboBox *rateCombo() const
    {
        return m_rateCombo;
    }
    QComboBox *modeCombo() const
    {
        return m_modeCombo;
    }

private:
    MixerSettings *m_mixer;
    QLabel *m_label;
    QColor m_color;
    QComboBox *m_rateCombo;
    QComboBox *m_modeCombo;
};

class ConfigOutputWidget : public ConfigTaskWidget {
    Q_OBJECT

public:
    ConfigOutputWidget(QWidget *parent = 0);
    ~ConfigOutputWidget();

public slots:
    void setInputCalibrationState(bool state);

signals:
    void outputConfigSafeChanged(bool newStatus);

protected:
    void enableControls(bool enable);
    void setBoardWarning(QString message);
    void setConfigWarning(QString message);

    virtual void refreshWidgetsValuesImpl(UAVObject *obj);
    virtual void updateObjectsFromWidgetsImpl();

private:
    Ui_OutputWidget *m_ui;
    QList<QSlider> m_sliders;
    int m_mccDataRate;
    UAVObject::Metadata m_accInitialData;
    QList<OutputBankControls> m_banks;
    int activeBanksCount;
    void setBanksEnabled(bool state);

    bool inputCalibrationStarted;
    bool channelTestsStarted;

    OutputChannelForm *getOutputChannelForm(const int index) const;
    void updateChannelInSlider(QSlider *slider, QLabel *min, QLabel *max, QCheckBox *rev, int value);
    void assignOutputChannel(UAVDataObject *obj, QString &str);
    void setColor(QWidget *widget, const QColor color);
    void sendAllChannelTests();
    enum ChannelConfigWarning { None, CannotDriveServo, IsNormalMotorCheckNeutral, IsReversibleMotorCheckNeutral, BiDirectionalDShotNotSupported };
    void setChannelLimits(OutputChannelForm *channelForm, OutputBankControls *bankControls);
    ChannelConfigWarning checkChannelConfig(OutputChannelForm *channelForm, OutputBankControls *bankControls);
    bool checkOutputConfig();
    void updateChannelConfigWarning(ChannelConfigWarning warning);

private slots:
    void updateBoardWarnings(UAVObject *);
    void updateSpinStabilizeCheckComboBoxes();
    void updateAlwaysStabilizeStatus();
    void stopTests();
    void runChannelTests(bool state);
    void sendChannelTest(int index, int value);
    void onBankTypeChange();
};

#endif // CONFIGOUTPUTWIDGET_H
