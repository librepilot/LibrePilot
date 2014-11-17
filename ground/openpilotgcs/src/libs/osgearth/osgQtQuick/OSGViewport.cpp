#include "OSGViewport.hpp"

#include "OSGNode.hpp"
#include "QuickWindowViewer.hpp"
#include "Utility.hpp"

#include <QQuickWindow>
#include <QOpenGLFramebufferObject>
#include <QSGSimpleTextureNode>
#include <QDebug>

#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>

#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/Sky>

namespace osgQtQuick {

struct OSGViewport::Hidden : public QObject
{
    Q_OBJECT

    struct PreDraw: public osg::Camera::DrawCallback {
    public:
        PreDraw(Hidden *h);

        void operator ()(osg::RenderInfo &renderInfo) const;

        mutable Hidden *h;
    };
    friend struct PreDraw;

    struct PostDraw: public osg::Camera::DrawCallback {
    public:
        PostDraw(Hidden *h);

        void operator ()(osg::RenderInfo &renderInfo) const;

        mutable Hidden *h;
    };
    friend struct PostDraw;

    struct CameraUpdateCallback : public osg::NodeCallback
    {
    public:
        CameraUpdateCallback(Hidden *h);

        void operator()(osg::Node* node, osg::NodeVisitor* nv);

        mutable Hidden *h;
    };
    friend class CameraUpdateCallback;

public:

    Hidden(OSGViewport *quickItem) :
        QObject(quickItem),
        drawingMode(OSGViewport::Buffer),
        window(0),
        quickItem(quickItem),
        sceneData(0),
        camera(0),
        cameraDirty(false),
        mode(OSGViewport::Native),
        fbo(0),
        texture(0),
        textureNode(0)
    {
        // Создание сцены и вьювера
        initOSG();

        acceptQuickItem();
    }

    ~Hidden() {

    }

    QPointF mousePoint(QMouseEvent *event) {
//        qreal x = 0.01 * (event->x() - quickItem->width() / 2);
//        qreal y = 0.01 * (event->y() - quickItem->height() / 2);
        qreal x = 2.0 * (event->x() - quickItem->width() / 2) / quickItem->width();
        qreal y = 2.0 * (event->y() - quickItem->height() / 2) / quickItem->height();
        return QPointF(x, y);
    }

    bool acceptSceneData(osgQtQuick::OSGNode *node)
    {
        if (sceneData == node) {
            return false;
        }

        if (sceneData) {
            disconnect(sceneData);
        }

        sceneData = node;

        acceptNode(node->node());

        if (sceneData) {
            connect(sceneData, SIGNAL(nodeChanged(osg::Node*)), this, SLOT(onNodeChanged(osg::Node*)));
        }

        return true;
    }

    bool acceptNode(osg::Node *node)
    {
        osgEarth::MapNode *mapNode = osgEarth::MapNode::findMapNode(node);
        qDebug() << "Map Node" << mapNode;
        if (mapNode) {
//            view->setCameraManipulator(new osgEarth::Util::EarthManipulator());
            view->getCamera()->setCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
        }

        osgEarth::Util::SkyNode *skyNode = osgQtQuick::findTopMostNodeOfType<osgEarth::Util::SkyNode>(node);
        qDebug() << "Sky Node" << skyNode;
        if (skyNode) {
            skyNode->attach(view, 0);
        }

        qDebug() << "set scene data" << node;
        view->setSceneData(node);

        return true;
    }

    bool acceptMode(OSGViewport::DrawingMode mode) {
        if (this->mode == mode) return false;

        this->mode = mode;
        if (mode == OSGViewport::Buffer) {
            qDebug() << "mode is now Buffer";
            if(!preDraw.valid()) {
                preDraw = new PreDraw(this);
                view->getCamera()->setPreDrawCallback(preDraw.get());
            }
            if(!postDraw.valid()) {
                postDraw = new PostDraw(this);
                view->getCamera()->setPostDrawCallback(postDraw.get());
            }
        }

        return true;
    }

    bool acceptCamera(OSGCamera *camera) {
        if (this->camera == camera) {
            return false;
        }

        qDebug() << "accept camera";

        if (this->camera) {
            disconnect(this->camera);
        }

        this->camera = camera;

        cameraDirty = true;

        this->camera->installCamera(view);

        // install camera update callback
        view->getCamera()->addUpdateCallback(new CameraUpdateCallback(this));

        if (this->camera) {
            connect(this->camera, SIGNAL(attitudeChanged(qreal, qreal, qreal)), this, SLOT(onCameraDirty()));
            connect(this->camera, SIGNAL(positionChanged(double, double, double)), this, SLOT(onCameraDirty()));
        }

        return true;
    }

    void setKeyboardModifiers(QInputEvent* event)
    {
        int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
        unsigned int mask = 0;
        if ( modkey & Qt::ShiftModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
        if ( modkey & Qt::ControlModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
        if ( modkey & Qt::AltModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
        view->getEventQueue()->getCurrentEventState()->setModKeyMask( mask );
    }

    // Public data
    OSGViewport::DrawingMode drawingMode;

    // Public Qt data
    QQuickWindow *window;
    OSGViewport *quickItem;

    // Public OSG data
    osg::ref_ptr<osgViewer::View> view;

    osgQtQuick::OSGNode *sceneData;

    OSGCamera *camera;
    bool cameraDirty;

    OSGViewport::DrawingMode mode;

    QOpenGLFramebufferObject *fbo;
    QSGTexture *texture;
    QSGSimpleTextureNode *textureNode;

    osg::ref_ptr<PreDraw> preDraw;
    osg::ref_ptr<PostDraw> postDraw;

    static QtKeyboardMap keyMap;

public slots:
    void updateViewport() {
        if (!quickItem->window()) return;
        if (mode == OSGViewport::Native) {
            QRectF rect = quickItem->mapRectToItem(0, quickItem->boundingRect());
            view->getCamera()->setViewport(rect.x(),
                                           quickItem->window()->height() - (rect.y() + rect.height()),
                                           rect.width(),
                                           rect.height());
            view->getCamera()->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(rect.width())/static_cast<double>(rect.height()), 1.0f, 10000.0f );
        }
        if (mode == OSGViewport::Buffer) {
            QSize size(quickItem->boundingRect().size().toSize());
            view->getCamera()->setViewport(0, 0, size.width(), size.height());
            view->getCamera()->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(size.width())/static_cast<double>(size.height()), 1.0f, 10000.0f );
            if (texture && texture->textureSize() != size) {
                updateFBO();
            }
        }
    }

private slots:

    void onWindowChanged(QQuickWindow *window) {
        // TODO if window is destroyed, the associated QuickWindowViewer should be destroyed too
        QuickWindowViewer *qwv;
        if (this->window && (qwv = QuickWindowViewer::instance(this->window))) {
            disconnect(this->window);
            qwv->compositeViewer()->removeView(view.get());
        }
        if (window && (qwv = QuickWindowViewer::instance(window))) {
            view->getCamera()->setGraphicsContext(qwv->graphicsContext());
            qwv->compositeViewer()->addView(view.get());
            connect(window, SIGNAL(widthChanged(int)), this, SLOT(updateViewport()));
            connect(window, SIGNAL(heightChanged(int)), this, SLOT(updateViewport()));
            updateViewport();
        }
        this->window = window;
    }

    void onNodeChanged(osg::Node *node)
    {
        qDebug() << "onNodeChanged";
        if (view.valid()) {
            acceptNode(node);
        }
    }

    void onCameraDirty()
    {
        cameraDirty = true;
    }

private:
    void initOSG() {
        view = new osgViewer::View();
        view->addEventHandler(new osgViewer::StatsHandler());
    }


    void acceptQuickItem() {
        Q_ASSERT(quickItem);
        qDebug() << "acceptQuickItem" << quickItem->window();
        connect(quickItem, SIGNAL(windowChanged(QQuickWindow*)),
                this, SLOT(onWindowChanged(QQuickWindow*)));
    }

    void initFBO()
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        QRectF rect = quickItem->mapRectToItem(0, quickItem->boundingRect());
        QSize size(rect.size().toSize());
        fbo = new QOpenGLFramebufferObject(size, format);
        texture = quickItem->window()->createTextureFromId(fbo->texture(), size);
        textureNode = new QSGSimpleTextureNode();
        textureNode->setRect(0, quickItem->height(), quickItem->width(), -quickItem->height());
        textureNode->setTexture(texture);
        quickItem->setFlag(QQuickItem::ItemHasContents, true);
        updateViewport();
        quickItem->update();
    }

    void updateFBO()
    {
        QRectF rect = quickItem->mapRectToItem(0, quickItem->boundingRect());
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        QSize size(rect.size().toSize());
        if (fbo) delete fbo;
        fbo = new QOpenGLFramebufferObject(size, format);
        if (texture) delete texture;
        texture = quickItem->window()->createTextureFromId(fbo->texture(), size);
        textureNode = new QSGSimpleTextureNode();
        textureNode->setRect(0, quickItem->height(), quickItem->width(), -quickItem->height());
        textureNode->setTexture(texture);
        quickItem->update();
    }
};

QtKeyboardMap OSGViewport::Hidden::keyMap = QtKeyboardMap();

/* ----------------------------------------------- struct Hidden::PreDraw --- */

OSGViewport::Hidden::PreDraw::PreDraw(Hidden *h) : h(h) {
}

void OSGViewport::Hidden::PreDraw::operator ()(osg::RenderInfo &/*renderInfo*/) const
{
    if (!h->fbo) h->initFBO();
    if (h->fbo) {
        if (!h->fbo->bind()) {
            qWarning() << "PreDraw - failed to bind fbo!";
        }
    }
}

/* ----------------------------------------------- struct Hidden::PostDraw --- */

OSGViewport::Hidden::PostDraw::PostDraw(Hidden *h) : h(h) {
}

void OSGViewport::Hidden::PostDraw::operator ()(osg::RenderInfo &/*renderInfo*/) const
{
    if (h->fbo) {
        if (!h->fbo->bindDefault()) {
            qWarning() << "PostDraw - failed to unbind fbo!";
        }
    }
}

/* struct Hidden::CameraUpdateCallback */

OSGViewport::Hidden::CameraUpdateCallback::CameraUpdateCallback(Hidden *h) : h(h) {}

void OSGViewport::Hidden::CameraUpdateCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if (h->cameraDirty) {
        h->cameraDirty = false;
        h->camera->updateCamera(h->view->getCamera());
    }
}

/* ---------------------------------------------------- class OSGViewport --- */

OSGViewport::OSGViewport(QQuickItem *parent) :
    QQuickItem(parent), h(new Hidden(this))
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

OSGViewport::~OSGViewport()
{
}

void OSGViewport::setDrawingMode(OSGViewport::DrawingMode mode)
{
    if (h->drawingMode != mode) {
        h->drawingMode = mode;
    }
}

osgQtQuick::OSGNode *OSGViewport::sceneData()
{
    return h->sceneData;
}

void OSGViewport::setSceneData(osgQtQuick::OSGNode *node)
{
    if (h->acceptSceneData(node)) {
        emit sceneDataChanged(node);
    }
}

OSGCamera* OSGViewport::camera()
{
    return h->camera;
}

void OSGViewport::setCamera(OSGCamera *camera)
{
    if (h->acceptCamera(camera)) {
        emit cameraChanged(camera);
    }
}

QColor OSGViewport::color() const
{
    const osg::Vec4 osgColor = h->view->getCamera()->getClearColor();
    return QColor::fromRgbF(
                osgColor.r(),
                osgColor.g(),
                osgColor.b(),
                osgColor.a());
}

void OSGViewport::setColor(const QColor &color)
{
    osg::Vec4 osgColor(
                color.redF(),
                color.greenF(),
                color.blueF(),
                color.alphaF());
    if (h->view->getCamera()->getClearColor() != osgColor) {
        h->view->getCamera()->setClearColor(osgColor);
        emit colorChanged(color);
    }
}

OSGViewport::DrawingMode OSGViewport::mode() const
{
    return h->mode;
}

void OSGViewport::setMode(OSGViewport::DrawingMode mode)
{
    if (h->acceptMode(mode)) emit modeChanged(mode);
}

void OSGViewport::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (window()) {
        h->updateViewport();
    }
}

void OSGViewport::mousePressEvent(QMouseEvent *event)
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonPress(pos.x(), pos.y(), button);
}

void OSGViewport::mouseMoveEvent(QMouseEvent *event)
{
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseMotion(pos.x(), pos.y());
}

void OSGViewport::mouseReleaseEvent(QMouseEvent *event)
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    h->setKeyboardModifiers( event );
    QPointF pos = h->mousePoint(event);
    h->view->getEventQueue()->mouseButtonRelease(pos.x(), pos.y(), button);
}

void OSGViewport::wheelEvent(QWheelEvent *event)
{
    h->view->getEventQueue()->mouseScroll(
                event->orientation() == Qt::Vertical ?
                    (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) :
                    (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT) );

}

void OSGViewport::keyPressEvent(QKeyEvent *event)
{
    h->setKeyboardModifiers( event );
    int value = h->keyMap.remapKey( event );
    h->view->getEventQueue()->keyPress( value );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
//    if( _forwardKeyEvents )
//        inherited::keyPressEvent( event );
}

void OSGViewport::keyReleaseEvent(QKeyEvent *event)
{
    if( event->isAutoRepeat() )
    {
        event->ignore();
    }
    else
    {
        h->setKeyboardModifiers( event );
        int value = h->keyMap.remapKey( event );
        h->view->getEventQueue()->keyRelease( value );
    }

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    // TODO implement
//    if( _forwardKeyEvents )
//        inherited::keyReleaseEvent( event );
}

QSGNode *OSGViewport::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);

    if (oldNode && oldNode != h->textureNode) {
        delete oldNode;
    }

    return h->textureNode;
}

} // namespace osgQtQuick

#include "OSGViewport.moc"
