#ifndef _H_OSGQTQUICK_OSGGROUP_H_
#define _H_OSGQTQUICK_OSGGROUP_H_

#include "osgQtQuick/OSGNode.hpp"

#include <QQmlListProperty>

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGGroup : public OSGNode
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<osgQtQuick::OSGNode> child READ child)

    Q_CLASSINFO("DefaultProperty", "child")

public:
    explicit OSGGroup(QObject *parent = 0);
    ~OSGGroup();
    
    QQmlListProperty<OSGNode> child();

signals:
    
public slots:
    
private:
    struct Hidden;
    friend struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGGROUP_H_
