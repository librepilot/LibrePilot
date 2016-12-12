/**
 ******************************************************************************
 *
 * @file       OSGNode.hpp
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

#ifndef _H_OSGQTQUICK_OSGNODE_H_
#define _H_OSGQTQUICK_OSGNODE_H_

#include "Export.hpp"

#include <QObject>
#include <QQmlParserStatus>

/**
 * Only update() methods are allowed to update the OSG scenegraph.
 * All other methods should call setDirty() which will later trigger an update.
 * Exceptions:
 * - node change events should be handled right away.
 *
 * Setting an OSGNode dirty will trigger the addition of a one time update callback.
 * This approach leads to some potential issues:
 * - if a child sets a parent dirty, the parent will be updated later on the next update traversal (i.e. before the next frame).
 *
 */

namespace osg {
class Node;
} // namespace osg

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGNode : public QObject, public QQmlParserStatus {
    Q_OBJECT
                        Q_INTERFACES(QQmlParserStatus)

public:
    explicit OSGNode(QObject *parent = 0);
    virtual ~OSGNode();

    osg::Node *node() const;
    void setNode(osg::Node *node);

signals:
    void nodeChanged(osg::Node *node) const;

protected:
    bool isDirty(int mask = 0xFFFF) const;
    void setDirty(int mask = 0xFFFF);
    void clearDirty();

    virtual osg::Node *createNode();
    virtual void updateNode();

    void emitNodeChanged();

    void classBegin();
    void componentComplete();

private:
    struct Hidden;
    Hidden *const h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGNODE_H_
