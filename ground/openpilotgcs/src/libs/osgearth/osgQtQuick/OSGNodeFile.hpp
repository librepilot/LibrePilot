#ifndef _H_OSGQTQUICK_NODEFILE_H_
#define _H_OSGQTQUICK_NODEFILE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGNodeFile : public OSGNode {
    Q_OBJECT Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool async READ async WRITE setAsync NOTIFY asyncChanged)

public:
    OSGNodeFile(QObject *parent = 0);
    virtual ~OSGNodeFile();

    const QUrl source() const;
    void setSource(const QUrl &url);

    bool async() const;
    void setAsync(const bool async);

    virtual void realize();

signals:
    void sourceChanged(const QUrl &url);
    void asyncChanged(const bool async);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_NODEFILE_H_
