/**
 ******************************************************************************
 *
 * @file       OSGNodeTrackerManipulator.cpp
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

#include "OSGNodeTrackerManipulator.hpp"

#include "../OSGNode.hpp"

#include <osgGA/NodeTrackerManipulator>

#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { TrackNode = 1 << 10, TrackerMode = 1 << 11 };

struct OSGNodeTrackerManipulator::Hidden : public QObject {
    Q_OBJECT

private:
    OSGNodeTrackerManipulator * const self;

public:
    osg::ref_ptr<osgGA::NodeTrackerManipulator> manipulator;

    OSGNode *trackNode;
    TrackerMode::Enum trackerMode;

public:
    Hidden(OSGNodeTrackerManipulator *self) : QObject(self), self(self),
        trackNode(NULL), trackerMode(TrackerMode::NodeCenterAndAzim)
    {
        manipulator = new osgGA::NodeTrackerManipulator(
            /*osgGA::StandardManipulator::COMPUTE_HOME_USING_BBOX | osgGA::StandardManipulator::DEFAULT_SETTINGS*/);
        manipulator->setTrackerMode(toOsg(trackerMode));
        manipulator->setVerticalAxisFixed(false);

        self->setManipulator(manipulator);
    }

    ~Hidden()
    {}

    bool acceptTrackNode(OSGNode *node)
    {
        // qDebug() << "OSGNodeTrackerManipulator::acceptTrackNode" << node;
        if (trackNode == node) {
            return false;
        }

        if (trackNode) {
            disconnect(trackNode);
        }

        trackNode = node;

        if (trackNode) {
            connect(trackNode, &OSGNode::nodeChanged, this, &OSGNodeTrackerManipulator::Hidden::onTrackNodeChanged);
        }

        return true;
    }

    void updateTrackNode()
    {
        if (!trackNode) {
            qWarning() << "OSGNodeTrackerManipulator::updateTrackNode - no track node";
            return;
        }
        // qDebug() << "OSGNodeTrackerManipulator::updateTrackNode" << trackNode->node();
        manipulator->setTrackNode(trackNode->node());
    }

    void updateTrackerMode()
    {
        // qDebug() << "OSGNodeTrackerManipulator::updateTrackerMode" << mode;
        manipulator->setTrackerMode(toOsg(trackerMode));
    }

    osgGA::NodeTrackerManipulator::TrackerMode toOsg(TrackerMode::Enum mode)
    {
        switch (mode) {
        case TrackerMode::NodeCenter:
            return osgGA::NodeTrackerManipulator::NODE_CENTER;

        case TrackerMode::NodeCenterAndAzim:
            return osgGA::NodeTrackerManipulator::NODE_CENTER_AND_AZIM;

        case TrackerMode::NodeCenterAndRotation:
            return osgGA::NodeTrackerManipulator::NODE_CENTER_AND_ROTATION;
        }
        return osgGA::NodeTrackerManipulator::NODE_CENTER_AND_ROTATION;
    }

private slots:
    void onTrackNodeChanged(osg::Node *node)
    {
        // qDebug() << "OSGNodeTrackerManipulator::onTrackNodeChanged" << node;
        qWarning() << "OSGNodeTrackerManipulator::onTrackNodeChanged - needs to be implemented";
    }
};

/* class OSGNodeTrackerManipulator */

OSGNodeTrackerManipulator::OSGNodeTrackerManipulator(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGNodeTrackerManipulator::~OSGNodeTrackerManipulator()
{
    delete h;
}

OSGNode *OSGNodeTrackerManipulator::trackNode() const
{
    return h->trackNode;
}

void OSGNodeTrackerManipulator::setTrackNode(OSGNode *node)
{
    if (h->acceptTrackNode(node)) {
        setDirty(TrackNode);
        emit trackNodeChanged(node);
    }
}

TrackerMode::Enum OSGNodeTrackerManipulator::trackerMode() const
{
    return h->trackerMode;
}

void OSGNodeTrackerManipulator::setTrackerMode(TrackerMode::Enum mode)
{
    if (h->trackerMode != mode) {
        h->trackerMode = mode;
        setDirty(TrackerMode);
        emit trackerModeChanged(trackerMode());
    }
}

void OSGNodeTrackerManipulator::update()
{
    Inherited::update();

    if (isDirty(TrackNode)) {
        h->updateTrackNode();
    }
    if (isDirty(TrackerMode)) {
        h->updateTrackerMode();
    }
}
} // namespace osgQtQuick

#include "OSGNodeTrackerManipulator.moc"
