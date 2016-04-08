/**
 ******************************************************************************
 *
 * @file       DirtySupport.hpp
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

#ifndef _H_OSGQTQUICK_DIRTYSUPPORT_H_
#define _H_OSGQTQUICK_DIRTYSUPPORT_H_

namespace osg {
class Node;
} // namespace osg

namespace osgQtQuick {
/**
 * Provides support to:
 * - manage dirty state flags
 * - add/remove node callback to trigger refresh (compatible with viewer on demand mode)
 */
class DirtySupport {
public:
    explicit DirtySupport();
    virtual ~DirtySupport();

protected:
    int dirty() const;
    bool isDirty(int mask = 0xFFFF) const;
    void setDirty(int mask = 0xFFFF);
    void clearDirty();

private:
    struct Hidden;
    struct NodeUpdateCallback;
    Hidden *const h;

    virtual osg::Node *nodeToUpdate() const = 0;

    virtual void update() = 0;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_DIRTYSUPPORT_H_
