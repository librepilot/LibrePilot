#include "Utility.hpp"

#include <osg/Camera>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgText/Font>
#include <osgText/Text>
#include <osgText/String>
#include <osgQt/QFontImplementation>

// osgQtQuick qml types
#include "osgQtQuick/OSGNode.hpp"
#include "osgQtQuick/OSGGroup.hpp"
#include "osgQtQuick/OSGNodeFile.hpp"
#include "osgQtQuick/OSGTextNode.hpp"
#include "osgQtQuick/OSGEarthNode.hpp"
#include "osgQtQuick/OSGViewport.hpp"

#include <QFont>

namespace osgQtQuick {

osg::Camera * createHUDCamera(double left, double right, double bottom, double top)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera();
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setAllowEventFocus(false);
    camera->setProjectionMatrix(
                osg::Matrix::ortho2D(left, right, bottom, top));
    return camera.release();
}

osgText::Font * createFont(const std::string &name)
{
    QFont font;
    if (!font.fromString(
                QString::fromStdString(
                    osgDB::getNameLessExtension(name)))) {
        return 0;
    }

    return new osgText::Font(new osgQt::QFontImplementation(font));
}

osgText::Font * createFont(const QFont &font)
{
    return new osgText::Font(new osgQt::QFontImplementation(font));
}

osgText::Text * createText(const osg::Vec3 &pos, const std::string &content, float size, osgText::Font *font)
{
    osg::ref_ptr<osgText::Text> text = new osgText::Text();
    if (font) text->setFont(font);
    text->setCharacterSize(size);
    text->setAxisAlignment(osgText::TextBase::XY_PLANE);
    text->setPosition(pos);
    text->setText(content, osgText::String::ENCODING_UTF8);
    return text.release();
}

void registerTypes(const char *uri)
{
    //Q_ASSERT(uri == QLatin1String("osgQtQuick"));
    int maj = 1, min = 0;
    // @uri osgQtQuick
    qmlRegisterType<osgQtQuick::OSGNode>(uri, maj, min, "OSGNode");
    qmlRegisterType<osgQtQuick::OSGGroup>(uri, maj, min, "OSGGroup");
    qmlRegisterType<osgQtQuick::OSGNodeFile>(uri, maj, min, "OSGNodeFile");
    qmlRegisterType<osgQtQuick::OSGTextNode>(uri, maj, min, "OSGTextNode");
    qmlRegisterType<osgQtQuick::OSGViewport>(uri, maj, min, "OSGViewport");

}

} // namespace osgQtQuick
