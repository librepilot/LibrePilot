/**
 ******************************************************************************
 *
 * @file       OSGTextNode.cpp
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

#include "OSGTextNode.hpp"

#include "../utility.h"

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

OSGTextNode::OSGTextNode(QObject *node) : OSGNode(node), h(new Hidden)
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
