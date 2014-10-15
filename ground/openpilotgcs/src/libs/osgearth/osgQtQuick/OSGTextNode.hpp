#ifndef _H_OSGQTQUICK_OSGTEXTNODE_H_
#define _H_OSGQTQUICK_OSGTEXTNODE_H_

#include <osgQtQuick/OSGNode>

#include <QColor>

namespace osgQtQuick {

class OSGQTQUICK_EXPORT OSGTextNode : public OSGNode
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit OSGTextNode(QObject *parent = 0);
    ~OSGTextNode();
    
    QString text() const;
    void setText(const QString &text);

    QColor color() const;
    void setColor(const QColor &color);

signals:
    void textChanged(const QString &text);
    void colorChanged(const QColor &color);
    
public slots:
    
private:
    struct Hidden;
    Hidden *h;
};

} // namespace osgQtQuick

#endif // _H_OSGQTQUICK_OSGTEXTNODE_H_
