#include "OSGNodeFile.hpp"

#include <osgDB/ReadFile>

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

struct OSGNodeFile::Hidden : public QObject {
    Q_OBJECT

public:
    Hidden(OSGNodeFile *parent) : QObject(parent), self(parent), url(), async(true) {}

    bool acceptSource(QUrl url)
    {
        qDebug() << "OSGNodeFile - acceptSource" << url;
        if (this->url == url) {
            return false;
        }

        this->url = url;


        return true;
    }

    OSGNodeFile *const self;

    QUrl url;
    bool async;

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
        self->setNode(node);
    }
};

OSGNodeFile::OSGNodeFile(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGNodeFile - <init>";
}

OSGNodeFile::~OSGNodeFile()
{
    qDebug() << "OSGNodeFile - <destruct>";
}

const QUrl OSGNodeFile::source() const
{
    return h->url;
}

void OSGNodeFile::setSource(const QUrl &url)
{
    qDebug() << "OSGNodeFile - setSource" << url;
    if (h->acceptSource(url)) {
        emit sourceChanged(source());
    }
}

bool OSGNodeFile::async() const
{
    return h->async;
}

void OSGNodeFile::setAsync(const bool async)
{
    qDebug() << "OSGNodeFile - setAsync" << async;
    if (h->async != async) {
        h->async = async;
        emit asyncChanged(async);
    }
}

void OSGNodeFile::realize()
{
    qDebug() << "OSGNodeFile - realize";
    if (h->async) {
        h->asyncLoad(h->url);
    } else {
        h->syncLoad(h->url);
    }
}
} // namespace osgQtQuick

#include "OSGNodeFile.moc"
