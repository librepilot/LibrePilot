#include "Utility.hpp"

#include <osg/NodeCallback>
#include <osg/Camera>
#include <osg/io_utils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
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
#include "osgQtQuick/OSGCamera.hpp"
#include "osgQtQuick/OSGViewport.hpp"

#include <osgEarth/CullingUtils>

#include <QFont>

namespace osgQtQuick {


template<class T>
class FindTopMostNodeOfTypeVisitor : public osg::NodeVisitor
{
public:
    FindTopMostNodeOfTypeVisitor():
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _foundNode(0)
    {}

    void apply(osg::Node& node)
    {
        T* result = dynamic_cast<T*>(&node);
        if (result)
        {
            _foundNode = result;
        }
        else
        {
            traverse(node);
        }
    }

    T* _foundNode;
};


template<class T> T* findTopMostNodeOfType(osg::Node* node)
{
    if (!node) return 0;

    FindTopMostNodeOfTypeVisitor<T> fnotv;
    node->accept(fnotv);

    return fnotv._foundNode;
}

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

    qmlRegisterType<osgQtQuick::OSGCamera>(uri, maj, min, "OSGCamera");
    qmlRegisterType<osgQtQuick::OSGEarthNode>(uri, maj, min, "OSGEarthNode");
}

} // namespace osgQtQuick
