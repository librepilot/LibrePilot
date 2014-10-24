#ifndef _H_OSGQTQUICK_EARTHNODE_H_
#define _H_OSGQTQUICK_EARTHNODE_H_

#include "osgQtQuick/OSGNode.hpp"
#include "osgQtQuick/Export.hpp"

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGEarthNode : public OSGNode
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)

public:
    OSGEarthNode(QObject *parent = 0);
    ~OSGEarthNode();

    const QUrl source() const;
    void setSource(const QUrl &url);

signals:
    void sourceChanged(const QUrl &url);

private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_EARTHNODE_H_
