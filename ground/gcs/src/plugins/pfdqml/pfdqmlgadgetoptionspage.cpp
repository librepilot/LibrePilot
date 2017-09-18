/**
 ******************************************************************************
 *
 * @file       pfdqmlgadgetoptionspage.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
 *****************************************************************************//*
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

#include "pfdqmlgadgetoptionspage.h"
#include "pfdqmlgadgetconfiguration.h"
#include "ui_pfdqmlgadgetoptionspage.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavdataobject.h"

#include <QFileDialog>
#include <QtAlgorithms>
#include <QStringList>

PfdQmlGadgetOptionsPage::PfdQmlGadgetOptionsPage(PfdQmlGadgetConfiguration *config, QObject *parent) :
    IOptionsPage(parent), options_page(NULL), m_config(config)
{}

// creates options page widget (uses the UI file)
QWidget *PfdQmlGadgetOptionsPage::createPage(QWidget *parent)
{
    options_page = new Ui::PfdQmlGadgetOptionsPage();
    // main widget
    QWidget *optionsPageWidget = new QWidget(parent);
    // main layout
    options_page->setupUi(optionsPageWidget);

    // QML file chooser
    options_page->qmlSourceFile->setExpectedKind(Utils::PathChooser::File);
    options_page->qmlSourceFile->setPromptDialogFilter(tr("QML file (*.qml)"));
    options_page->qmlSourceFile->setPromptDialogTitle(tr("Choose QML File"));
    options_page->qmlSourceFile->setPath(m_config->qmlFile());

    // Speed Unit combo
    QMapIterator<double, QString> iter = m_config->speedMapIterator();
    while (iter.hasNext()) {
        iter.next();
        options_page->speedUnitCombo->addItem(iter.value(), iter.key());
    }
    options_page->speedUnitCombo->setCurrentIndex(options_page->speedUnitCombo->findData(m_config->speedFactor()));

    // Altitude Unit combo
    iter = m_config->altitudeMapIterator();
    while (iter.hasNext()) {
        iter.next();
        options_page->altUnitCombo->addItem(iter.value(), iter.key());
    }
    options_page->altUnitCombo->setCurrentIndex(options_page->altUnitCombo->findData(m_config->altitudeFactor()));

    // Terrain check boxes
    options_page->showTerrain->setChecked(m_config->terrainEnabled());

    // Terrain file chooser
    options_page->earthFile->setExpectedKind(Utils::PathChooser::File);
    options_page->earthFile->setPromptDialogFilter(tr("OsgEarth (*.earth)"));
    options_page->earthFile->setPromptDialogTitle(tr("Choose Terrain File"));
    options_page->earthFile->setPath(m_config->terrainFile());

    // Terrain position
    options_page->latitude->setText(QString::number(m_config->latitude()));
    options_page->longitude->setText(QString::number(m_config->longitude()));
    options_page->altitude->setText(QString::number(m_config->altitude()));

    options_page->useOnlyCache->setChecked(m_config->cacheOnly());

    // Sky options
    options_page->useLocalTime->setChecked(m_config->timeMode() == TimeMode::Local);
    options_page->usePredefinedTime->setChecked(m_config->timeMode() == TimeMode::Predefined);
    options_page->dateEdit->setDate(m_config->dateTime().date());
    options_page->timeEdit->setTime(m_config->dateTime().time());
    options_page->minAmbientLightSpinBox->setValue(m_config->minAmbientLight());

    // Model check boxes
    options_page->showModel->setChecked(m_config->modelEnabled());
    options_page->useAutomaticModel->setChecked(m_config->modelSelectionMode() == ModelSelectionMode::Auto);
    options_page->usePredefinedModel->setChecked(m_config->modelSelectionMode() == ModelSelectionMode::Predefined);

    // Model file chooser
    options_page->modelFile->setExpectedKind(Utils::PathChooser::File);
    options_page->modelFile->setPromptDialogFilter(tr("Model file (*.3ds)"));
    options_page->modelFile->setPromptDialogTitle(tr("Choose Model File"));
    options_page->modelFile->setPath(m_config->modelFile());

    // Background image chooser
    options_page->backgroundImageFile->setExpectedKind(Utils::PathChooser::File);
    // options_page->backgroundImageFile->setPromptDialogFilter(tr("Image file"));
    options_page->backgroundImageFile->setPromptDialogTitle(tr("Choose Background Image File"));
    options_page->backgroundImageFile->setPath(m_config->backgroundImageFile());

    // gstreamer pipeline
    options_page->pipelineTextEdit->setPlainText(m_config->gstPipeline());

#ifndef USE_OSG
    options_page->showTerrain->setChecked(false);
    options_page->showTerrain->setVisible(false);
#endif

    QObject::connect(options_page->actualizeDateTimeButton, SIGNAL(clicked()),
                     this, SLOT(actualizeDateTime()));

    return optionsPageWidget;
}

/**
 * Called when the user presses apply or OK.
 *
 * Saves the current values
 *
 */
void PfdQmlGadgetOptionsPage::apply()
{
    m_config->setQmlFile(options_page->qmlSourceFile->path());

    m_config->setSpeedFactor(options_page->speedUnitCombo->itemData(options_page->speedUnitCombo->currentIndex()).toDouble());
    m_config->setAltitudeFactor(options_page->altUnitCombo->itemData(options_page->altUnitCombo->currentIndex()).toDouble());

#ifdef USE_OSG
    m_config->setTerrainEnabled(options_page->showTerrain->isChecked());
    m_config->setTerrainFile(options_page->earthFile->path());

    m_config->setLatitude(options_page->latitude->text().toDouble());
    m_config->setLongitude(options_page->longitude->text().toDouble());
    m_config->setAltitude(options_page->altitude->text().toDouble());
    m_config->setCacheOnly(options_page->useOnlyCache->isChecked());

    if (options_page->useLocalTime->isChecked()) {
        m_config->setTimeMode(TimeMode::Local);
    } else {
        m_config->setTimeMode(TimeMode::Predefined);
    }
    QDateTime dateTime(options_page->dateEdit->date(), options_page->timeEdit->time());
    m_config->setDateTime(dateTime);
    m_config->setMinAmbientLight(options_page->minAmbientLightSpinBox->value());

#else // ifdef USE_OSG
    m_config->setTerrainEnabled(false);
#endif // ifdef USE_OSG

#ifdef USE_OSG
    m_config->setModelEnabled(options_page->showModel->isChecked());
    m_config->setModelFile(options_page->modelFile->path());

    if (options_page->useAutomaticModel->isChecked()) {
        m_config->setModelSelectionMode(ModelSelectionMode::Auto);
    } else {
        m_config->setModelSelectionMode(ModelSelectionMode::Predefined);
    }
    m_config->setBackgroundImageFile(options_page->backgroundImageFile->path());
#else
    m_config->setModelEnabled(false);
#endif

    m_config->setGstPipeline(options_page->pipelineTextEdit->toPlainText());
}

void PfdQmlGadgetOptionsPage::finish()
{}

void PfdQmlGadgetOptionsPage::actualizeDateTime()
{
    QDateTime dateTime = QDateTime::currentDateTime();

    options_page->dateEdit->setDate(dateTime.date());
    options_page->timeEdit->setTime(dateTime.time());
}
