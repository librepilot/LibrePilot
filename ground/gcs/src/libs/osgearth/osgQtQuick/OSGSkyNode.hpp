/**
 ******************************************************************************
 *
 * @file       OSGSkyNode.hpp
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

#ifndef _H_OSGQTQUICK_SKYNODE_H_
#define _H_OSGQTQUICK_SKYNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QDateTime>
#include <QUrl>

namespace osgViewer {
class View;
}

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGSkyNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(osgQtQuick::OSGNode *sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)

    Q_PROPERTY(bool sunLightEnabled READ sunLightEnabled WRITE setSunLightEnabled NOTIFY sunLightEnabledChanged)
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(double minimumAmbientLight READ minimumAmbientLight WRITE setMinimumAmbientLight NOTIFY minimumAmbientLightChanged)

public:
    OSGSkyNode(QObject *parent = 0);
    virtual ~OSGSkyNode();

    OSGNode *sceneData();
    void setSceneData(OSGNode *node);

    bool sunLightEnabled();
    void setSunLightEnabled(bool arg);

    QDateTime dateTime();
    void setDateTime(QDateTime arg);

    double minimumAmbientLight();
    void setMinimumAmbientLight(double arg);

    virtual bool attach(osgViewer::View *view);
    virtual bool detach(osgViewer::View *view);

signals:
    void sceneDataChanged(OSGNode *node);

    void sunLightEnabledChanged(bool arg);
    void dateTimeChanged(QDateTime arg);
    void minimumAmbientLightChanged(double arg);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_SKYNODE_H_
