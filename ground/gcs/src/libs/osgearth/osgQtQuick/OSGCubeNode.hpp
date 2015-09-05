/**
 ******************************************************************************
 *
 * @file       OSGCubeNode.hpp
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

#ifndef _H_OSGQTQUICK_CUBENODE_H_
#define _H_OSGQTQUICK_CUBENODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGCubeNode : public OSGNode {
    Q_OBJECT

public:
    OSGCubeNode(QObject *parent = 0);
    virtual ~OSGCubeNode();

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_CUBENODE_H_
