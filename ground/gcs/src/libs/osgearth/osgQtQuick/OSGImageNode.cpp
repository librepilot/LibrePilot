/**
 ******************************************************************************
 *
 * @file       OSGImageNode.cpp
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

#include "OSGImageNode.hpp"

#include "utils/imagesource.hpp"

#ifdef USE_GSTREAMER
#include "utils/gstreamer/gstimagesource.hpp"
#endif

#include <osg/Image>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Texture2D>

#include <QUrl>
#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { ImageFile = 1 << 0 };

struct OSGImageNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGImageNode * const self;

    osg::ref_ptr<osg::Texture2D> texture;

    ImageSource *imageSource;

public:
    QUrl imageUrl;

    Hidden(OSGImageNode *self) : QObject(self), self(self), imageSource(NULL), imageUrl()
    {
        if (imageSource) {
            delete imageSource;
        }
    }

    osg::Node *createNode()
    {
        osg::Geode *geode = new osg::Geode;

        return geode;
    }

    osg::Image *loadImage()
    {
        if (!imageSource) {
            if (imageUrl.scheme() == "gst") {
#ifdef USE_GSTREAMER
                imageSource = new GstImageSource();
#else
                qWarning() << "gstreamer image source is not supported";
#endif
            } else {
                imageSource = new ImageSource();
            }
        }
        return imageSource ? imageSource->createImage(imageUrl) : 0;
    }

    void updateImageFile()
    {
        update();
    }

    void update()
    {
        osg::Image *image = loadImage();

        if (!image) {
            return;
        }

        // qDebug() << "OSGImageNode::update" << image;
        osg::Node *geode = createGeodeForImage(image);

        self->setNode(geode);
    }

    osg::Geode *createGeodeForImage(osg::Image *image)
    {
        // vertex
        osg::Vec3Array *coords = new osg::Vec3Array(4);

        (*coords)[0].set(0, 1, 0);
        (*coords)[1].set(0, 0, 0);
        (*coords)[2].set(1, 0, 0);
        (*coords)[3].set(1, 1, 0);

        // texture coords
        osg::Vec2Array *texcoords = new osg::Vec2Array(4);
        float x_b = 0.0f;
        float x_t = 1.0f;
        float y_b = (image->getOrigin() == osg::Image::BOTTOM_LEFT) ? 0.0f : 1.0f;
        float y_t = (image->getOrigin() == osg::Image::BOTTOM_LEFT) ? 1.0f : 0.0f;
        (*texcoords)[0].set(x_b, y_t);
        (*texcoords)[1].set(x_b, y_b);
        (*texcoords)[2].set(x_t, y_b);
        (*texcoords)[3].set(x_t, y_t);

        // color
        osg::Vec4Array *color = new osg::Vec4Array(1);
        (*color)[0].set(1.0f, 1.0f, 1.0f, 1.0f);

        // setup the geometry
        osg::Geometry *geom = new osg::Geometry;
        geom->setVertexArray(coords);
        geom->setTexCoordArray(0, texcoords);
        geom->setColorArray(color, osg::Array::BIND_OVERALL);
        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

        // set up the texture.
        osg::Texture2D *texture = new osg::Texture2D;
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setResizeNonPowerOfTwoHint(false);
        texture->setImage(image);

        // set up the state.
        osg::StateSet *state = new osg::StateSet;
        state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        state->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        state->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
        geom->setStateSet(state);

        // set up the geode.
        osg::Geode *geode = new osg::Geode;
        geode->addDrawable(geom);

        return geode;
    }
};

/* class OSGImageNode */

OSGImageNode::OSGImageNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGImageNode::~OSGImageNode()
{
    delete h;
}

const QUrl OSGImageNode::imageUrl() const
{
    return h->imageUrl;
}

void OSGImageNode::setImageUrl(QUrl &url)
{
    if (h->imageUrl != url) {
        h->imageUrl = url;
        setDirty(ImageFile);
        emit imageUrlChanged(url);
    }
}

osg::Node *OSGImageNode::createNode()
{
    return h->createNode();
}

void OSGImageNode::updateNode()
{
    Inherited::updateNode();

    if (isDirty(ImageFile)) {
        h->updateImageFile();
    }
}
} // namespace osgQtQuick

#include "OSGImageNode.moc"
