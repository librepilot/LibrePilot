/**
 ******************************************************************************
 *
 * @file       OSGImageNode.hpp
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

#ifndef _H_OSGQTQUICK_IMAGENODE_H_
#define _H_OSGQTQUICK_IMAGENODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QUrl>

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGImageNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(QUrl imageFile READ imageFile WRITE setImageFile NOTIFY imageFileChanged)

    typedef OSGNode Inherited;

public:
    OSGImageNode(QObject *parent = 0);
    virtual ~OSGImageNode();

    const QUrl imageFile() const;
    void setImageFile(const QUrl &url);

signals:
    void imageFileChanged(const QUrl &url);

protected:
    virtual osg::Node *createNode();
    virtual void updateNode();

private:
    struct Hidden;
    Hidden *const h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_IMAGENODE_H_
