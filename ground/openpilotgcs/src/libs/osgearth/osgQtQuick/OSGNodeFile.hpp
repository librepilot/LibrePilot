#ifndef _H_OSGQTQUICK_NODEFILE_H_
#define _H_OSGQTQUICK_NODEFILE_H_

#include <osgQtQuick/OSGNode>

#include <QUrl>
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGNodeFile : public OSGNode
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)

public:
    OSGNodeFile(QObject *parent = 0);
    ~OSGNodeFile();

    const QUrl source() const;
    void setSource(const QUrl &url);

signals:
    void sourceChanged(const QUrl &url);

private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_NODEFILE_H_
