/**
 ******************************************************************************
 *
 * @file       dialgadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup DialPlugin Dial Plugin
 * @{
 * @brief Plots flight information rotary style dials
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

#include "dialgadgetconfiguration.h"
#include "utils/pathutils.h"

/**
 * Loads a saved configuration.
 *
 */
DialGadgetConfiguration::DialGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    QString dialFile = settings.value("dialFile", "Unknown").toString();

    m_defaultDial      = Utils::InsertDataPath(dialFile);
    dialBackgroundID   = settings.value("dialBackgroundID", "background").toString();
    dialForegroundID   = settings.value("dialForegroundID", "foreground").toString();
    dialNeedleID1      = settings.value("dialNeedleID1", "needle").toString();
    dialNeedleID2      = settings.value("dialNeedleID2", "needle2").toString();
    dialNeedleID3      = settings.value("dialNeedleID3", "needle3").toString();
    needle1MinValue    = settings.value("needle1MinValue", 0).toDouble();
    needle1MaxValue    = settings.value("needle1MaxValue", 100).toDouble();
    needle2MinValue    = settings.value("needle2MinValue", 0).toDouble();
    needle2MaxValue    = settings.value("needle2MaxValue", 100).toDouble();
    needle3MinValue    = settings.value("needle3MinValue", 0).toDouble();
    needle3MaxValue    = settings.value("needle3MaxValue", 100).toDouble();
    needle1DataObject  = settings.value("needle1DataObject").toString();
    needle1ObjectField = settings.value("needle1ObjectField").toString();
    needle2DataObject  = settings.value("needle2DataObject").toString();
    needle2ObjectField = settings.value("needle2ObjectField").toString();
    needle3DataObject  = settings.value("needle3DataObject").toString();
    needle3ObjectField = settings.value("needle3ObjectField").toString();
    needle1Factor      = settings.value("needle1Factor", 1).toDouble();
    needle2Factor      = settings.value("needle2Factor", 1).toDouble();
    needle3Factor      = settings.value("needle3Factor", 1).toDouble();
    needle1Move   = settings.value("needle1Move", "Rotate").toString();
    needle2Move   = settings.value("needle2Move", "Rotate").toString();
    needle3Move   = settings.value("needle3Move", "Rotate").toString();
    font = settings.value("font").toString();
    useOpenGLFlag = settings.value("useOpenGLFlag", false).toBool();
    beSmooth = settings.value("beSmooth", true).toBool();
}

DialGadgetConfiguration::DialGadgetConfiguration(const DialGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    m_defaultDial      = obj.m_defaultDial;
    dialBackgroundID   = obj.dialBackgroundID;
    dialForegroundID   = obj.dialForegroundID;
    dialNeedleID1      = obj.dialNeedleID1;
    dialNeedleID2      = obj.dialNeedleID2;
    dialNeedleID3      = obj.dialNeedleID3;
    needle1MinValue    = obj.needle1MinValue;
    needle1MaxValue    = obj.needle1MaxValue;
    needle2MinValue    = obj.needle2MinValue;
    needle2MaxValue    = obj.needle2MaxValue;
    needle3MinValue    = obj.needle3MinValue;
    needle3MaxValue    = obj.needle3MaxValue;
    needle1DataObject  = obj.needle1DataObject;
    needle1ObjectField = obj.needle1ObjectField;
    needle2DataObject  = obj.needle2DataObject;
    needle2ObjectField = obj.needle2ObjectField;
    needle3DataObject  = obj.needle3DataObject;
    needle3ObjectField = obj.needle3ObjectField;
    needle1Factor      = obj.needle1Factor;
    needle2Factor      = obj.needle2Factor;
    needle3Factor      = obj.needle3Factor;
    needle1Move   = obj.needle1Move;
    needle2Move   = obj.needle2Move;
    needle3Move   = obj.needle3Move;
    font = obj.font;
    useOpenGLFlag = obj.useOpenGLFlag;
    beSmooth = obj.beSmooth;
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *DialGadgetConfiguration::clone() const
{
    return new DialGadgetConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void DialGadgetConfiguration::saveConfig(QSettings &settings) const
{
    QString dialFile = Utils::RemoveDataPath(m_defaultDial);

    settings.setValue("dialFile", dialFile);

    settings.setValue("dialBackgroundID", dialBackgroundID);
    settings.setValue("dialForegroundID", dialForegroundID);

    settings.setValue("dialNeedleID1", dialNeedleID1);
    settings.setValue("dialNeedleID2", dialNeedleID2);
    settings.setValue("dialNeedleID3", dialNeedleID3);

    settings.setValue("needle1MinValue", QString::number(needle1MinValue));
    settings.setValue("needle1MaxValue", QString::number(needle1MaxValue));
    settings.setValue("needle2MinValue", QString::number(needle2MinValue));
    settings.setValue("needle2MaxValue", QString::number(needle2MaxValue));
    settings.setValue("needle3MinValue", QString::number(needle3MinValue));
    settings.setValue("needle3MaxValue", QString::number(needle3MaxValue));

    settings.setValue("needle1DataObject", needle1DataObject);
    settings.setValue("needle1ObjectField", needle1ObjectField);
    settings.setValue("needle2DataObject", needle2DataObject);
    settings.setValue("needle2ObjectField", needle2ObjectField);
    settings.setValue("needle3DataObject", needle3DataObject);
    settings.setValue("needle3ObjectField", needle3ObjectField);

    settings.setValue("needle1Factor", QString::number(needle1Factor));
    settings.setValue("needle2Factor", QString::number(needle2Factor));
    settings.setValue("needle3Factor", QString::number(needle3Factor));

    settings.setValue("needle1Move", needle1Move);
    settings.setValue("needle2Move", needle2Move);
    settings.setValue("needle3Move", needle3Move);

    settings.setValue("font", font);

    settings.setValue("useOpenGLFlag", useOpenGLFlag);
    settings.setValue("beSmooth", beSmooth);
}
