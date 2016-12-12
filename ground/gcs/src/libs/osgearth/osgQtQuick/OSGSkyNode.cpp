/**
 ******************************************************************************
 *
 * @file       OSGSkyNode.cpp
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

#include "OSGSkyNode.hpp"

#include "OSGViewport.hpp"

#include <osgViewer/View>

#include <osgEarth/Config>
#include <osgEarth/DateTime>
#include <osgEarth/MapNode>
#include <osgEarthUtil/Sky>

// #include <osgEarthDrivers/sky_silverlining/SilverLiningOptions>

#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { Scene = 1 << 0, Viewport = 1 << 1, DateTime = 1 << 2, Light = 1 << 3 };

struct OSGSkyNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGSkyNode * const self;

    osg::ref_ptr<osgEarth::Util::SkyNode> skyNode;

public:
    OSGNode     *sceneNode;
    OSGViewport *viewport;

    bool        sunLightEnabled;
    QDateTime   dateTime;
    double      minimumAmbientLight;

    Hidden(OSGSkyNode *self) : QObject(self), self(self), sceneNode(NULL), viewport(NULL),
        sunLightEnabled(true), minimumAmbientLight(0.03)
    {
        dateTime = QDateTime::currentDateTime();
    }

    ~Hidden()
    {}

    bool acceptSceneNode(OSGNode *node)
    {
        // qDebug() << "OSGSkyNode::acceptSceneNode" << node;
        if (sceneNode == node) {
            return false;
        }

        if (sceneNode) {
            disconnect(sceneNode);
        }

        sceneNode = node;

        if (sceneNode) {
            connect(sceneNode, &OSGNode::nodeChanged, this, &OSGSkyNode::Hidden::onSceneNodeChanged);
        }

        return true;
    }

    void updateScene()
    {
        if (!sceneNode || !sceneNode->node()) {
            qWarning() << "OSGSkyNode::updateScene - scene node not valid";
            self->setNode(NULL);
            return;
        }
        // qDebug() << "OSGSkyNode::updateScene - scene node" << sceneNode->node();
        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(sceneNode->node());
        if (!mapNode) {
            qWarning() << "OSGSkyNode::updateScene - scene node does not contain a map node";
            self->setNode(NULL);
            return;
        }
        if (!mapNode->getMap()->isGeocentric()) {
            qWarning() << "OSGSkyNode::updateScene - map node is not geocentric";
            self->setNode(NULL);
            return;
        }

        // create sky node
        if (!skyNode.valid()) {
            skyNode = createSimpleSky(mapNode);
            // skyNode = createSilverLiningSky(mapNode);

            // Ocean
            // const osgEarth::Config & externals = mapNode->externalConfig();
            // if (externals.hasChild("ocean")) {
            // s_ocean = osgEarth::Util::OceanNode::create(osgEarth::Util::OceanOptions(externals.child("ocean")), mapNode);
            // if (s_ocean) root->addChild(s_ocean);

            skyNode->addChild(sceneNode->node());
            self->setNode(skyNode);
        } else {
            skyNode->removeChild(0, 1);
            skyNode->addChild(sceneNode->node());
            // self->emitNodeChanged();
        }
    }

    osgEarth::Util::SkyNode *createSimpleSky(osgEarth::MapNode *mapNode)
    {
        return osgEarth::Util::SkyNode::create(mapNode);
    }

/*
    osgEarth::Util::SkyNode *createSilverLiningSky(osgEarth::MapNode *mapNode)
    {
        osgEarth::Drivers::SilverLining::SilverLiningOptions skyOptions;
        skyOptions.user() = "OpenPilot";
        skyOptions.licenseCode() = "1f02040d24000f0e18";
        skyOptions.resourcePath() = "D:/Projects/OpenPilot/3rdparty/SilverLining-SDK-FullSource/Resources";
        skyOptions.drawClouds() = true;
        skyOptions.cloudsMaxAltitude() = 10000.0;

        osgEarth::Util::SkyNode *skyNode = osgEarth::Util::SkyNode::create(skyOptions, mapNode);

        return skyNode;
    }
 */

    void updateViewport()
    {
        // qDebug() << "OSGSkyNode::updateViewport" << skyNode;
        if (!skyNode.valid()) {
            qWarning() << "OSGSkyNode::updateViewport - invalid sky node" << skyNode;
            return;
        }
        // qDebug() << "OSGSkyNode::updateViewport - attaching to" << viewport->asView();
        skyNode->attach(viewport->asView());
    }

    void updateSunLightEnabled()
    {
        if (!skyNode.valid()) {
            qWarning() << "OSGSkyNode::updateSunLightEnabled - invalid sky node";
            return;
        }
        if (!skyNode.valid()) {
            return;
        }
        // skyNode->setLighting(sunLightEnabled ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
    }

    void updateDateTime()
    {
        if (!skyNode.valid()) {
            qWarning() << "OSGSkyNode::updateDateTime - invalid sky node";
            return;
        }
        if (!dateTime.isValid()) {
            qWarning() << "OSGSkyNode::updateDateTime - invalid date/time" << dateTime;
        }

        QDate date   = dateTime.date();
        QTime time   = dateTime.time();
        double hours = time.hour() + (double)time.minute() / 60.0 + (double)time.second() / 3600.0;
        skyNode->setDateTime(osgEarth::DateTime(date.year(), date.month(), date.day(), hours));
    }

    void updateMinimumAmbientLight()
    {
        if (!skyNode.valid()) {
            qWarning() << "OSGSkyNode::updateMinimumAmbientLight - invalid sky node";
            return;
        }
        double d = minimumAmbientLight;
        // skyNode->getSunLight()->setAmbient(osg::Vec4(d, d, d, 1.0f));
        skyNode->setMinimumAmbient(osg::Vec4(d, d, d, 1.0f));
    }

    void attachSkyNode(osgViewer::View *view)
    {
        if (!skyNode.valid()) {
            qWarning() << "OSGSkyNode::attachSkyNode - invalid sky node" << skyNode;
            return;
        }
        skyNode->attach(view);
    }

    void detachSkyNode(osgViewer::View *view)
    {
        // TODO find a way to detach the skyNode (?)
    }

private slots:
    void onSceneNodeChanged(osg::Node *node)
    {
        // qDebug() << "OSGSkyNode::onSceneNodeChanged" << node;
        updateScene();
    }
};

/* class OSGSkyNode */

OSGSkyNode::OSGSkyNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{
    setDirty(DateTime | Light);
}

OSGSkyNode::~OSGSkyNode()
{
    delete h;
}

OSGNode *OSGSkyNode::sceneNode() const
{
    return h->sceneNode;
}

void OSGSkyNode::setSceneNode(OSGNode *node)
{
    if (h->acceptSceneNode(node)) {
        setDirty(Scene);
        emit sceneNodeChanged(node);
    }
}

OSGViewport *OSGSkyNode::viewport() const
{
    return h->viewport;
}

void OSGSkyNode::setViewport(OSGViewport *viewport)
{
    if (h->viewport != viewport) {
        h->viewport = viewport;
        setDirty(Viewport);
        emit viewportChanged(viewport);
    }
}

bool OSGSkyNode::sunLightEnabled() const
{
    return h->sunLightEnabled;
}

void OSGSkyNode::setSunLightEnabled(bool enabled)
{
    if (h->sunLightEnabled != enabled) {
        h->sunLightEnabled = enabled;
        setDirty(Light);
        emit sunLightEnabledChanged(enabled);
    }
}

QDateTime OSGSkyNode::dateTime() const
{
    return h->dateTime;
}

void OSGSkyNode::setDateTime(QDateTime dateTime)
{
    if (h->dateTime != dateTime) {
        h->dateTime = dateTime;
        setDirty(DateTime);
        emit dateTimeChanged(dateTime);
    }
}

double OSGSkyNode::minimumAmbientLight() const
{
    return h->minimumAmbientLight;
}

void OSGSkyNode::setMinimumAmbientLight(double ambient)
{
    if (h->minimumAmbientLight != ambient) {
        h->minimumAmbientLight = ambient;
        setDirty(Light);
        emit minimumAmbientLightChanged(ambient);
    }
}

osg::Node *OSGSkyNode::createNode()
{
    return NULL;
}

void OSGSkyNode::updateNode()
{
    Inherited::updateNode();

    if (isDirty(Scene)) {
        h->updateScene();
    }
    if (isDirty(Viewport)) {
        h->updateViewport();
    }
    if (isDirty(Light)) {
        h->updateSunLightEnabled();
        h->updateMinimumAmbientLight();
    }
    if (isDirty(DateTime)) {
        h->updateDateTime();
    }
}
} // namespace osgQtQuick

#include "OSGSkyNode.moc"
