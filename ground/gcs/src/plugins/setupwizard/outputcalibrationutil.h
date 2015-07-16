/**
 ******************************************************************************
 *
 * @file       outputcalibrationutil.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup
 * @{
 * @addtogroup OutputCalibrationUtil
 * @{
 * @brief
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

#ifndef OUTPUTCALIBRATIONUTIL_H
#define OUTPUTCALIBRATIONUTIL_H

#include <QObject>
#include "actuatorcommand.h"


class OutputCalibrationUtil : public QObject {
    Q_OBJECT
public:
    explicit OutputCalibrationUtil(QObject *parent = 0);
    ~OutputCalibrationUtil();

    static void startOutputCalibration();
    static void stopOutputCalibration();
    static ActuatorCommand *getActuatorCommandObject();

public slots:
    void startChannelOutput(quint16 channel, quint16 safeValue);
    void startChannelOutput(QList<quint16> &channels, quint16 safeValue);
    void stopChannelOutput();
    void setChannelOutputValue(quint16 value);

private:
    static bool c_prepared;
    static ActuatorCommand::Metadata c_savedActuatorCommandMetaData;
    QList<quint16> m_outputChannels;
    quint16 m_safeValue;
};

#endif // OUTPUTCALIBRATIONUTIL_H
