#ifndef _H_OSGQTQUICK_MODELNODE_H_
#define _H_OSGQTQUICK_MODELNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QVector3D>
#include <QUrl>

namespace osgViewer {
class View;
}

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGModelNode : public OSGNode {
    Q_OBJECT
    // TODO rename to parentNode and modelNode
    Q_PROPERTY(osgQtQuick::OSGNode *modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)
    Q_PROPERTY(osgQtQuick::OSGNode * sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)

    Q_PROPERTY(bool clampToTerrain READ clampToTerrain WRITE setClampToTerrain NOTIFY clampToTerrainChanged)
    Q_PROPERTY(bool intoTerrain READ intoTerrain NOTIFY intoTerrainChanged)

    Q_PROPERTY(QVector3D attitude READ attitude WRITE setAttitude NOTIFY attitudeChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)

public:
    OSGModelNode(QObject *parent = 0);
    virtual ~OSGModelNode();

    OSGNode *modelData();
    void setModelData(OSGNode *node);

    OSGNode *sceneData();
    void setSceneData(OSGNode *node);

    bool clampToTerrain() const;
    void setClampToTerrain(bool arg);

    bool intoTerrain() const;

    QVector3D attitude() const;
    void setAttitude(QVector3D arg);

    QVector3D position() const;
    void setPosition(QVector3D arg);

    virtual bool attach(osgViewer::View *view);
    virtual bool detach(osgViewer::View *view);

signals:
    void modelDataChanged(OSGNode *node);
    void sceneDataChanged(OSGNode *node);

    void clampToTerrainChanged(bool arg);
    void intoTerrainChanged(bool arg);

    void attitudeChanged(QVector3D arg);
    void positionChanged(QVector3D arg);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_MODELNODE_H_
