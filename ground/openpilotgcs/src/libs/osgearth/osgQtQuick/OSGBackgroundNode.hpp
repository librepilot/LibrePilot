#ifndef _H_OSGQTQUICK_BACKGROUNDNODE_H_
#define _H_OSGQTQUICK_BACKGROUNDNODE_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {
class OSGQTQUICK_EXPORT OSGBackgroundNode : public OSGNode {
    Q_OBJECT Q_PROPERTY(QUrl imageFile READ imageFile WRITE setImageFile NOTIFY imageFileChanged)

public:
    OSGBackgroundNode(QObject *parent = 0);
    virtual ~OSGBackgroundNode();

    const QUrl imageFile() const;
    void setImageFile(const QUrl &url);

    virtual void realize();

signals:
    void imageFileChanged(const QUrl &url);

private:
    struct Hidden;
    Hidden *h;
};
} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_BACKGROUNDNODE_H_
