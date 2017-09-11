/**
 ******************************************************************************
 *
 * @file       lineardialgadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup LinearDialPlugin Linear Dial Plugin
 * @{
 * @brief Implements a gadget that displays linear gauges
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

#include "lineardialgadgetconfiguration.h"
#include "utils/pathutils.h"

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
LineardialGadgetConfiguration::LineardialGadgetConfiguration(QString classId, QSettings &settings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    QString dFile = settings.value("dFile").toString();

    dialFile          = Utils::InsertDataPath(dFile);
    sourceDataObject  = settings.value("sourceDataObject", "Unknown").toString();
    sourceObjectField = settings.value("sourceObjectField", "Unknown").toString();
    minValue          = settings.value("minValue", 0).toDouble();
    maxValue          = settings.value("maxValue", 100).toDouble();
    redMin            = settings.value("redMin", 0).toDouble();
    redMax            = settings.value("redMax", 33).toDouble();
    yellowMin         = settings.value("yellowMin", 33).toDouble();
    yellowMax         = settings.value("yellowMax", 66).toDouble();
    greenMin          = settings.value("greenMin", 66).toDouble();
    greenMax          = settings.value("greenMax", 100).toDouble();
    font = settings.value("font").toString();
    decimalPlaces     = settings.value("decimalPlaces", 0).toInt();
    factor            = settings.value("factor", 1.0).toDouble();
    useOpenGLFlag     = settings.value("useOpenGLFlag").toBool();
}

LineardialGadgetConfiguration::LineardialGadgetConfiguration(const LineardialGadgetConfiguration &obj) :
    IUAVGadgetConfiguration(obj.classId(), obj.parent())
{
    dialFile          = obj.dialFile;
    sourceDataObject  = obj.sourceDataObject;
    sourceObjectField = obj.sourceObjectField;
    minValue          = obj.minValue;
    maxValue          = obj.maxValue;
    redMin            = obj.redMin;
    redMax            = obj.redMax;
    yellowMin         = obj.yellowMin;
    yellowMax         = obj.yellowMax;
    greenMin          = obj.greenMin;
    greenMax          = obj.greenMax;
    font = obj.font;
    decimalPlaces     = obj.decimalPlaces;
    factor            = obj.factor;
    useOpenGLFlag     = obj.useOpenGLFlag;
}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *LineardialGadgetConfiguration::clone() const
{
    return new LineardialGadgetConfiguration(*this);
}

/**
 * Saves a configuration.
 *
 */
void LineardialGadgetConfiguration::saveConfig(QSettings &settings) const
{
    QString dFile = Utils::RemoveDataPath(dialFile);

    settings.setValue("dFile", dFile);
    settings.setValue("sourceDataObject", sourceDataObject);
    settings.setValue("sourceObjectField", sourceObjectField);
    settings.setValue("minValue", minValue);
    settings.setValue("maxValue", maxValue);
    settings.setValue("redMin", redMin);
    settings.setValue("redMax", redMax);
    settings.setValue("yellowMin", yellowMin);
    settings.setValue("yellowMax", yellowMax);
    settings.setValue("greenMin", greenMin);
    settings.setValue("greenMax", greenMax);
    settings.setValue("font", font);
    settings.setValue("decimalPlaces", decimalPlaces);
    settings.setValue("factor", factor);
    settings.setValue("useOpenGLFlag", useOpenGLFlag);
}
