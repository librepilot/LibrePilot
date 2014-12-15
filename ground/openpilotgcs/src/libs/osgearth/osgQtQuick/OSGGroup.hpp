#ifndef _H_OSGQTQUICK_OSGGROUP_H_
#define _H_OSGQTQUICK_OSGGROUP_H_

#include "Export.hpp"
#include "OSGNode.hpp"

#include <QQmlListProperty>

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGGroup : public OSGNode
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<osgQtQuick::OSGNode> children READ children)

    Q_CLASSINFO("DefaultProperty", "children")

public:
    explicit OSGGroup(QObject *parent = 0);
    virtual ~OSGGroup();
    
    QQmlListProperty<OSGNode> children();

private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGGROUP_H_
