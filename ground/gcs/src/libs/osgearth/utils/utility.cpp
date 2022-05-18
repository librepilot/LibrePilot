/**
 ******************************************************************************
 *
 * @file       utility.cpp
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

#include "utils/utility.h"

// osgQtQuick qml types
#include "osgQtQuick/OSGNode.hpp"
#include "osgQtQuick/OSGGroup.hpp"
#include "osgQtQuick/OSGFileNode.hpp"
#include "osgQtQuick/OSGTransformNode.hpp"
#include "osgQtQuick/OSGShapeNode.hpp"
#include "osgQtQuick/OSGImageNode.hpp"
#include "osgQtQuick/OSGTextNode.hpp"
#include "osgQtQuick/OSGBillboardNode.hpp"
#include "osgQtQuick/OSGCamera.hpp"
#include "osgQtQuick/OSGViewport.hpp"

#include "osgQtQuick/ga/OSGCameraManipulator.hpp"
#include "osgQtQuick/ga/OSGNodeTrackerManipulator.hpp"
#include "osgQtQuick/ga/OSGTrackballManipulator.hpp"

#include <osg/NodeCallback>
#include <osg/Camera>
#include <osg/MatrixTransform>
#include <osg/io_utils>
#include <osg/ApplicationUsage>
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
#include <osgGA/GUIEventAdapter>
#include <osgText/Font>
#include <osgText/Text>
#include <osgText/String>

#ifdef USE_OSG_QT
#include <osgQt/QFontImplementation>
#endif // USE_OSG_QT

#ifdef USE_OSGEARTH
#include "osgQtQuick/OSGSkyNode.hpp"
#include "osgQtQuick/OSGGeoTransformNode.hpp"

#include "osgQtQuick/ga/OSGEarthManipulator.hpp"
#include "osgQtQuick/ga/OSGGeoTransformManipulator.hpp"

#include <osgEarth/Version>
#include <osgEarth/Capabilities>
#include <osgEarth/MapNode>
#include <osgEarth/SpatialReference>
#include <osgEarth/Terrain>
#include <osgEarth/ElevationQuery>
#endif // USE_OSGEARTH

#include <QFont>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QThread>

namespace osgQtQuick {
class CullCallback : public osg::NodeCallback {
public:
    CullCallback()
    {}

    virtual ~CullCallback()
    {}

public:
    virtual void operator()(osg::Node *node, osg::NodeVisitor *nv)
    {
        osgUtil::CullVisitor *cv = 0; // osgEarth::Culling::asCullVisitor(nv);

        if (cv) {
            OSG_DEBUG << "****** Node:" << node << " " << node->getName() << std::endl;
            OSG_DEBUG << "projection matrix: " << *(cv->getProjectionMatrix()) << std::endl;
            OSG_DEBUG << "model view matrix: " << *(cv->getModelViewMatrix()) << std::endl;
        }
        osg::MatrixTransform *mt = dynamic_cast<osg::MatrixTransform *>(node);
        if (mt) {
            OSG_DEBUG << "matrix: " << mt->getMatrix() << std::endl;
        }
        traverse(node, nv);
    }
};

class InsertCallbacksVisitor : public osg::NodeVisitor {
public:
    InsertCallbacksVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {}

    virtual void apply(osg::Node & node)
    {
        // node.setUpdateCallback(new UpdateCallback());
        node.setCullCallback(new CullCallback());
        traverse(node);
    }

    virtual void apply(osg::Geode & geode)
    {
// geode.setUpdateCallback(new UpdateCallback());
////note, it makes no sense to attach a cull callback to the node
////at there are no nodes to traverse below the geode, only
////drawables, and as such the Cull node callbacks is ignored.
////If you wish to control the culling of drawables
////then use a drawable cullback...
// for(unsigned int i=0;i<geode.getNumDrawables();++i)
// {
// geode.getDrawable(i)->setUpdateCallback(new DrawableUpdateCallback());
// geode.getDrawable(i)->setCullCallback(new DrawableCullCallback());
// geode.getDrawable(i)->setDrawCallback(new DrawableDrawCallback());
// }
    }

    virtual void apply(osg::Transform & node)
    {
        apply((osg::Node &)node);
    }
};

void insertCallbacks(osg::Node *node)
{
    InsertCallbacksVisitor icv;

    node->accept(icv);
}

osg::Camera *createHUDCamera(double left, double right, double bottom, double top)
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

osgText::Font *createFont(const std::string &name)
{
    QFont font;

    if (!font.fromString(QString::fromStdString(osgDB::getNameLessExtension(name)))) {
        return 0;
    }

    return createFont(font);
}

osgText::Font *createFont(const QFont &font)
{
#ifdef USE_OSG_QT
    return new osgText::Font(new osgQt::QFontImplementation(font));

#else
    qWarning() << "Cannot create osgText::Font from QFont (osgQt is not available)";
    return osgText::Font::getDefaultFont();

#endif // USE_OSG_QT
}

osgText::Text *createText(const osg::Vec3 &pos, const std::string &content, float size, osgText::Font *font)
{
    osg::ref_ptr<osgText::Text> text = new osgText::Text();

    if (font) {
        text->setFont(font);
    }
    text->setCharacterSize(size);
    text->setAxisAlignment(osgText::TextBase::XY_PLANE);
    text->setPosition(pos);
    text->setText(content, osgText::String::ENCODING_UTF8);
    return text.release();
}

// Copied from "GraphicsWindowQt.cpp" (osg module osgQt)
QtKeyboardMap::QtKeyboardMap()
{
    mKeyMap[Qt::Key_Escape]    = osgGA::GUIEventAdapter::KEY_Escape;
    mKeyMap[Qt::Key_Delete]    = osgGA::GUIEventAdapter::KEY_Delete;
    mKeyMap[Qt::Key_Home]      = osgGA::GUIEventAdapter::KEY_Home;
    mKeyMap[Qt::Key_Enter]     = osgGA::GUIEventAdapter::KEY_KP_Enter;
    mKeyMap[Qt::Key_End]       = osgGA::GUIEventAdapter::KEY_End;
    mKeyMap[Qt::Key_Return]    = osgGA::GUIEventAdapter::KEY_Return;
    mKeyMap[Qt::Key_PageUp]    = osgGA::GUIEventAdapter::KEY_Page_Up;
    mKeyMap[Qt::Key_PageDown]  = osgGA::GUIEventAdapter::KEY_Page_Down;
    mKeyMap[Qt::Key_Left]      = osgGA::GUIEventAdapter::KEY_Left;
    mKeyMap[Qt::Key_Right]     = osgGA::GUIEventAdapter::KEY_Right;
    mKeyMap[Qt::Key_Up] = osgGA::GUIEventAdapter::KEY_Up;
    mKeyMap[Qt::Key_Down]      = osgGA::GUIEventAdapter::KEY_Down;
    mKeyMap[Qt::Key_Backspace] = osgGA::GUIEventAdapter::KEY_BackSpace;
    mKeyMap[Qt::Key_Tab]       = osgGA::GUIEventAdapter::KEY_Tab;
    mKeyMap[Qt::Key_Space]     = osgGA::GUIEventAdapter::KEY_Space;
    mKeyMap[Qt::Key_Delete]    = osgGA::GUIEventAdapter::KEY_Delete;
    mKeyMap[Qt::Key_Alt]       = osgGA::GUIEventAdapter::KEY_Alt_L;
    mKeyMap[Qt::Key_Shift]     = osgGA::GUIEventAdapter::KEY_Shift_L;
    mKeyMap[Qt::Key_Control]   = osgGA::GUIEventAdapter::KEY_Control_L;
    mKeyMap[Qt::Key_Meta]      = osgGA::GUIEventAdapter::KEY_Meta_L;

    mKeyMap[Qt::Key_F1]       = osgGA::GUIEventAdapter::KEY_F1;
    mKeyMap[Qt::Key_F2]       = osgGA::GUIEventAdapter::KEY_F2;
    mKeyMap[Qt::Key_F3]       = osgGA::GUIEventAdapter::KEY_F3;
    mKeyMap[Qt::Key_F4]       = osgGA::GUIEventAdapter::KEY_F4;
    mKeyMap[Qt::Key_F5]       = osgGA::GUIEventAdapter::KEY_F5;
    mKeyMap[Qt::Key_F6]       = osgGA::GUIEventAdapter::KEY_F6;
    mKeyMap[Qt::Key_F7]       = osgGA::GUIEventAdapter::KEY_F7;
    mKeyMap[Qt::Key_F8]       = osgGA::GUIEventAdapter::KEY_F8;
    mKeyMap[Qt::Key_F9]       = osgGA::GUIEventAdapter::KEY_F9;
    mKeyMap[Qt::Key_F10]      = osgGA::GUIEventAdapter::KEY_F10;
    mKeyMap[Qt::Key_F11]      = osgGA::GUIEventAdapter::KEY_F11;
    mKeyMap[Qt::Key_F12]      = osgGA::GUIEventAdapter::KEY_F12;
    mKeyMap[Qt::Key_F13]      = osgGA::GUIEventAdapter::KEY_F13;
    mKeyMap[Qt::Key_F14]      = osgGA::GUIEventAdapter::KEY_F14;
    mKeyMap[Qt::Key_F15]      = osgGA::GUIEventAdapter::KEY_F15;
    mKeyMap[Qt::Key_F16]      = osgGA::GUIEventAdapter::KEY_F16;
    mKeyMap[Qt::Key_F17]      = osgGA::GUIEventAdapter::KEY_F17;
    mKeyMap[Qt::Key_F18]      = osgGA::GUIEventAdapter::KEY_F18;
    mKeyMap[Qt::Key_F19]      = osgGA::GUIEventAdapter::KEY_F19;
    mKeyMap[Qt::Key_F20]      = osgGA::GUIEventAdapter::KEY_F20;

    mKeyMap[Qt::Key_hyphen]   = '-';
    mKeyMap[Qt::Key_Equal]    = '=';

    mKeyMap[Qt::Key_division] = osgGA::GUIEventAdapter::KEY_KP_Divide;
    mKeyMap[Qt::Key_multiply] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
    mKeyMap[Qt::Key_Minus]    = '-';
    mKeyMap[Qt::Key_Plus]     = '+';
    // mKeyMap[Qt::Key_H          ] = osgGA::GUIEventAdapter::KEY_KP_Home;
    // mKeyMap[Qt::Key_           ] = osgGA::GUIEventAdapter::KEY_KP_Up;
    // mKeyMap[92                 ] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
    // mKeyMap[86                 ] = osgGA::GUIEventAdapter::KEY_KP_Left;
    // mKeyMap[87                 ] = osgGA::GUIEventAdapter::KEY_KP_Begin;
    // mKeyMap[88                 ] = osgGA::GUIEventAdapter::KEY_KP_Right;
    // mKeyMap[83                 ] = osgGA::GUIEventAdapter::KEY_KP_End;
    // mKeyMap[84                 ] = osgGA::GUIEventAdapter::KEY_KP_Down;
    // mKeyMap[85                 ] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
    mKeyMap[Qt::Key_Insert] = osgGA::GUIEventAdapter::KEY_KP_Insert;
    // mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_KP_Delete;
}

QtKeyboardMap::~QtKeyboardMap()
{}

int QtKeyboardMap::remapKey(QKeyEvent *event)
{
    KeyMap::iterator itr = mKeyMap.find(event->key());

    if (itr == mKeyMap.end()) {
        return int(*(event->text().toLatin1().data()));
    }
    return itr->second;
}

QSurfaceFormat traitsToFormat(const osg::GraphicsContext::Traits *traits)
{
    QSurfaceFormat format(QSurfaceFormat::defaultFormat());

    format.setRedBufferSize(traits->red);
    format.setGreenBufferSize(traits->green);
    format.setBlueBufferSize(traits->blue);
    format.setAlphaBufferSize(traits->alpha);
    format.setDepthBufferSize(traits->depth);
    format.setStencilBufferSize(traits->stencil);

    // format.setSampleBuffers(traits->sampleBuffers);
    format.setSamples(traits->samples);

    // format.setAlpha(traits->alpha > 0);
    // format.setDepth(traits->depth > 0);
    // format.setStencil(traits->stencil > 0);

    format.setStereo(traits->quadBufferStereo ? 1 : 0);

    format.setSwapBehavior(traits->doubleBuffer ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
    format.setSwapInterval(traits->vsync ? 1 : 0);

    return format;
}

void formatToTraits(const QSurfaceFormat & format, osg::GraphicsContext::Traits *traits)
{
    traits->red     = format.redBufferSize();
    traits->green   = format.greenBufferSize();
    traits->blue    = format.blueBufferSize();
    traits->alpha   = format.hasAlpha() ? format.alphaBufferSize() : 0;
    traits->depth   = format.depthBufferSize();
    traits->stencil = format.stencilBufferSize();

    // traits->sampleBuffers = format.sampleBuffers() ? 1 : 0;
    traits->samples = format.samples();

    traits->quadBufferStereo = format.stereo();

    traits->doubleBuffer     = format.swapBehavior() == QSurfaceFormat::DoubleBuffer;
    traits->vsync = format.swapInterval() >= 1;
}

void openGLContextInfo(QOpenGLContext *context, const char *at)
{
    qDebug() << "opengl context -----------------------------------------------------";
    qDebug() << "at            :" << at;
    qDebug() << "context       :" << context;
    if (context) {
        qDebug() << "share context :" << context->shareContext();
        // formatInfo(context->format());
    }
    qDebug() << "thread        :" << QThread::currentThread() << "/" << QCoreApplication::instance()->thread();
    qDebug() << "--------------------------------------------------------------------";
}

void formatInfo(const QSurfaceFormat & format)
{
    qDebug().nospace() << "surface ----------------------------------------";
    qDebug().nospace() << "version : " << format.majorVersion() << "." << format.minorVersion();
    qDebug().nospace() << "profile : " << formatProfileName(format.profile());

    qDebug().nospace() << "redBufferSize     : " << format.redBufferSize();
    qDebug().nospace() << "greenBufferSize   : " << format.greenBufferSize();
    qDebug().nospace() << "blueBufferSize    : " << format.blueBufferSize();
    qDebug().nospace() << "alphaBufferSize   : " << format.alphaBufferSize();
    qDebug().nospace() << "depthBufferSize   : " << format.depthBufferSize();
    qDebug().nospace() << "stencilBufferSize : " << format.stencilBufferSize();

    // qDebug().nospace() << "sampleBuffers" << format.sampleBuffers();
    qDebug().nospace() << "samples : " << format.samples();

    qDebug().nospace() << "stereo : " << format.stereo();

    qDebug().nospace() << "swapBehavior : " << formatSwapBehaviorName(format.swapBehavior());
    qDebug().nospace() << "swapInterval : " << format.swapInterval();
}

void traitsInfo(const osg::GraphicsContext::Traits &traits)
{
    unsigned int major, minor;

    traits.getContextVersion(major, minor);

    qDebug().nospace() << "traits  ----------------------------------------";
    qDebug().nospace() << "gl version        : " << major << "." << minor;
    qDebug().nospace() << "glContextVersion     : " << QString::fromStdString(traits.glContextVersion);
    qDebug().nospace() << "glContextFlags       : " << QString("%1").arg(traits.glContextFlags);
    qDebug().nospace() << "glContextProfileMask : " << QString("%1").arg(traits.glContextProfileMask);

    qDebug().nospace() << "red     : " << traits.red;
    qDebug().nospace() << "green   : " << traits.green;
    qDebug().nospace() << "blue    : " << traits.blue;
    qDebug().nospace() << "alpha   : " << traits.alpha;
    qDebug().nospace() << "depth   : " << traits.depth;
    qDebug().nospace() << "stencil : " << traits.stencil;

    qDebug().nospace() << "sampleBuffers : " << traits.sampleBuffers;
    qDebug().nospace() << "samples       : " << traits.samples;

    qDebug().nospace() << "pbuffer : " << traits.pbuffer;
    qDebug().nospace() << "quadBufferStereo : " << traits.quadBufferStereo;
    qDebug().nospace() << "doubleBuffer : " << traits.doubleBuffer;

    qDebug().nospace() << "vsync : " << traits.vsync;

    // qDebug().nospace() << "swapMethod : " << traits.swapMethod;
    // qDebug().nospace() << "swapInterval : " << traits.swapInterval();
}

QString formatProfileName(QSurfaceFormat::OpenGLContextProfile profile)
{
    switch (profile) {
    case QSurfaceFormat::NoProfile:
        return "No profile";

    case QSurfaceFormat::CoreProfile:
        return "Core profile";

    case QSurfaceFormat::CompatibilityProfile:
        return "Compatibility profile>";
    }
    return "<Unknown profile>";
}

QString formatSwapBehaviorName(QSurfaceFormat::SwapBehavior swapBehavior)
{
    switch (swapBehavior) {
    case QSurfaceFormat::DefaultSwapBehavior:
        return "Default";

    case QSurfaceFormat::SingleBuffer:
        return "Single buffer";

    case QSurfaceFormat::DoubleBuffer:
        return "Double buffer";

    case QSurfaceFormat::TripleBuffer:
        return "Triple buffer";
    }
    return "<Unknown swap behavior>";
}

QString getUsageString(osg::ApplicationUsage *applicationUsage)
{
    const osg::ApplicationUsage::UsageMap & keyboardBinding = applicationUsage->getKeyboardMouseBindings();

    QString desc;

    for (osg::ApplicationUsage::UsageMap::const_iterator itr = keyboardBinding.begin();
         itr != keyboardBinding.end();
         ++itr) {
        desc += QString::fromStdString(itr->first);
        desc += " : ";
        desc += QString::fromStdString(itr->second);
        desc += "\n";
    }
    return desc;
}

QString getUsageString(osgViewer::Viewer *viewer)
{
    osg::ref_ptr<osg::ApplicationUsage> applicationUsage = new osg::ApplicationUsage();

    viewer->getUsage(*applicationUsage);
    return getUsageString(applicationUsage);
}

QString getUsageString(osgViewer::CompositeViewer *viewer)
{
    osg::ref_ptr<osg::ApplicationUsage> applicationUsage = new osg::ApplicationUsage();

    viewer->getUsage(*applicationUsage);
    return getUsageString(applicationUsage);
}

#ifdef USE_OSGEARTH
osgEarth::GeoPoint createGeoPoint(const QVector3D &position, osgEarth::MapNode *mapNode)
{
    const osgEarth::SpatialReference *srs = NULL;

    if (mapNode) {
        srs = mapNode->getTerrain()->getSRS();
    } else {
        qWarning() << "Utility::createGeoPoint - null map node";
        srs = osgEarth::SpatialReference::get("wgs84");
    }
    osgEarth::GeoPoint geoPoint(srs, position.x(), position.y(), position.z(), osgEarth::ALTMODE_ABSOLUTE);
    return geoPoint;
}

bool clampGeoPoint(osgEarth::GeoPoint &geoPoint, float offset, osgEarth::MapNode *mapNode)
{
    if (!mapNode) {
        qWarning() << "Utility::clampGeoPoint - null map node";
        return false;
    }

    // establish an elevation query interface based on the features' SRS.
    osgEarth::ElevationQuery eq(mapNode->getMap());
    // qDebug() << "Utility::clampGeoPoint - SRS :" << QString::fromStdString(mapNode->getMap()->getSRS()->getName());

    double elevation;
    bool gotElevation;
#if OSGEARTH_VERSION_LESS_THAN(2, 9, 0)
    gotElevation = eq.getElevation(geoPoint, elevation, 0.0);
#else
    const double resolution = 0.0;
    double actualResolution;
    elevation    = eq.getElevation(geoPoint, resolution, &actualResolution);
    gotElevation = (elevation != NO_DATA_VALUE);
#endif
    bool clamped = false;
    if (gotElevation) {
        clamped = ((geoPoint.z() - offset) < elevation);
        if (clamped) {
            // qDebug() << "Utility::clampGeoPoint - clamping" << geoPoint.z() - offset << "/" << elevation;
            geoPoint.z() = elevation + offset;
        }
    } else {
        qDebug() << "Utility::clampGeoPoint - failed to get elevation";
    }

    return clamped;
}

void capabilitiesInfo(const osgEarth::Capabilities &caps)
{
    qDebug().nospace() << "capabilities  ----------------------------------------";

    qDebug().nospace() << "Vendor : " << QString::fromStdString(caps.getVendor());
    qDebug().nospace() << "Version : " << QString::fromStdString(caps.getVersion());
    qDebug().nospace() << "Renderer : " << QString::fromStdString(caps.getRenderer());

    qDebug().nospace() << "GLSL supported : " << caps.supportsGLSL();
    qDebug().nospace() << "GLSL version   : " << caps.getGLSLVersionInt();

    qDebug().nospace() << "GLES : " << caps.isGLES();

    qDebug().nospace() << "Num Processors : " << caps.getNumProcessors();

    qDebug().nospace() << "MaxFFPTextureUnits : " << caps.getMaxFFPTextureUnits();
    qDebug().nospace() << "MaxGPUTextureUnits : " << caps.getMaxGPUTextureUnits();
    qDebug().nospace() << "MaxGPUAttribs : " << caps.getMaxGPUAttribs();
    qDebug().nospace() << "MaxTextureSize : " << caps.getMaxTextureSize();
    qDebug().nospace() << "MaxLights : " << caps.getMaxLights();
    qDebug().nospace() << "DepthBufferBits : " << caps.getDepthBufferBits();
    qDebug().nospace() << "TextureArrays : " << caps.supportsTextureArrays();
    qDebug().nospace() << "Texture3D : " << caps.supportsTexture3D();
    qDebug().nospace() << "MultiTexture : " << caps.supportsMultiTexture();
    qDebug().nospace() << "StencilWrap : " << caps.supportsStencilWrap();
    qDebug().nospace() << "TwoSidedStencil : " << caps.supportsTwoSidedStencil();
    qDebug().nospace() << "Texture2DLod : " << caps.supportsTexture2DLod();
    qDebug().nospace() << "MipmappedTextureUpdates : " << caps.supportsMipmappedTextureUpdates();
    qDebug().nospace() << "DepthPackedStencilBuffer : " << caps.supportsDepthPackedStencilBuffer();
    qDebug().nospace() << "OcclusionQuery : " << caps.supportsOcclusionQuery();
    qDebug().nospace() << "DrawInstanced : " << caps.supportsDrawInstanced();
    qDebug().nospace() << "UniformBufferObjects : " << caps.supportsUniformBufferObjects();
    qDebug().nospace() << "NonPowerOfTwoTextures : " << caps.supportsNonPowerOfTwoTextures();
    qDebug().nospace() << "MaxUniformBlockSize : " << caps.getMaxUniformBlockSize();
    qDebug().nospace() << "PreferDisplayListsForStaticGeometry : " << caps.preferDisplayListsForStaticGeometry();
    qDebug().nospace() << "FragDepthWrite : " << caps.supportsFragDepthWrite();
}
#endif // USE_OSGEARTH

void registerTypes()
{
    int maj = 1, min = 0;

    // viewport
    qmlRegisterType<osgQtQuick::OSGViewport>("OsgQtQuick", maj, min, "OSGViewport");
    qmlRegisterType<osgQtQuick::UpdateMode>("OsgQtQuick", maj, min, "UpdateMode");

    // basic nodes
    qmlRegisterType<osgQtQuick::OSGNode>("OsgQtQuick", maj, min, "OSGNode");

    qmlRegisterType<osgQtQuick::OSGGroup>("OsgQtQuick", maj, min, "OSGGroup");

    qmlRegisterType<osgQtQuick::OSGTransformNode>("OsgQtQuick", maj, min, "OSGTransformNode");

    // primitive nodes
    qmlRegisterType<osgQtQuick::OSGShapeNode>("OsgQtQuick", maj, min, "OSGShapeNode");
    qmlRegisterType<osgQtQuick::ShapeType>("OsgQtQuick", maj, min, "ShapeType");

    qmlRegisterType<osgQtQuick::OSGImageNode>("OsgQtQuick", maj, min, "OSGImageNode");

    qmlRegisterType<osgQtQuick::OSGTextNode>("OsgQtQuick", maj, min, "OSGTextNode");

    qmlRegisterType<osgQtQuick::OSGBillboardNode>("OsgQtQuick", maj, min, "OSGBillboardNode");

    qmlRegisterType<osgQtQuick::OSGFileNode>("OsgQtQuick", maj, min, "OSGFileNode");
    qmlRegisterType<osgQtQuick::OptimizeMode>("OsgQtQuick", maj, min, "OptimizeMode");

    // camera nodes
    qmlRegisterType<osgQtQuick::OSGCamera>("OsgQtQuick", maj, min, "OSGCamera");

    // camera manipulators
    qmlRegisterType<osgQtQuick::OSGCameraManipulator>("OsgQtQuick", maj, min, "OSGCameraManipulator");
    qmlRegisterType<osgQtQuick::OSGNodeTrackerManipulator>("OsgQtQuick", maj, min, "OSGNodeTrackerManipulator");
    qmlRegisterType<osgQtQuick::TrackerMode>("OsgQtQuick", maj, min, "TrackerMode");
    qmlRegisterType<osgQtQuick::OSGTrackballManipulator>("OsgQtQuick", maj, min, "OSGTrackballManipulator");

#ifdef USE_OSGEARTH
    qmlRegisterType<osgQtQuick::OSGSkyNode>("OsgQtQuick", maj, min, "OSGSkyNode");
    qmlRegisterType<osgQtQuick::OSGGeoTransformNode>("OsgQtQuick", maj, min, "OSGGeoTransformNode");

    qmlRegisterType<osgQtQuick::OSGEarthManipulator>("OsgQtQuick", maj, min, "OSGEarthManipulator");
    qmlRegisterType<osgQtQuick::OSGGeoTransformManipulator>("OsgQtQuick", maj, min, "OSGGeoTransformManipulator");
#endif // USE_OSGEARTH
}
} // namespace osgQtQuick
