/**
 ******************************************************************************
 *
 * @file       OSGTrackballManipulator.cpp
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

#include "OSGTrackballManipulator.hpp"

#include "../OSGNode.hpp"

#include <osgGA/TrackballManipulator>

#include <QDebug>

namespace osgQtQuick {
struct OSGTrackballManipulator::Hidden : public QObject {
    Q_OBJECT

private:
    OSGTrackballManipulator * const self;

public:
    osg::ref_ptr<osgGA::TrackballManipulator> manipulator;

    Hidden(OSGTrackballManipulator *self) : QObject(self), self(self)
    {
        manipulator = new osgGA::TrackballManipulator(
            /*osgGA::StandardManipulator::COMPUTE_HOME_USING_BBOX | osgGA::StandardManipulator::DEFAULT_SETTINGS*/);

        self->setManipulator(manipulator);
    }

    ~Hidden()
    {}
};

/* class OSGTrackballManipulator */

OSGTrackballManipulator::OSGTrackballManipulator(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGTrackballManipulator::~OSGTrackballManipulator()
{
    delete h;
}
} // namespace osgQtQuick

#include "OSGTrackballManipulator.moc"
