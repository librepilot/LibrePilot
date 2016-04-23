/**
 ******************************************************************************
 *
 * @file       OSGNodeTrackerManipulator.hpp
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

#ifndef _H_OSGQTQUICK_OSGNODETRACKERMANIPULATOR_H_
#define _H_OSGQTQUICK_OSGNODETRACKERMANIPULATOR_H_

#include "../Export.hpp"
#include "OSGCameraManipulator.hpp"

#include <QObject>

namespace osgQtQuick {
class TrackerMode : public QObject {
    Q_OBJECT
public:
    enum Enum { NodeCenter, NodeCenterAndAzim, NodeCenterAndRotation };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5
};

class OSGQTQUICK_EXPORT OSGNodeTrackerManipulator : public OSGCameraManipulator {
    Q_OBJECT Q_PROPERTY(osgQtQuick::OSGNode *trackNode READ trackNode WRITE setTrackNode NOTIFY trackNodeChanged)
    Q_PROPERTY(osgQtQuick::TrackerMode::Enum trackerMode READ trackerMode WRITE setTrackerMode NOTIFY trackerModeChanged)

    typedef OSGCameraManipulator Inherited;

public:
    explicit OSGNodeTrackerManipulator(QObject *parent = 0);
    virtual ~OSGNodeTrackerManipulator();

    OSGNode *trackNode() const;
    void setTrackNode(OSGNode *node);

    TrackerMode::Enum trackerMode() const;
    void setTrackerMode(TrackerMode::Enum);

signals:
    void trackNodeChanged(OSGNode *node);
    void trackerModeChanged(TrackerMode::Enum);

private:
    struct Hidden;
    Hidden *const h;

    virtual void update();
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGNODETRACKERMANIPULATOR_H_
