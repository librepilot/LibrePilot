/**
 ******************************************************************************
 *
 * @file       OSGSkyNode.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
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

#include <osgViewer/View>

#include <osgEarth/Config>
#include <osgEarth/DateTime>
#include <osgEarth/MapNode>
#include <osgEarthUtil/Sky>

// #include <osgEarthDrivers/sky_silverlining/SilverLiningOptions>

#include <QDebug>

namespace osgQtQuick {
struct OSGSkyNode::Hidden : public QObject {
    Q_OBJECT

public:
    Hidden(OSGSkyNode *parent) : QObject(parent),
        self(parent), sceneData(NULL), sunLightEnabled(true), minimumAmbientLight(0.03)
    {
        dateTime = QDateTime::currentDateTime();
    }

    ~Hidden()
    {}

    bool acceptSceneNode(OSGNode *node)
    {
        qDebug() << "OSGSkyNode::acceptSceneNode" << node;
        if (sceneData == node) {
            return false;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        if (sceneData) {
            acceptNode(sceneData->node());
            connect(sceneData, SIGNAL(nodeChanged(osg::Node *)), this, SLOT(onNodeChanged(osg::Node *)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGSkyNode::acceptNode" << node;

        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        if (!mapNode) {
            qWarning() << "OSGSkyNode::acceptNode - scene data does not contain a map node";
            return false;
        }
        if (!mapNode->getMap()->isGeocentric()) {
            qWarning() << "OSGSkyNode::acceptNode - map node is not geocentric";
            return false;
        }

        // create sky node
        skyNode = createSimpleSky(mapNode);
        // skyNode = createSilverLiningSky(mapNode);

        acceptSunLightEnabled(sunLightEnabled);
        acceptDateTime(dateTime);
        acceptMinimumAmbientLight(minimumAmbientLight);

        // skyNode->setStarsVisible(false);

        // Ocean
        // const osgEarth::Config & externals = mapNode->externalConfig();
        // if (externals.hasChild("ocean")) {
        // s_ocean = osgEarth::Util::OceanNode::create(osgEarth::Util::OceanOptions(externals.child("ocean")), mapNode);
        // if (s_ocean) root->addChild(s_ocean);

        skyNode->addChild(node);

        self->setNode(skyNode);

        return true;
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

    bool acceptSunLightEnabled(bool enabled)
    {
        // qDebug() << "OSGSkyNode::acceptSunLightEnabled" << enabled;

        this->sunLightEnabled = enabled;

        // TODO should be done in a node visitor...
        if (skyNode) {
            // skyNode->setLighting(sunLightEnabled ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
        }

        return true;
    }

    bool acceptDateTime(QDateTime dateTime)
    {
        qDebug() << "OSGSkyNode::acceptDateTime" << dateTime;

        if (!dateTime.isValid()) {
            qWarning() << "OSGSkyNode::acceptDateTime - invalid date/time" << dateTime;
            return false;
        }

        this->dateTime = dateTime;

        // TODO should be done in a node visitor...
        if (skyNode) {
            QDate date   = dateTime.date();
            QTime time   = dateTime.time();
            double hours = time.hour() + (double)time.minute() / 60.0 + (double)time.second() / 3600.0;
            skyNode->setDateTime(osgEarth::DateTime(date.year(), date.month(), date.day(), hours));
        }

        return true;
    }

    bool acceptMinimumAmbientLight(double minimumAmbientLight)
    {
        // qDebug() << "OSGSkyNode::acceptMinimumAmbientLight" << minimumAmbientLight;

        this->minimumAmbientLight = minimumAmbientLight;

        // TODO should be done in a node visitor...
        if (skyNode) {
            double d = minimumAmbientLight;
            // skyNode->getSunLight()->setAmbient(osg::Vec4(d, d, d, 1.0f));
            skyNode->setMinimumAmbient(osg::Vec4(d, d, d, 1.0f));
        }

        return true;
    }

    bool attach(osgViewer::View *view)
    {
        if (!skyNode.valid()) {
            qWarning() << "OSGSkyNode::attach - invalid sky node" << skyNode;
            return false;
        }
        skyNode->attach(view, 0);
        return true;
    }

    bool detach(osgViewer::View *view)
    {
        qWarning() << "OSGSkyNode::detach - not implemented";
        return true;
    }

    OSGSkyNode *const self;

    OSGNode   *sceneData;
    osg::ref_ptr<osgEarth::Util::SkyNode> skyNode;

    bool      sunLightEnabled;
    QDateTime dateTime;
    double    minimumAmbientLight;

private slots:

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "OSGSkyNode::onNodeChanged" << node;
        acceptNode(node);
    }
};

OSGSkyNode::OSGSkyNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGSkyNode::OSGSkyNode";
}

OSGSkyNode::~OSGSkyNode()
{
    qDebug() << "OSGSkyNode::~OSGSkyNode";
}

OSGNode *OSGSkyNode::sceneData()
{
    return h->sceneData;
}

void OSGSkyNode::setSceneData(OSGNode *node)
{
    if (h->acceptSceneNode(node)) {
        emit sceneDataChanged(node);
    }
}

bool OSGSkyNode::sunLightEnabled()
{
    return h->sunLightEnabled;
}

void OSGSkyNode::setSunLightEnabled(bool arg)
{
    if (h->acceptSunLightEnabled(arg)) {
        emit sunLightEnabledChanged(sunLightEnabled());
    }
}

QDateTime OSGSkyNode::dateTime()
{
    return h->dateTime;
}

void OSGSkyNode::setDateTime(QDateTime arg)
{
    if (h->acceptDateTime(arg)) {
        emit dateTimeChanged(dateTime());
    }
}

double OSGSkyNode::minimumAmbientLight()
{
    return h->minimumAmbientLight;
}

void OSGSkyNode::setMinimumAmbientLight(double arg)
{
    if (h->acceptMinimumAmbientLight(arg)) {
        emit minimumAmbientLightChanged(minimumAmbientLight());
    }
}

bool OSGSkyNode::attach(osgViewer::View *view)
{
    return h->attach(view);
}

bool OSGSkyNode::detach(osgViewer::View *view)
{
    return h->detach(view);
}
} // namespace osgQtQuick

#include "OSGSkyNode.moc"
