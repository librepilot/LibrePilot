#include "Utility.hpp"

// osgQtQuick qml types
#include "OSGNode.hpp"
#include "OSGGroup.hpp"
#include "OSGNodeFile.hpp"
#include "OSGTransformNode.hpp"
#include "OSGCubeNode.hpp"
#include "OSGTextNode.hpp"
#include "OSGModelNode.hpp"
#include "OSGSkyNode.hpp"
#include "OSGCamera.hpp"
#include "OSGViewport.hpp"

#include <osg/NodeCallback>
#include <osg/Camera>
#include <osg/io_utils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
#include <osgGA/GUIEventAdapter>
#include <osgText/Font>
#include <osgText/Text>
#include <osgText/String>
#include <osgQt/QFontImplementation>

#include <osgEarth/CullingUtils>

#include <QFont>
#include <QKeyEvent>

namespace osgQtQuick {

class MyCullCallback : public osg::NodeCallback
{
public:
    MyCullCallback() { }

    virtual ~MyCullCallback() { }

public:
    virtual void operator()( osg::Node* node, osg::NodeVisitor* nv )
    {
        osgUtil::CullVisitor* cv = osgEarth::Culling::asCullVisitor(nv);
        if ( cv )
        {
            OSG_DEBUG << "****** Node:" << node << " " << node->getName() << std::endl;
            OSG_DEBUG << "projection matrix: " << *(cv->getProjectionMatrix()) << std::endl;
            OSG_DEBUG << "model view matrix: " << *(cv->getModelViewMatrix()) << std::endl;
        }
        osg::MatrixTransform *mt = dynamic_cast<osg::MatrixTransform*>( node );
        if (mt) {
            OSG_DEBUG << "matrix: " << mt->getMatrix() << std::endl;
        }
        traverse(node, nv);
    }

};

class InsertCallbacksVisitor : public osg::NodeVisitor
{
public:
    InsertCallbacksVisitor():osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
    }
    virtual void apply(osg::Node& node)
    {
        //node.setUpdateCallback(new UpdateCallback());
        node.setCullCallback(new MyCullCallback());
        traverse(node);
    }
    virtual void apply(osg::Geode& geode)
    {
//        geode.setUpdateCallback(new UpdateCallback());
//        //note, it makes no sense to attach a cull callback to the node
//        //at there are no nodes to traverse below the geode, only
//        //drawables, and as such the Cull node callbacks is ignored.
//        //If you wish to control the culling of drawables
//        //then use a drawable cullback...
//        for(unsigned int i=0;i<geode.getNumDrawables();++i)
//        {
//            geode.getDrawable(i)->setUpdateCallback(new DrawableUpdateCallback());
//            geode.getDrawable(i)->setCullCallback(new DrawableCullCallback());
//            geode.getDrawable(i)->setDrawCallback(new DrawableDrawCallback());
//        }
    }
    virtual void apply(osg::Transform& node)
    {
        apply((osg::Node&) node);
    }
};

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
    if (!font.fromString(
                QString::fromStdString(
                    osgDB::getNameLessExtension(name)))) {
        return 0;
    }

    return new osgText::Font(new osgQt::QFontImplementation(font));
}

osgText::Font *createFont(const QFont &font)
{
    return new osgText::Font(new osgQt::QFontImplementation(font));
}

osgText::Text *createText(const osg::Vec3 &pos, const std::string &content, float size, osgText::Font *font)
{
    osg::ref_ptr<osgText::Text> text = new osgText::Text();
    if (font) text->setFont(font);
    text->setCharacterSize(size);
    text->setAxisAlignment(osgText::TextBase::XY_PLANE);
    text->setPosition(pos);
    text->setText(content, osgText::String::ENCODING_UTF8);
    return text.release();
}

// Copied from "GraphicsWindowQt.cpp" (osg module osgQt)
QtKeyboardMap::QtKeyboardMap()
{
    mKeyMap[Qt::Key_Escape     ] = osgGA::GUIEventAdapter::KEY_Escape;
    mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
    mKeyMap[Qt::Key_Home       ] = osgGA::GUIEventAdapter::KEY_Home;
    mKeyMap[Qt::Key_Enter      ] = osgGA::GUIEventAdapter::KEY_KP_Enter;
    mKeyMap[Qt::Key_End        ] = osgGA::GUIEventAdapter::KEY_End;
    mKeyMap[Qt::Key_Return     ] = osgGA::GUIEventAdapter::KEY_Return;
    mKeyMap[Qt::Key_PageUp     ] = osgGA::GUIEventAdapter::KEY_Page_Up;
    mKeyMap[Qt::Key_PageDown   ] = osgGA::GUIEventAdapter::KEY_Page_Down;
    mKeyMap[Qt::Key_Left       ] = osgGA::GUIEventAdapter::KEY_Left;
    mKeyMap[Qt::Key_Right      ] = osgGA::GUIEventAdapter::KEY_Right;
    mKeyMap[Qt::Key_Up         ] = osgGA::GUIEventAdapter::KEY_Up;
    mKeyMap[Qt::Key_Down       ] = osgGA::GUIEventAdapter::KEY_Down;
    mKeyMap[Qt::Key_Backspace  ] = osgGA::GUIEventAdapter::KEY_BackSpace;
    mKeyMap[Qt::Key_Tab        ] = osgGA::GUIEventAdapter::KEY_Tab;
    mKeyMap[Qt::Key_Space      ] = osgGA::GUIEventAdapter::KEY_Space;
    mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
    mKeyMap[Qt::Key_Alt        ] = osgGA::GUIEventAdapter::KEY_Alt_L;
    mKeyMap[Qt::Key_Shift      ] = osgGA::GUIEventAdapter::KEY_Shift_L;
    mKeyMap[Qt::Key_Control    ] = osgGA::GUIEventAdapter::KEY_Control_L;
    mKeyMap[Qt::Key_Meta       ] = osgGA::GUIEventAdapter::KEY_Meta_L;

    mKeyMap[Qt::Key_F1         ] = osgGA::GUIEventAdapter::KEY_F1;
    mKeyMap[Qt::Key_F2         ] = osgGA::GUIEventAdapter::KEY_F2;
    mKeyMap[Qt::Key_F3         ] = osgGA::GUIEventAdapter::KEY_F3;
    mKeyMap[Qt::Key_F4         ] = osgGA::GUIEventAdapter::KEY_F4;
    mKeyMap[Qt::Key_F5         ] = osgGA::GUIEventAdapter::KEY_F5;
    mKeyMap[Qt::Key_F6         ] = osgGA::GUIEventAdapter::KEY_F6;
    mKeyMap[Qt::Key_F7         ] = osgGA::GUIEventAdapter::KEY_F7;
    mKeyMap[Qt::Key_F8         ] = osgGA::GUIEventAdapter::KEY_F8;
    mKeyMap[Qt::Key_F9         ] = osgGA::GUIEventAdapter::KEY_F9;
    mKeyMap[Qt::Key_F10        ] = osgGA::GUIEventAdapter::KEY_F10;
    mKeyMap[Qt::Key_F11        ] = osgGA::GUIEventAdapter::KEY_F11;
    mKeyMap[Qt::Key_F12        ] = osgGA::GUIEventAdapter::KEY_F12;
    mKeyMap[Qt::Key_F13        ] = osgGA::GUIEventAdapter::KEY_F13;
    mKeyMap[Qt::Key_F14        ] = osgGA::GUIEventAdapter::KEY_F14;
    mKeyMap[Qt::Key_F15        ] = osgGA::GUIEventAdapter::KEY_F15;
    mKeyMap[Qt::Key_F16        ] = osgGA::GUIEventAdapter::KEY_F16;
    mKeyMap[Qt::Key_F17        ] = osgGA::GUIEventAdapter::KEY_F17;
    mKeyMap[Qt::Key_F18        ] = osgGA::GUIEventAdapter::KEY_F18;
    mKeyMap[Qt::Key_F19        ] = osgGA::GUIEventAdapter::KEY_F19;
    mKeyMap[Qt::Key_F20        ] = osgGA::GUIEventAdapter::KEY_F20;

    mKeyMap[Qt::Key_hyphen     ] = '-';
    mKeyMap[Qt::Key_Equal      ] = '=';

    mKeyMap[Qt::Key_division   ] = osgGA::GUIEventAdapter::KEY_KP_Divide;
    mKeyMap[Qt::Key_multiply   ] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
    mKeyMap[Qt::Key_Minus      ] = '-';
    mKeyMap[Qt::Key_Plus       ] = '+';
    //mKeyMap[Qt::Key_H          ] = osgGA::GUIEventAdapter::KEY_KP_Home;
    //mKeyMap[Qt::Key_           ] = osgGA::GUIEventAdapter::KEY_KP_Up;
    //mKeyMap[92                 ] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
    //mKeyMap[86                 ] = osgGA::GUIEventAdapter::KEY_KP_Left;
    //mKeyMap[87                 ] = osgGA::GUIEventAdapter::KEY_KP_Begin;
    //mKeyMap[88                 ] = osgGA::GUIEventAdapter::KEY_KP_Right;
    //mKeyMap[83                 ] = osgGA::GUIEventAdapter::KEY_KP_End;
    //mKeyMap[84                 ] = osgGA::GUIEventAdapter::KEY_KP_Down;
    //mKeyMap[85                 ] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
    mKeyMap[Qt::Key_Insert     ] = osgGA::GUIEventAdapter::KEY_KP_Insert;
    //mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_KP_Delete;
}

QtKeyboardMap::~QtKeyboardMap()
{
}

int QtKeyboardMap::remapKey(QKeyEvent* event)
{
    KeyMap::iterator itr = mKeyMap.find(event->key());
    if (itr == mKeyMap.end())
    {
        return int(*(event->text().toLatin1().data()));
    }
    return itr->second;
}

void registerTypes(const char *uri)
{
    //Q_ASSERT(uri == QLatin1String("osgQtQuick"));
    int maj = 1, min = 0;
    // @uri osgQtQuick
    qmlRegisterType<osgQtQuick::OSGNode>(uri, maj, min, "OSGNode");
    qmlRegisterType<osgQtQuick::OSGGroup>(uri, maj, min, "OSGGroup");
    qmlRegisterType<osgQtQuick::OSGNodeFile>(uri, maj, min, "OSGNodeFile");
    qmlRegisterType<osgQtQuick::OSGTransformNode>(uri, maj, min, "OSGTransformNode");
    qmlRegisterType<osgQtQuick::OSGTextNode>(uri, maj, min, "OSGTextNode");
    qmlRegisterType<osgQtQuick::OSGCubeNode>(uri, maj, min, "OSGCubeNode");
    qmlRegisterType<osgQtQuick::OSGViewport>(uri, maj, min, "OSGViewport");

    qmlRegisterType<osgQtQuick::OSGModelNode>(uri, maj, min, "OSGModelNode");
    qmlRegisterType<osgQtQuick::OSGSkyNode>(uri, maj, min, "OSGSkyNode");
    qmlRegisterType<osgQtQuick::OSGCamera>(uri, maj, min, "OSGCamera");
}

} // namespace osgQtQuick
