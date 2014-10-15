#include "OSGNodeFile.hpp"

#include <osgDB/ReadFile>

#include <QUrl>
#include <QThread>

#include <QDebug>

namespace osgQtQuick {

class OSGFileLoader: public QThread
{
    Q_OBJECT

public:
    OSGFileLoader(const QUrl &url) : url(url) {
    }

    void run() {
        osg::Node *node = osgDB::readNodeFile(url.toString().toStdString());
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
    Hidden(OSGNodeFile *parent) : QObject(parent), pub(parent) {}

    void asyncLoad(const QUrl &url) {
        OSGFileLoader *loader = new OSGFileLoader(url);
        connect(loader, &OSGFileLoader::loaded, this, &Hidden::onLoaded);
        connect(loader, &OSGFileLoader::finished, loader, &QObject::deleteLater);
        loader->start();
    }

    QUrl url;
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
    h->asyncLoad(url);
//    if (h->url != url)
//    {
//        h->url = url;
//        setNode(osgDB::readNodeFile(url.toString().toStdString()));
//        emit sourceChanged(url);
//    }
}

} // namespace osgQtQuick

#include "OSGNodeFile.moc"
