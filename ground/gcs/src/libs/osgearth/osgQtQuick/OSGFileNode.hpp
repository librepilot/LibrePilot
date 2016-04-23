/**
 ******************************************************************************
 *
 * @file       OSGFileNode.hpp
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

#ifndef _H_OSGQTQUICK_FILENODE_H_
#define _H_OSGQTQUICK_FILENODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OptimizeMode : public QObject {
    Q_OBJECT

public:
    enum Enum { None, Optimize, OptimizeAndCheck };
    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5
};

class OSGQTQUICK_EXPORT OSGFileNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool async READ async WRITE setAsync NOTIFY asyncChanged)
    Q_PROPERTY(osgQtQuick::OptimizeMode::Enum optimizeMode READ optimizeMode WRITE setOptimizeMode NOTIFY optimizeModeChanged)

    typedef OSGNode Inherited;

public:
    OSGFileNode(QObject *parent = 0);
    virtual ~OSGFileNode();

    const QUrl source() const;
    void setSource(const QUrl &url);

    bool async() const;
    void setAsync(const bool async);

    OptimizeMode::Enum optimizeMode() const;
    void setOptimizeMode(OptimizeMode::Enum);

signals:
    void sourceChanged(const QUrl &url);
    void asyncChanged(const bool async);
    void optimizeModeChanged(OptimizeMode::Enum);

protected:
    virtual osg::Node *createNode();
    virtual void updateNode();

private:
    struct Hidden;
    Hidden *const h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_FILENODE_H_
