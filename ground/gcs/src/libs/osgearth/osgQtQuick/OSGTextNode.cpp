/**
 ******************************************************************************
 *
 * @file       OSGTextNode.cpp
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

#include "OSGTextNode.hpp"

#include "utils/utility.h"

#include <osgText/Text>
#include <osg/Geode>
#include <osg/Group>

#include <QFont>
#include <QColor>

namespace osgQtQuick {
enum DirtyFlag { Text = 1 << 0, Color = 1 << 1 };

struct OSGTextNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGTextNode * const self;

    osg::ref_ptr<osgText::Text> text;

public:
    QString textString;
    QColor  color;

    Hidden(OSGTextNode *self) : QObject(self), self(self)
    {}

    osg::Node *createNode()
    {
        osg::ref_ptr<osgText::Font> textFont = createFont(QFont("Times"));

        text = createText(osg::Vec3(-100, 20, 0), "Hello World", 20.0f, textFont.get());
        osg::ref_ptr<osg::Geode> textGeode   = new osg::Geode();
        textGeode->addDrawable(text.get());
    #if 0
        text->setAutoRotateToScreen(true);
    #else
        osg::Camera *camera = createHUDCamera(-100, 100, -100, 100);
        camera->addChild(textGeode.get());
        camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    #endif
        return textGeode;
    }

    void updateText()
    {
        text->setText(textString.toStdString(), osgText::String::ENCODING_UTF8);
    }

    void updateColor()
    {
        osg::Vec4 osgColor(
            color.redF(),
            color.greenF(),
            color.blueF(),
            color.alphaF());

        text->setColor(osgColor);
    }
};

/* class OSGTextNode */

OSGTextNode::OSGTextNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{
    setDirty(Text | Color);
}

OSGTextNode::~OSGTextNode()
{
    delete h;
}

QString OSGTextNode::text() const
{
    return h->textString;
}

void OSGTextNode::setText(const QString &text)
{
    if (h->textString != text) {
        h->textString != text;
        setDirty(Text);
        emit textChanged(text);
    }
}

QColor OSGTextNode::color() const
{
    return h->color;
}

void OSGTextNode::setColor(const QColor &color)
{
    if (h->color != color) {
        h->color != color;
        setDirty(Color);
        emit colorChanged(color);
    }
}

osg::Node *OSGTextNode::createNode()
{
    return h->createNode();
}

void OSGTextNode::updateNode()
{
    Inherited::updateNode();

    if (isDirty(Text)) {
        h->updateText();
    }
    if (isDirty(Color)) {
        h->updateColor();
    }
}
} // namespace osgQtQuick

#include "OSGTextNode.moc"
