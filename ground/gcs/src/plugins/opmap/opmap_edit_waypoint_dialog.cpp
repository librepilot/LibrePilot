/**
 ******************************************************************************
 *
 * @file       opmap_edit_waypoint_dialog.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OPMapPlugin OpenPilot Map Plugin
 * @{
 * @brief The OpenPilot Map plugin
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

#include "opmap_edit_waypoint_dialog.h"
#include "ui_opmap_edit_waypoint_dialog.h"
#include "opmapcontrol/opmapcontrol.h"
#include "widgetdelegates.h"
// *********************************************************************

// constructor
opmap_edit_waypoint_dialog::opmap_edit_waypoint_dialog(QWidget *parent, QAbstractItemModel *model, QItemSelectionModel *selection) :
    QWidget(parent, Qt::Window), model(model), itemSelection(selection),
    ui(new Ui::opmap_edit_waypoint_dialog)
{
    ui->setupUi(this);
    ui->pushButtonPrevious->setEnabled(false);
    ui->pushButtonNext->setEnabled(false);
    connect(ui->checkBoxLocked, SIGNAL(toggled(bool)), this, SLOT(enableEditWidgets(bool)));
    connect(ui->cbMode, SIGNAL(currentIndexChanged(int)), this, SLOT(setupModeWidgets()));
    connect(ui->cbCondition, SIGNAL(currentIndexChanged(int)), this, SLOT(setupConditionWidgets()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), this, SLOT(pushButtonCancel_clicked()));
    MapDataDelegate::loadComboBox(ui->cbMode, flightDataModel::MODE);
    MapDataDelegate::loadComboBox(ui->cbCondition, flightDataModel::CONDITION);
    MapDataDelegate::loadComboBox(ui->cbCommand, flightDataModel::COMMAND);
    mapper = new QDataWidgetMapper(this);

    mapper->setItemDelegate(new MapDataDelegate(this));
    connect(mapper, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
    mapper->setModel(model);
    mapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
    mapper->addMapping(ui->checkBoxLocked, flightDataModel::LOCKED);
    mapper->addMapping(ui->doubleSpinBoxLatitude, flightDataModel::LATPOSITION);
    mapper->addMapping(ui->doubleSpinBoxLongitude, flightDataModel::LNGPOSITION);
    mapper->addMapping(ui->doubleSpinBoxAltitude, flightDataModel::ALTITUDE);
    mapper->addMapping(ui->lineEditDescription, flightDataModel::WPDESCRIPTION);
    mapper->addMapping(ui->checkBoxRelative, flightDataModel::ISRELATIVE);
    mapper->addMapping(ui->doubleSpinBoxBearing, flightDataModel::BEARELATIVE);
    mapper->addMapping(ui->doubleSpinBoxVelocity, flightDataModel::VELOCITY);
    mapper->addMapping(ui->doubleSpinBoxDistance, flightDataModel::DISRELATIVE);
    mapper->addMapping(ui->doubleSpinBoxRelativeAltitude, flightDataModel::ALTITUDERELATIVE);
    mapper->addMapping(ui->cbMode, flightDataModel::MODE);
    mapper->addMapping(ui->dsb_modeParam1, flightDataModel::MODE_PARAMS0);
    mapper->addMapping(ui->dsb_modeParam2, flightDataModel::MODE_PARAMS1);
    mapper->addMapping(ui->dsb_modeParam3, flightDataModel::MODE_PARAMS2);
    mapper->addMapping(ui->dsb_modeParam4, flightDataModel::MODE_PARAMS3);

    mapper->addMapping(ui->cbCondition, flightDataModel::CONDITION);
    mapper->addMapping(ui->dsb_condParam1, flightDataModel::CONDITION_PARAMS0);
    mapper->addMapping(ui->dsb_condParam2, flightDataModel::CONDITION_PARAMS1);
    mapper->addMapping(ui->dsb_condParam3, flightDataModel::CONDITION_PARAMS2);
    mapper->addMapping(ui->dsb_condParam4, flightDataModel::CONDITION_PARAMS3);

    mapper->addMapping(ui->cbCommand, flightDataModel::COMMAND);
    mapper->addMapping(ui->sbJump, flightDataModel::JUMPDESTINATION);
    mapper->addMapping(ui->sbError, flightDataModel::ERRORDESTINATION);
    connect(itemSelection, SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), this, SLOT(currentRowChanged(QModelIndex, QModelIndex)));

    ui->descriptionCommandLabel->setText(tr("<p>The Command specifies the transition to the next state (aka waypoint), as well as when it "
                                            "is to be executed. This command will always switch to another waypoint instantly, but which waypoint depends on the Condition.</p>"
                                            "<p>The JumpDestination is the waypoint to jump to in unconditional or conditional jumps.</p>"));

    ui->descriptionErrorDestinationLabel->setText(tr("<p>The ErrorDestination is special; it allows exception handling based on the PathFollower. "
                                                     "If the PathFollower indicates that it is unable to execute Mode, it indicates this error in the PathStatus UAVObject. "
                                                     "If that happens, then the sequence of waypoints is interrupted and the waypoint in ErrorDestination becomes the Active Waypoint. "
                                                     "A thinkable use case for this functionality would be to steer towards a safe emergency landing site if the engine fails.</p>"));
}

void opmap_edit_waypoint_dialog::currentIndexChanged(int index)
{
    ui->lbNumber->setText(QString::number(index + 1));
    ui->wpNumberSpinBox->setValue(index + 1);

    bool isMin = (index == 0);
    bool isMax = ((index + 1) == mapper->model()->rowCount());
    ui->pushButtonPrevious->setEnabled(!isMin);
    ui->pushButtonNext->setEnabled(!isMax);

    QModelIndex idx = mapper->model()->index(index, 0);
    if (index == itemSelection->currentIndex().row()) {
        return;
    }
    itemSelection->clear();
    itemSelection->setCurrentIndex(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

opmap_edit_waypoint_dialog::~opmap_edit_waypoint_dialog()
{
    delete ui;
}

void opmap_edit_waypoint_dialog::setupModeWidgets()
{
    MapDataDelegate::ModeOptions mode = (MapDataDelegate::ModeOptions)ui->cbMode->itemData(ui->cbMode->currentIndex()).toInt();

    ui->modeParam1->setText("");
    ui->modeParam2->setText("");
    ui->modeParam3->setText("");
    ui->modeParam4->setText("");
    ui->modeParam1->setEnabled(false);
    ui->modeParam2->setEnabled(false);
    ui->modeParam3->setEnabled(false);
    ui->modeParam4->setEnabled(false);
    ui->dsb_modeParam1->setEnabled(false);
    ui->dsb_modeParam2->setEnabled(false);
    ui->dsb_modeParam3->setEnabled(false);
    ui->dsb_modeParam4->setEnabled(false);

    switch (mode) {
    case MapDataDelegate::MODE_GOTOENDPOINT:
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot will try to hold position at a fixed end-coordinate. "
                                             "If elsewhere, the craft will steer towards the coordinate but not "
                                             "necessarily in a straight line as drift will not be compensated.</p>"));
        break;
    case MapDataDelegate::MODE_FOLLOWVECTOR:
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot will attempt to stay on a defined trajectory from the previous waypoint "
                                             "to the current one. Any deviation from this path, for example by wind, will be corrected. "
                                             "This is the best and obvious choice for direct straight-line (rhumb line) navigation in any vehicle.</p>"));
        break;
    case MapDataDelegate::MODE_CIRCLERIGHT:
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot will attempt to fly a circular trajectory around a waypoint. "
                                             "The curve radius is defined by the distance from the previous waypoint, therefore setting the coordinate "
                                             "of the waypoint perpendicular to the previous flight path leg (and on the side the vehicle is supposed to turn toward!) "
                                             "will lead to the smoothest possible transition.</p><p>Staying in FlyCircle for a prolonged time will lead to a circular "
                                             "loiter trajectory around a waypoint, but this mode can be used in combination with the PointingTowardsNext-EndCondition "
                                             "to stay on the curved trajectory until the vehicle is moving towards the next waypoint, which allows for very controlled "
                                             "turns in flight paths.</p>"));
        break;
    case MapDataDelegate::MODE_CIRCLELEFT:
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot will attempt to fly a circular trajectory around a waypoint. "
                                             "The curve radius is defined by the distance from the previous waypoint, therefore setting the coordinate "
                                             "of the waypoint perpendicular to the previous flight path leg (and on the side the vehicle is supposed to turn toward!) "
                                             "will lead to the smoothest possible transition.</p><p>Staying in FlyCircle for a prolonged time will lead to a circular "
                                             "loiter trajectory around a waypoint, but this mode can be used in combination with the PointingTowardsNext-EndCondition "
                                             "to stay on the curved trajectory until the vehicle is moving towards the next waypoint, which allows for very controlled "
                                             "turns in flight paths.</p>"));
        break;
    case MapDataDelegate::MODE_DISARMALARM:
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot is instructed to force a CRITICAL PathFollower alarm. A PathFollower alarm of type CRITICAL "
                                             "is supposed to automatically set the vehicle to DISARMED and thus disable the engines, cut fuel supply, ...</p><p>"
                                             "The PathFollower can do this during any mode in case of emergency, but through this mode the PathPlanner "
                                             "can force this behavior, for example after a landing.</p>"));
        break;
    case MapDataDelegate::MODE_AUTOTAKEOFF:
        // FixedWing do not use parameters, vertical velocity can be set for multirotors as param0
        ui->modeParam1->setText("Speed (m/s):");
        ui->modeParam1->setEnabled(true);
        ui->dsb_modeParam1->setEnabled(true);
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot will engage a hardcoded, fully automated takeoff sequence. "
                                             "Using a fixed attitude, heading towards the destination waypoint for a fixed wing or "
                                             "a vertical climb for a multirotor.</p>"
                                             "<p>Vertical speed in meters/second, for multirotors. (Will be around 0.6m/s)</p>"));
        break;
    case MapDataDelegate::MODE_LAND:
        // FixedWing do not use parameters, vertical velocity can be set for multirotors as param0 (0.1/0.6m/s range)
        ui->modeParam1->setText("Speed (m/s):");
        ui->modeParam1->setEnabled(true);
        ui->dsb_modeParam1->setEnabled(true);
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot will engage a hardcoded, fully automated landing sequence. "
                                             "Using a fixed attitude, heading towards the destination waypoint for a fixed wing or "
                                             "a vertical descent for a multirotor.</p>"
                                             "<p>Vertical speed in meters/second, for multirotors. (Will be around 0.6m/s)</p>"));
        break;
    case MapDataDelegate::MODE_BRAKE:
        ui->descriptionModeLabel->setText(tr("<p>This mode is used internally by assisted flight modes with multirotors to slow down the velocity.</p>"));
        break;
    case MapDataDelegate::MODE_VELOCITY:
        ui->descriptionModeLabel->setText(tr("<p>This mode is used internally by assisted flight modes with multirotors to maintain a velocity in 3D space.</p>"));
        break;
    case MapDataDelegate::MODE_FIXEDATTITUDE:
        ui->modeParam1->setText("Roll:");
        ui->modeParam2->setText("Pitch:");
        ui->modeParam3->setText("Yaw:");
        ui->modeParam4->setText("Thrust:");
        ui->modeParam1->setEnabled(true);
        ui->modeParam2->setEnabled(true);
        ui->modeParam3->setEnabled(true);
        ui->modeParam4->setEnabled(true);
        ui->dsb_modeParam1->setEnabled(true);
        ui->dsb_modeParam2->setEnabled(true);
        ui->dsb_modeParam3->setEnabled(true);
        ui->dsb_modeParam4->setEnabled(true);
        ui->descriptionModeLabel->setText(tr("<p>The Autopilot will play dumb and simply instruct the Stabilization Module to assume a certain Roll, Pitch angle "
                                             "and optionally a Yaw rotation rate and Thrust setting. This allows for very simple auto-takeoff and "
                                             "auto-landing maneuvers.</p>"
                                             "<p>Roll(+-180°) and Pitch (+-90°) angles in degrees</p>"
                                             "<p>Yaw rate in deg/s</p>"
                                             "<p>Thrust from 0 to 1</p>"));
        break;
    case MapDataDelegate::MODE_SETACCESSORY:
        ui->modeParam1->setText("Acc.channel:");
        ui->modeParam2->setText("Value:");
        ui->modeParam1->setEnabled(true);
        ui->modeParam2->setEnabled(true);
        ui->dsb_modeParam1->setEnabled(true);
        ui->dsb_modeParam2->setEnabled(true);
        ui->descriptionModeLabel->setText(tr("<p>Not yet implemented.</p>"));
        // ui->descriptionModeLabel->setText(tr("<p>In this mode, the PathFollower is supposed to inject a specific value (-1|0|+1 range) into an AccessoryDesired channel. "
        // "This would allow one to control arbitrary auxilliary components from the PathPlanner like flaps and landing gear.</p>"));
        break;
    default:
        ui->descriptionModeLabel->setText("");
        break;
    }
}

void opmap_edit_waypoint_dialog::setupConditionWidgets()
{
    MapDataDelegate::EndConditionOptions mode = (MapDataDelegate::EndConditionOptions)ui->cbCondition->itemData(ui->cbCondition->currentIndex()).toInt();

    ui->condParam1->setText("");
    ui->condParam2->setText("");
    ui->condParam3->setText("");
    ui->condParam4->setText("");
    ui->condParam1->setEnabled(false);
    ui->condParam2->setEnabled(false);
    ui->condParam3->setEnabled(false);
    ui->condParam4->setEnabled(false);
    ui->dsb_condParam1->setEnabled(false);
    ui->dsb_condParam2->setEnabled(false);
    ui->dsb_condParam3->setEnabled(false);
    ui->dsb_condParam4->setEnabled(false);

    switch (mode) {
    case MapDataDelegate::ENDCONDITION_NONE:
        ui->descriptionConditionLabel->setText(tr("<p>This condition is always false. A WaypointAction with EndCondition to None will stay in "
                                                  "its mode until forever, or until an Error in the PathFollower triggers the ErrorJump. (For example out of fuel!)</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_IMMEDIATE:
        ui->descriptionConditionLabel->setText(tr("<p>Opposite to the None condition, the immediate condition is always true.</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_PYTHONSCRIPT:
        ui->descriptionConditionLabel->setText(tr("<p>Not yet implemented.</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_TIMEOUT:
        ui->condParam1->setEnabled(true);
        ui->dsb_condParam1->setEnabled(true);
        ui->condParam1->setText("Timeout (s)");
        ui->descriptionConditionLabel->setText(tr("<p>The Timeout condition measures time this waypoint is active (in seconds).</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_DISTANCETOTARGET:
        ui->condParam1->setEnabled(true);
        ui->condParam2->setEnabled(true);
        ui->dsb_condParam1->setEnabled(true);
        ui->dsb_condParam2->setEnabled(true);
        ui->condParam1->setText("Distance (m):");
        ui->condParam2->setText("Flag:");
        ui->descriptionConditionLabel->setText(tr("<p>The DistanceToTarget condition measures the distance to a waypoint, returns true if closer.</p>"
                                                  "<p>Flag: <b>0</b>= 2D distance, <b>1</b>= 3D distance</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_LEGREMAINING:
        ui->condParam1->setEnabled(true);
        ui->dsb_condParam1->setEnabled(true);
        ui->condParam1->setText("Relative Distance:");
        ui->descriptionConditionLabel->setText(tr("<p>The LegRemaining condition measures how far the pathfollower got on a linear path segment, returns true "
                                                  "if closer to destination(path more complete).</p>"
                                                  "<p><b>0</b>=complete, <b>1</b>=just starting</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_BELOWERROR:
        ui->condParam1->setEnabled(true);
        ui->dsb_condParam1->setEnabled(true);
        ui->condParam1->setText("Error margin (m):");
        ui->descriptionConditionLabel->setText(tr("<p>The BelowError condition measures the error on a path segment, returns true if error is below margin in meters.</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_ABOVEALTITUDE:
        ui->condParam1->setEnabled(true);
        ui->dsb_condParam1->setEnabled(true);
        ui->condParam1->setText("Altitude (m):");
        ui->descriptionConditionLabel->setText(tr("<p>The AboveAltitude condition measures the flight altitude relative to home position, returns true if above critical altitude.</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_ABOVESPEED:
        ui->condParam1->setEnabled(true);
        ui->condParam2->setEnabled(true);
        ui->dsb_condParam1->setEnabled(true);
        ui->dsb_condParam2->setEnabled(true);
        ui->condParam1->setText("Speed (m/s):");
        ui->condParam2->setText("Flag:");
        ui->descriptionConditionLabel->setText(tr("<p>The AboveSpeed measures the movement speed(3D), returns true if above critical speed</p>"
                                                  "<p>Speed in meters / second</p>"
                                                  "<p>Flag: <b>0</b>= groundspeed <b>1</b>= airspeed</p>"));
        break;
    case MapDataDelegate::ENDCONDITION_POINTINGTOWARDSNEXT:
        ui->condParam1->setEnabled(true);
        ui->dsb_condParam1->setEnabled(true);
        ui->condParam1->setText("Degrees:");
        ui->descriptionConditionLabel->setText(tr("<p>The PointingTowardsNext condition measures the horizontal movement vector direction relative "
                                                  "to the next waypoint regardless whether this waypoint will ever be active "
                                                  "(Command could jump to a different waypoint on true).</p><p>This is useful for curve segments where "
                                                  "the craft should stop circling when facing a certain way(usually the next waypoint), "
                                                  "returns true if within a certain angular margin in degrees.</p>"));
        break;
    default:
        ui->descriptionConditionLabel->setText("");
        break;
    }
}

void opmap_edit_waypoint_dialog::editWaypoint(mapcontrol::WayPointItem *waypoint_item)
{
    if (!waypoint_item) {
        return;
    }
    if (!isVisible()) {
        show();
    }
    if (isMinimized()) {
        showNormal();
    }
    if (!isActiveWindow()) {
        activateWindow();
    }
    raise();
    setFocus(Qt::OtherFocusReason);
    mapper->setCurrentIndex(waypoint_item->Number());
}

void opmap_edit_waypoint_dialog::on_pushButtonOK_clicked()
{
    mapper->submit();
    close();
}

void opmap_edit_waypoint_dialog::pushButtonCancel_clicked()
{
    mapper->revert();
    close();
}

void opmap_edit_waypoint_dialog::on_pushButtonPrevious_clicked()
{
    mapper->toPrevious();
}

void opmap_edit_waypoint_dialog::on_pushButtonNext_clicked()
{
    mapper->toNext();
}

void opmap_edit_waypoint_dialog::on_pushButtonApply_clicked()
{
    mapper->submit();
}

void opmap_edit_waypoint_dialog::enableEditWidgets(bool value)
{
    QWidget *w;

    foreach(QWidget * obj, this->findChildren<QWidget *>()) {
        w = qobject_cast<QComboBox *>(obj);
        if (w) {
            w->setEnabled(!value);
        }
        w = qobject_cast<QLineEdit *>(obj);
        if (w) {
            w->setEnabled(!value);
        }
        w = qobject_cast<QDoubleSpinBox *>(obj);
        if (w) {
            w->setEnabled(!value);
        }
        w = qobject_cast<QCheckBox *>(obj);
        if (w && w != ui->checkBoxLocked) {
            w->setEnabled(!value);
        }
        w = qobject_cast<QSpinBox *>(obj);
        if (w) {
            w->setEnabled(!value);
        }
    }
}

void opmap_edit_waypoint_dialog::currentRowChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    mapper->setCurrentIndex(current.row());
}
