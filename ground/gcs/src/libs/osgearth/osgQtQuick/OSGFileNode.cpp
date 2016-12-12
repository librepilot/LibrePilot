/**
 ******************************************************************************
 *
 * @file       OSGFileNode.cpp
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

#include "OSGFileNode.hpp"

#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>

#include <QUrl>
#include <QOpenGLContext>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>

namespace osgQtQuick {
enum DirtyFlag { Source = 1 << 0, Async = 1 << 1, OptimizeMode = 1 << 2 };

class OSGFileLoader : public QThread {
    Q_OBJECT

public:
    OSGFileLoader(const QUrl &url) : url(url)
    {}

    void run()
    {
        osg::Node *node = load();
        emit loaded(url, node);
    }

    osg::Node *load()
    {
        QElapsedTimer t;

        t.start();
        // qDebug() << "OSGFileLoader::load - reading node file" << url.path();
        // qDebug() << "OSGFileLoader - load - currentContext" << QOpenGLContext::currentContext();
        osg::Node *node = osgDB::readNodeFile(url.path().toStdString());
        if (!node) {
            qWarning() << "OSGFileLoader::load - failed to load" << url.path();
        }
        // qDebug() << "OSGFileLoader::load - reading node" << node << "took" << t.elapsed() << "ms";
        return node;
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
    QUrl source;
    bool async;
    OptimizeMode::Enum optimizeMode;

    Hidden(OSGFileNode *self) : QObject(self), self(self), source(), async(false), optimizeMode(OptimizeMode::None)
    {}

    void updateSource()
    {
        // qDebug() << "OSGFileNode::updateNode" << source;
        if (!source.isValid()) {
            self->setNode(NULL);
            if (!source.isEmpty()) {
                qWarning() << "OSGFileNode::updateNode - invalid source" << source;
            }
        }
        if (false /*async*/) {
            // not supported yet
            // it is not clear if thread safety is insured...
            asyncLoad(source);
        } else {
            setNode(syncLoad(source));
        }
    }

private:
    osg::Node *syncLoad(const QUrl &url)
    {
        OSGFileLoader loader(url);

        return loader.load();
    }

    void asyncLoad(const QUrl &url)
    {
        OSGFileLoader *loader = new OSGFileLoader(url);

        connect(loader, &OSGFileLoader::loaded, this, &Hidden::onLoaded);
        connect(loader, &OSGFileLoader::finished, loader, &OSGFileLoader::deleteLater);
        loader->start();
    }

    void setNode(osg::Node *node)
    {
        // qDebug() << "OSGFileNode::setNode" << node;
        if (node && optimizeMode != OptimizeMode::None) {
            // qDebug() << "OSGFileNode::acceptNode - optimize" << node << optimizeMode;
            osgUtil::Optimizer optimizer;
            optimizer.optimize(node, osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS);
        }
        self->setNode(node);
    }

private slots:
    void onLoaded(const QUrl &url, osg::Node *node)
    {
        // called in async mode
        // question : is it thread safe to call setNode() ?
        // could calling setDirty help? is setDirty() thread safe ?
        setNode(node);
    }
};

/* class OSGFileNode */

OSGFileNode::OSGFileNode(QObject *parent) : Inherited(parent), h(new Hidden(this))
{}

OSGFileNode::~OSGFileNode()
{
    delete h;
}

const QUrl OSGFileNode::source() const
{
    return h->source;
}

void OSGFileNode::setSource(const QUrl &source)
{
    if (h->source != source) {
        h->source = source;
        setDirty(Source);
        emit sourceChanged(source);
    }
}

bool OSGFileNode::async() const
{
    return h->async;
}

void OSGFileNode::setAsync(const bool async)
{
    if (h->async != async) {
        h->async = async;
        setDirty(Async);
        emit asyncChanged(async);
    }
}

OptimizeMode::Enum OSGFileNode::optimizeMode() const
{
    return h->optimizeMode;
}

void OSGFileNode::setOptimizeMode(OptimizeMode::Enum optimizeMode)
{
    if (h->optimizeMode != optimizeMode) {
        h->optimizeMode = optimizeMode;
        setDirty(OptimizeMode);
        emit optimizeModeChanged(optimizeMode);
    }
}

osg::Node *OSGFileNode::createNode()
{
    // node is created later
    return NULL;
}

void OSGFileNode::updateNode()
{
    Inherited::updateNode();

    if (isDirty(Async)) {
        // do nothing...
    }
    if (isDirty(OptimizeMode)) {
        // TODO: trigger a node update ?
    }
    if (isDirty(Source)) {
        h->updateSource();
    }
}
} // namespace osgQtQuick

#include "OSGFileNode.moc"
