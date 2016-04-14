/**
 ******************************************************************************
 *
 * @file       OSGCameraManipulator.hpp
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

#ifndef _H_OSGQTQUICK_OSGCAMERAMANIPULATOR_H_
#define _H_OSGQTQUICK_OSGCAMERAMANIPULATOR_H_

#include "../Export.hpp"

#include <QObject>
#include <QQmlParserStatus>

namespace osgGA {
class CameraManipulator;
}

namespace osgQtQuick {
class OSGNode;

class OSGQTQUICK_EXPORT OSGCameraManipulator : public QObject, public QQmlParserStatus {
    Q_OBJECT
                        Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(osgQtQuick::OSGNode * sceneNode READ sceneNode WRITE setSceneNode NOTIFY sceneNodeChanged)

public:
    explicit OSGCameraManipulator(QObject *parent = 0);
    virtual ~OSGCameraManipulator();

    osgGA::CameraManipulator *asCameraManipulator() const;

    OSGNode *sceneNode() const;
    void setSceneNode(OSGNode *node);

signals:
    void sceneNodeChanged(OSGNode *node);

protected:
    bool isDirty(int mask = 0xFFFF) const;
    void setDirty(int mask = 0xFFFF);
    void clearDirty();

    void classBegin();
    void componentComplete();

    osgGA::CameraManipulator *manipulator() const;
    void setManipulator(osgGA::CameraManipulator *manipulator);

protected:
    virtual void update();

private:
    struct Hidden;
    Hidden *const h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGCAMERAMANIPULATOR_H_
