/**
 ******************************************************************************
 *
 * @file       OSGCameraManipulator.cpp
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

#include "OSGCameraManipulator.hpp"

#include "../DirtySupport.hpp"
#include "../OSGNode.hpp"

#include <osgGA/CameraManipulator>

#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { Scene = 1 << 0 };

struct OSGCameraManipulator::Hidden : public QObject, public DirtySupport {
    Q_OBJECT

    friend class OSGCameraManipulator;

private:
    OSGCameraManipulator *const self;

public:
    osg::ref_ptr<osgGA::CameraManipulator> manipulator;

    OSGNode *sceneNode;

public:
    Hidden(OSGCameraManipulator *self) : QObject(self), self(self), sceneNode(NULL)
    {}

    ~Hidden()
    {}

    osg::Node *nodeToUpdate() const
    {
        return manipulator ? manipulator->getNode() : NULL;
    }

    void update()
    {
        return self->update();
    }

    bool acceptSceneNode(OSGNode *node)
    {
        // qDebug() << "OSGCameraManipulator::acceptSceneNode" << node;
        if (sceneNode == node) {
            return true;
        }

        if (sceneNode) {
            disconnect(sceneNode);
        }

        sceneNode = node;

        if (sceneNode) {
            connect(sceneNode, &OSGNode::nodeChanged, this, &OSGCameraManipulator::Hidden::onSceneNodeChanged);
        }

        return true;
    }

    void updateSceneNode()
    {
        if (!sceneNode) {
            qWarning() << "OSGCameraManipulator::updateSceneNode - no scene node";
            return;
        }
        // qDebug() << "OSGCameraManipulator::updateSceneNode" << sceneNode;
        manipulator->setNode(sceneNode->node());
    }

private slots:
    void onSceneNodeChanged(osg::Node *node)
    {
        // qDebug() << "OSGCameraManipulator::onSceneNodeChanged" << node;
        updateSceneNode();
    }
};

/* class OSGCameraManipulator */

OSGCameraManipulator::OSGCameraManipulator(QObject *parent) : QObject(parent), h(new Hidden(this))
{}

OSGCameraManipulator::~OSGCameraManipulator()
{
    delete h;
}

OSGNode *OSGCameraManipulator::sceneNode() const
{
    return h->sceneNode;
}

void OSGCameraManipulator::setSceneNode(OSGNode *node)
{
    if (h->acceptSceneNode(node)) {
        setDirty(Scene);
        emit sceneNodeChanged(node);
    }
}

bool OSGCameraManipulator::isDirty(int mask) const
{
    return h->isDirty(mask);
}

void OSGCameraManipulator::setDirty(int mask)
{
    h->setDirty(mask);
}

void OSGCameraManipulator::clearDirty()
{
    h->clearDirty();
}

void OSGCameraManipulator::classBegin()
{
    // qDebug() << "OSGCameraManipulator::classBegin" << this;
}

void OSGCameraManipulator::componentComplete()
{
    // qDebug() << "OSGCameraManipulator::componentComplete" << this;
    update();
    clearDirty();
}

osgGA::CameraManipulator *OSGCameraManipulator::manipulator() const
{
    return h->manipulator;
}

void OSGCameraManipulator::setManipulator(osgGA::CameraManipulator *manipulator)
{
    h->manipulator = manipulator;
}

osgGA::CameraManipulator *OSGCameraManipulator::asCameraManipulator() const
{
    return h->manipulator;
}

void OSGCameraManipulator::update()
{
    if (isDirty(Scene)) {
        h->updateSceneNode();
    }
}
} // namespace osgQtQuick

#include "OSGCameraManipulator.moc"
