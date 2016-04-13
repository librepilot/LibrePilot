/**
 ******************************************************************************
 *
 * @file       OSGEarthManipulator.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @addtogroup
 * @{
 * @addtogroup
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

#include "OSGEarthManipulator.hpp"

#include "../OSGNode.hpp"

#include <osgEarthUtil/EarthManipulator>

#include <QDebug>

namespace osgQtQuick {
struct OSGEarthManipulator::Hidden : public QObject {
    Q_OBJECT

private:
    OSGEarthManipulator * const self;

public:
    osg::ref_ptr<osgEarth::Util::EarthManipulator> manipulator;

    Hidden(OSGEarthManipulator *self) : QObject(self), self(self)
    {
        manipulator = new osgEarth::Util::EarthManipulator(
            /*osgGA::StandardManipulator::COMPUTE_HOME_USING_BBOX | osgGA::StandardManipulator::DEFAULT_SETTINGS*/);
        manipulator->getSettings()->setThrowingEnabled(true);
        self->setManipulator(manipulator);
    }

    ~Hidden()
    {}
};

/* class OSGEarthManipulator */

OSGEarthManipulator::OSGEarthManipulator(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGEarthManipulator::~OSGEarthManipulator()
{
    delete h;
}
} // namespace osgQtQuick

#include "OSGEarthManipulator.moc"
