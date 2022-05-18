/**
 ******************************************************************************
 *
 * @file       utility.h
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

#ifndef OSGEARTH_UTILITY_H
#define OSGEARTH_UTILITY_H

#include "osgearth_global.h"

#include <osg/NodeVisitor>
#include <osg/GraphicsContext>

#include <QtGlobal>
#include <QOpenGLContext>
#include <QSurfaceFormat>

#include <string>
#include <map>

namespace osg {
class Vec3f;
typedef Vec3f Vec3;
class Node;
class Camera;
} // namespace osg

namespace osgViewer {
class Viewer;
class CompositeViewer;
} // namespace osgViewer

namespace osgText {
class Text;
class Font;
} // namespace osgText

#ifdef USE_OSGEARTH
namespace osgEarth {
class Capabilities;
class GeoPoint;
class MapNode;
class SpatialReference;
} // namespace osgEarth
#endif

QT_BEGIN_NAMESPACE
class QFont;
class QKeyEvent;
QT_END_NAMESPACE

namespace osgQtQuick {
class QtKeyboardMap {
public:
    QtKeyboardMap();
    ~QtKeyboardMap();

    int remapKey(QKeyEvent *event);

private:
    typedef std::map<unsigned int, int> KeyMap;
    KeyMap mKeyMap;
};

template<class T>
class FindTopMostNodeOfTypeVisitor : public osg::NodeVisitor {
public:
    FindTopMostNodeOfTypeVisitor() :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _foundNode(0)
    {}

    void apply(osg::Node & node)
    {
        T *result = dynamic_cast<T *>(&node);

        if (result) {
            _foundNode = result;
        } else {
            traverse(node);
        }
    }

    T *_foundNode;
};

template<class T>
T *findTopMostNodeOfType(osg::Node *node)
{
    if (!node) {
        return 0;
    }

    FindTopMostNodeOfTypeVisitor<T> fnotv;
    node->accept(fnotv);

    return fnotv._foundNode;
}

void insertCallbacks(osg::Node *node);

osg::Camera *createHUDCamera(double left, double right, double bottom, double top);

osgText::Font *createFont(const std::string &name);
osgText::Font *createFont(const QFont &font);

osgText::Text *createText(const osg::Vec3 &pos,
                          const std::string &content,
                          float size,
                          osgText::Font *font = 0);

QSurfaceFormat traitsToFormat(const osg::GraphicsContext::Traits *traits);
void formatToTraits(const QSurfaceFormat & format, osg::GraphicsContext::Traits *traits);

void formatInfo(const QSurfaceFormat & format);
void traitsInfo(const osg::GraphicsContext::Traits & traits);
void openGLContextInfo(QOpenGLContext *context, const char *at);

QString formatProfileName(QSurfaceFormat::OpenGLContextProfile profile);
QString formatSwapBehaviorName(QSurfaceFormat::SwapBehavior swapBehavior);

QString getUsageString(osgViewer::Viewer *viewer);
QString getUsageString(osgViewer::CompositeViewer *viewer);

#ifdef USE_OSGEARTH
osgEarth::GeoPoint createGeoPoint(const QVector3D &position, osgEarth::MapNode *mapNode);
bool clampGeoPoint(osgEarth::GeoPoint &geoPoint, float offset, osgEarth::MapNode *mapNode);
void capabilitiesInfo(const osgEarth::Capabilities & caps);
#endif

void registerTypes();
} // namespace osgQtQuick

#endif // OSGEARTH_UTILITY_H
