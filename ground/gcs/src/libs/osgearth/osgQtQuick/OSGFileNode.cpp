/**
 ******************************************************************************
 *
 * @file       OSGFileNode.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
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
        qDebug() << "OSGFileLoader::load - reading node file" << url.path();
        // qDebug() << "OSGFileLoader - load - currentContext" << QOpenGLContext::currentContext();
        osg::Node *node = osgDB::readNodeFile(url.path().toStdString());
        // qDebug() << "OSGFileLoader::load - reading node" << node << "took" << t.elapsed() << "ms";

        emit loaded(url, node);
    }

signals:
    void loaded(const QUrl & url, osg::Node *node);

private:
    QUrl url;
};

struct OSGFileNode::Hidden : public QObject {
    Q_OBJECT

private:
    OSGFileNode * const self;

public:
    Hidden(OSGFileNode *parent) : QObject(parent), self(parent), url(), async(false), optimizeMode(OptimizeMode::None) {}

    bool acceptSource(QUrl url)
    {
        // qDebug() << "OSGFileNode::acceptSource" << url;

        if (this->url == url) {
            return false;
        }

        this->url = url;

        if (url.isValid()) {
            realize();
        } else {
            qWarning() << "OSGFileNode::acceptNode - invalid url" << url;
            self->setNode(NULL);
        }

        return true;
    }

    QUrl url;
    bool async;
    OptimizeMode::Enum optimizeMode;

private:

    void asyncLoad(const QUrl &url)
    {
        OSGFileLoader *loader = new OSGFileLoader(url);

        connect(loader, &OSGFileLoader::loaded, this, &Hidden::onLoaded);
        connect(loader, &OSGFileLoader::finished, loader, &OSGFileLoader::deleteLater);
        loader->start();
    }

    void syncLoad(const QUrl &url)
    {
        OSGFileLoader loader(url);

        connect(&loader, &OSGFileLoader::loaded, this, &Hidden::onLoaded);
        loader.load();
    }

    void realize()
    {
        qDebug() << "OSGFileNode::realize";
        if (async) {
            asyncLoad(url);
        } else {
            syncLoad(url);
        }
    }

    bool acceptNode(osg::Node *node)
    {
        qDebug() << "OSGFileNode::acceptNode" << node;
        if (node && optimizeMode != OptimizeMode::None) {
            // qDebug() << "OSGFileNode::acceptNode - optimize" << node << optimizeMode;
            osgUtil::Optimizer optimizer;
            optimizer.optimize(node, osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS);
        }
        self->setNode(node);
        return true;
    }

private slots:
    void onLoaded(const QUrl &url, osg::Node *node)
    {
        acceptNode(node);
    }
};

OSGFileNode::OSGFileNode(QObject *parent) : OSGNode(parent), h(new Hidden(this))
{
    qDebug() << "OSGFileNode::OSGFileNode";
}

OSGFileNode::~OSGFileNode()
{
    qDebug() << "OSGFileNode::~OSGFileNode";
}

const QUrl OSGFileNode::source() const
{
    return h->url;
}

void OSGFileNode::setSource(const QUrl &url)
{
    qDebug() << "OSGFileNode::setSource" << url;
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
    // qDebug() << "OSGFileNode::setAsync" << async;
    if (h->async != async) {
        h->async = async;
        emit asyncChanged(async);
    }
}

OptimizeMode::Enum OSGFileNode::optimizeMode() const
{
    return h->optimizeMode;
}

void OSGFileNode::setOptimizeMode(OptimizeMode::Enum mode)
{
    // qDebug() << "OSGFileNode::setOptimizeMode" << mode;
    if (h->optimizeMode != mode) {
        h->optimizeMode = mode;
        emit optimizeModeChanged(optimizeMode());
    }
}
} // namespace osgQtQuick

#include "OSGFileNode.moc"
