#include "OSGNodeFile.hpp"

#include <osgDB/ReadFile>

#include <QUrl>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>

namespace osgQtQuick {

class OSGFileLoader: public QThread
{
    Q_OBJECT

public:
    OSGFileLoader(const QUrl &url) : url(url) {
    }

    void run() {
        load();
    }

    void load() {
        QElapsedTimer t;
        t.start();
        qDebug() << "OSGFileLoader - reading URL" << url;
        QString s = url.toString();
        s = s.right(s.length() - QString("file://").length());
        //qDebug() << "OSGFileLoader - file" << s;
        // TODO use Options to control caching...
        osg::Node *node = osgDB::readNodeFile(s.toStdString());
        qDebug() << "OSGFileLoader - reading node" << node << "took" << t.elapsed();

        emit loaded(url, node);
    }

signals:
    void loaded(const QUrl& url, osg::Node *node);

private:
    QUrl url;
};

struct OSGNodeFile::Hidden: public QObject
{
    Q_OBJECT

public:
    Hidden(OSGNodeFile *parent) : QObject(parent), url(), async(true), pub(parent) {}

    void asyncLoad(const QUrl &url) {
        OSGFileLoader *loader = new OSGFileLoader(url);
        connect(loader, SIGNAL(loaded(const QUrl&, osg::Node*)), this, SLOT(onLoaded(const QUrl&, osg::Node*)));
        connect(loader, SIGNAL(finished()), loader, SLOT(deleteLater()));
        loader->start();
    }

    void syncLoad(const QUrl &url) {
        OSGFileLoader loader(url);
        connect(&loader, SIGNAL(loaded(const QUrl&, osg::Node*)), this, SLOT(onLoaded(const QUrl&, osg::Node*)));
        loader.load();
    }

    QUrl url;
    bool async;
    OSGNodeFile *pub;

public slots:
    void onLoaded(const QUrl &url, osg::Node *node) {
        pub->setNode(node);
        if (this->url != url) {
            this->url = url;
            emit pub->sourceChanged(url);
        }
    }
};

OSGNodeFile::OSGNodeFile(QObject *parent) :
    OSGNode(parent),
    h(new Hidden(this))
{    
}

OSGNodeFile::~OSGNodeFile()
{
}

const QUrl OSGNodeFile::source() const
{
    return h->url;
}

void OSGNodeFile::setSource(const QUrl &url)
{
    qDebug() << "OSGNodeFile - setSource" << url;
    if (h->async) {
        h->asyncLoad(url);
    }
    else {
        h->syncLoad(url);
    }
//    if (h->url != url)
//    {
//        h->url = url;
//        setNode(osgDB::readNodeFile(url.toString().toStdString()));
//        emit sourceChanged(url);
//    }
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

} // namespace osgQtQuick

#include "OSGNodeFile.moc"
