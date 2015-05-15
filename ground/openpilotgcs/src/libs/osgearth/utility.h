#ifndef OSGEARTH_UTILITY_H
#define OSGEARTH_UTILITY_H

#include "osgearth_global.h"

#include <osg/NodeVisitor>
#include <osg/GraphicsContext>

#include <osgEarth/GeoData>

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

namespace osgText {
class Text;
class Font;
} // namespace osgText

namespace osgEarth {
class Capabilities;
class MapNode;
} // namespace osgEarth

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

osgEarth::GeoPoint toGeoPoint(const QVector3D &position);
bool clampGeoPoint(osgEarth::GeoPoint &geoPoint, float offset, osgEarth::MapNode *mapNode);

QSurfaceFormat traitsToFormat(const osg::GraphicsContext::Traits *traits);
void formatToTraits(const QSurfaceFormat & format, osg::GraphicsContext::Traits *traits);

void formatInfo(const QSurfaceFormat & format);
void traitsInfo(const osg::GraphicsContext::Traits & traits);
void capabilitiesInfo(const osgEarth::Capabilities & caps);
void openGLContextInfo(QOpenGLContext *context, const char *at);

QString formatProfileName(QSurfaceFormat::OpenGLContextProfile profile);
QString formatSwapBehaviorName(QSurfaceFormat::SwapBehavior swapBehavior);

void registerTypes(const char *uri);
} // namespace osgQtQuick

#endif // OSGEARTH_UTILITY_H
