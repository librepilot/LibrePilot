#ifndef OSGQTQUICK_UTILITY_HPP
#define OSGQTQUICK_UTILITY_HPP

#include "osgQtQuick/Export.hpp"

#include <QtGlobal>

#include <string>

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
QT_END_NAMESPACE

namespace osgQtQuick {

OSGQTQUICK_EXPORT template<class T> T* findTopMostNodeOfType(osg::Node* node);

OSGQTQUICK_EXPORT osg::Camera* createHUDCamera(double left, double right,
                              double bottom, double top);

OSGQTQUICK_EXPORT osgText::Font* createFont(const std::string &name);
OSGQTQUICK_EXPORT osgText::Font* createFont(const QFont &font);

OSGQTQUICK_EXPORT osgText::Text* createText(const osg::Vec3 &pos,
                          const std::string &content,
                          float size,
                          osgText::Font *font = 0);

OSGQTQUICK_EXPORT void registerTypes(const char *uri);

} // namespace osgQtQuick

#endif // OSGQTQUICK_UTILITY_HPP
