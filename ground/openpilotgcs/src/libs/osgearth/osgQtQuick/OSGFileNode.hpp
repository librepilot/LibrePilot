#ifndef _H_OSGQTQUICK_FILENODE_H_
#define _H_OSGQTQUICK_FILENODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGFileNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool async READ async WRITE setAsync NOTIFY asyncChanged)
    Q_PROPERTY(OptimizeMode optimizeMode READ optimizeMode WRITE setOptimizeMode NOTIFY optimizeModeChanged)

    Q_ENUMS(OptimizeMode)

public:
    enum OptimizeMode { None, Optimize, OptimizeAndCheck };

    OSGFileNode(QObject *parent = 0);
    virtual ~OSGFileNode();

    const QUrl source() const;
    void setSource(const QUrl &url);

    bool async() const;
    void setAsync(const bool async);

    OptimizeMode optimizeMode() const;
    void setOptimizeMode(OptimizeMode);

signals:
    void sourceChanged(const QUrl &url);
    void asyncChanged(const bool async);
    void optimizeModeChanged(OptimizeMode);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_FILENODE_H_
