#include "OSGFileNode.hpp"

#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>

#include <QUrl>
#include <QOpenGLContext>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>

namespace osgQtQuick {
class OSGFileLoader : public QThread {
    Q_OBJECT

public:
    OSGFileLoader(const QUrl &url) : url(url) {}

    void run()
    {
        load();
    }

    void load()
    {
        QElapsedTimer t;

        t.start();
        qDebug() << "OSGFileLoader - reading URL" << url;
        QString s = url.toString();
        if (s.startsWith("file://")) {
            s = s.right(s.length() - QString("file://").length());
        }
        s = s.replace("%5C", "/");
        // qDebug() << "OSGFileLoader - file" << s;
        // TODO use Options to control caching...
        qDebug() << "OSGFileLoader - load - currentContext" << QOpenGLContext::currentContext();
        osg::Node *node = osgDB::readNodeFile(s.toStdString());
        qDebug() << "OSGFileLoader - reading node" << node << "took" << t.elapsed() << "ms";

        emit loaded(url, node);
    }

signals:
    void loaded(const QUrl & url, osg::Node *node);

private:
    QUrl url;
};

struct OSGFileNode::Hidden : public QObject {
    Q_OBJECT

public:
    Hidden(OSGFileNode *parent) : QObject(parent), self(parent), url(), async(true), optimizeMode(None) {}

    bool acceptSource(QUrl url)
    {
        qDebug() << "OSGFileNode - acceptSource" << url;
        if (this->url == url) {
            return false;
        }

        this->url = url;


        return true;
    }

    OSGFileNode *const self;

    QUrl url;
    bool async;
    OptimizeMode optimizeMode;

// private:

    void asyncLoad(const QUrl &url)
    {
        OSGFileLoader *loader = new OSGFileLoader(url);

        connect(loader, SIGNAL(loaded(const QUrl &, osg::Node *)), this, SLOT(onLoaded(const QUrl &, osg::Node *)));
        connect(loader, SIGNAL(finished()), loader, SLOT(deleteLater()));
        loader->start();
    }

    void syncLoad(const QUrl &url)
    {
        OSGFileLoader loader(url);

        connect(&loader, SIGNAL(loaded(const QUrl &, osg::Node *)), this, SLOT(onLoaded(const QUrl &, osg::Node *)));
        loader.load();
    }

public slots:
    void onLoaded(const QUrl &url, osg::Node *node)
    {
        if (optimizeMode != OSGFileNode::None) {
            qDebug() << "OSGFileNode - optimize" << node << optimizeMode;
            osgUtil::Optimizer optimizer;
            optimizer.optimize(node, osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS);
        }
        self->setNode(node);
    }
};

OSGFileNode::OSGFileNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGFileNode - <init>";
}

OSGFileNode::~OSGFileNode()
{
    qDebug() << "OSGFileNode - <destruct>";
}

const QUrl OSGFileNode::source() const
{
    return h->url;
}

void OSGFileNode::setSource(const QUrl &url)
{
    qDebug() << "OSGFileNode - setSource" << url;
    if (h->acceptSource(url)) {
        emit sourceChanged(source());
    }
}

bool OSGFileNode::async() const
{
    return h->async;
}

void OSGFileNode::setAsync(const bool async)
{
    qDebug() << "OSGFileNode - setAsync" << async;
    if (h->async != async) {
        h->async = async;
        emit asyncChanged(async);
    }
}

OSGFileNode::OptimizeMode OSGFileNode::optimizeMode() const
{
    return h->optimizeMode;
}

void OSGFileNode::setOptimizeMode(OptimizeMode mode)
{
    qDebug() << "OSGFileNode - setOptimizeMode" << mode;
    if (h->optimizeMode != mode) {
        h->optimizeMode = mode;
        emit optimizeModeChanged(optimizeMode());
    }
}

void OSGFileNode::realize()
{
    qDebug() << "OSGFileNode - realize";
    if (h->async) {
        h->asyncLoad(h->url);
    } else {
        h->syncLoad(h->url);
    }
}
} // namespace osgQtQuick

#include "OSGFileNode.moc"
