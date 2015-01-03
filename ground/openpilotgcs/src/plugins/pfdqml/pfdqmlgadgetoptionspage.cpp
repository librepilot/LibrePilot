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

    options_page->useGPSLocation->setChecked(m_config->positionMode() == Pfd::GPS);
    options_page->useHomeLocation->setChecked(m_config->positionMode() == Pfd::Home);
    options_page->usePredefinedLocation->setChecked(m_config->positionMode() == Pfd::Predefined);

    options_page->useOnlyCache->setChecked(m_config->cacheOnly());

    // Terrain file chooser
    options_page->earthFile->setExpectedKind(Utils::PathChooser::File);
    options_page->earthFile->setPromptDialogFilter(tr("OsgEarth (*.earth)"));
    options_page->earthFile->setPromptDialogTitle(tr("Choose Terrain File"));
    options_page->earthFile->setPath(m_config->terrainFile());

    // Terrain position
    options_page->latitude->setText(QString::number(m_config->latitude()));
    options_page->longitude->setText(QString::number(m_config->longitude()));
    options_page->altitude->setText(QString::number(m_config->altitude()));

    // Model file chooser
    options_page->modelFile->setExpectedKind(Utils::PathChooser::File);
    options_page->modelFile->setPromptDialogFilter(tr("Model file (*.3ds)"));
    options_page->modelFile->setPromptDialogTitle(tr("Choose Model File"));
    options_page->modelFile->setPath(m_config->modelFile());

    // Model check boxes
    options_page->useAutomaticModel->setChecked(m_config->modelSelectionMode() == Pfd::Auto);
    options_page->usePredefinedModel->setChecked(m_config->modelSelectionMode() == Pfd::Fixed);

    // OpenGL
    options_page->useOpenGL->setChecked(m_config->openGLEnabled());

#ifndef USE_OSG
    options_page->showTerrain->setChecked(false);
    options_page->showTerrain->setVisible(false);
#endif

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
#else
    m_config->setTerrainEnabled(false);
#endif

    m_config->setTerrainFile(options_page->earthFile->path());

    if (options_page->useGPSLocation->isChecked()) {
        m_config->setPositionMode(Pfd::GPS);
    } else if (options_page->useHomeLocation->isChecked()) {
        m_config->setPositionMode(Pfd::Home);
    } else {
        m_config->setPositionMode(Pfd::Predefined);
    }
    m_config->setLatitude(options_page->latitude->text().toDouble());
    m_config->setLongitude(options_page->longitude->text().toDouble());
    m_config->setAltitude(options_page->altitude->text().toDouble());
    m_config->setCacheOnly(options_page->useOnlyCache->isChecked());

#ifdef USE_OSG
    m_config->setModelEnabled(options_page->showModel->isChecked());
#else
    m_config->setModelEnabled(false);
#endif

    m_config->setModelFile(options_page->modelFile->path());

    if (options_page->useAutomaticModel->isChecked()) {
        m_config->setModelSelectionMode(Pfd::Auto);
    } else {
        m_config->setModelSelectionMode(Pfd::Fixed);
    }

    m_config->setOpenGLEnabled(options_page->useOpenGL->isChecked());
}

void PfdQmlGadgetOptionsPage::finish()
{}
