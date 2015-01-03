#ifndef OSGQTQUICK_UTILITY_HPP
#define OSGQTQUICK_UTILITY_HPP

#include "Export.hpp"

#include <osg/NodeVisitor>

#include <QtGlobal>

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

OSGQTQUICK_EXPORT osg::Camera *createHUDCamera(double left, double right,
                                               double bottom, double top);

OSGQTQUICK_EXPORT osgText::Font *createFont(const std::string &name);
OSGQTQUICK_EXPORT osgText::Font *createFont(const QFont &font);

OSGQTQUICK_EXPORT osgText::Text *createText(const osg::Vec3 &pos,
                                            const std::string &content,
                                            float size,
                                            osgText::Font *font = 0);

OSGQTQUICK_EXPORT void registerTypes(const char *uri);
} // namespace osgQtQuick

#endif // OSGQTQUICK_UTILITY_HPP
