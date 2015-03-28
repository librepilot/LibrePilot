#ifndef OSGQTQUICK_UTILITY_HPP
#define OSGQTQUICK_UTILITY_HPP

#include "Export.hpp"

#include <osg/NodeVisitor>
#include <osg/GraphicsContext>

#include <QtGlobal>
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

OSGQTQUICK_EXPORT void insertCallbacks(osg::Node *node);

OSGQTQUICK_EXPORT osg::Camera *createHUDCamera(double left, double right, double bottom, double top);

OSGQTQUICK_EXPORT osgText::Font *createFont(const std::string &name);
OSGQTQUICK_EXPORT osgText::Font *createFont(const QFont &font);

OSGQTQUICK_EXPORT osgText::Text *createText(const osg::Vec3 &pos,
                                            const std::string &content,
                                            float size,
                                            osgText::Font *font = 0);

OSGQTQUICK_EXPORT QSurfaceFormat traitsToFormat(const osg::GraphicsContext::Traits *traits);
OSGQTQUICK_EXPORT void formatToTraits(const QSurfaceFormat & format, osg::GraphicsContext::Traits *traits);
OSGQTQUICK_EXPORT void formatInfo(const QSurfaceFormat & format);
OSGQTQUICK_EXPORT void traitsInfo(const osg::GraphicsContext::Traits *traits);
OSGQTQUICK_EXPORT void capabilitiesInfo(const osgEarth::Capabilities & caps);
OSGQTQUICK_EXPORT QString formatProfileName(QSurfaceFormat::OpenGLContextProfile profile);
OSGQTQUICK_EXPORT QString formatSwapBehaviorName(QSurfaceFormat::SwapBehavior swapBehavior);

OSGQTQUICK_EXPORT void registerTypes(const char *uri);
} // namespace osgQtQuick

#endif // OSGQTQUICK_UTILITY_HPP
