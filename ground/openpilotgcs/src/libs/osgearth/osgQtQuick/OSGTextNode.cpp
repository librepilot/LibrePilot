#include "OSGTextNode.hpp"

#include "Utility.hpp"

#include <osgText/Text>
#include <osg/Geode>
#include <osg/Group>

#include <QFont>
#include <QColor>

namespace osgQtQuick {
struct OSGTextNode::Hidden {
public:
    osg::ref_ptr<osgText::Text> text;
};

OSGTextNode::OSGTextNode(QObject *parent) :
    osgQtQuick::OSGNode(parent),
    h(new Hidden)
{
    osg::ref_ptr<osgText::Font> textFont = createFont(QFont("Times"));

    h->text = createText(osg::Vec3(-100, 20, 0),
                         "The osgQtQuick :-)\n"
                         "И даже по русски!",
                         20.0f,
                         textFont.get());
    osg::ref_ptr<osg::Geode> textGeode = new osg::Geode();
    h->text->setColor(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    textGeode->addDrawable(h->text.get());
#if 0
    h->text->setAutoRotateToScreen(true);
    setNode(textGeode.get());
#else
    osg::Camera *camera = createHUDCamera(-100, 100, -100, 100);
    camera->addChild(textGeode.get());
    camera->getOrCreateStateSet()->setMode(
        GL_LIGHTING, osg::StateAttribute::OFF);
    setNode(camera);
#endif
}

OSGTextNode::~OSGTextNode()
{
    delete h;
}

QString OSGTextNode::text() const
{
    return QString::fromUtf8(
        h->text->getText().createUTF8EncodedString().data());
}

void OSGTextNode::setText(const QString &text)
{
    std::string oldText = h->text->getText().createUTF8EncodedString();

    if (text.toStdString() != oldText) {
        h->text->setText(text.toStdString(), osgText::String::ENCODING_UTF8);
        emit textChanged(text);
    }
}

QColor OSGTextNode::color() const
{
    const osg::Vec4 osgColor = h->text->getColor();

    return QColor::fromRgbF(
        osgColor.r(),
        osgColor.g(),
        osgColor.b(),
        osgColor.a());
}

void OSGTextNode::setColor(const QColor &color)
{
    osg::Vec4 osgColor(
        color.redF(),
        color.greenF(),
        color.blueF(),
        color.alphaF());

    if (h->text->getColor() != osgColor) {
        h->text->setColor(osgColor);
        emit colorChanged(color);
    }
}
} // namespace osgQtQuick
