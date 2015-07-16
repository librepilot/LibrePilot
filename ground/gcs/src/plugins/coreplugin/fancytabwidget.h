/**
 ******************************************************************************
 *
 * @file       fancytabwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup CorePlugin Core Plugin
 * @{
 * @brief The Core GCS plugin
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

#ifndef FANCYTABWIDGET_H
#define FANCYTABWIDGET_H

#include <QIcon>
#include <QWidget>

#include <QTimer>
#include <QPropertyAnimation>

QT_BEGIN_NAMESPACE
class QPainter;
class QStackedLayout;
class QStatusBar;
QT_END_NAMESPACE

namespace Core {
namespace Internal {
class FancyTab : public QObject {
    Q_OBJECT Q_PROPERTY(float fader READ fader WRITE setFader)
public:
    FancyTab(QWidget *tabbar) : enabled(false), tabbar(tabbar), m_fader(0)
    {
        animator.setPropertyName("fader");
        animator.setTargetObject(this);
    }
    float fader()
    {
        return m_fader;
    }
    void setFader(float value);

    void fadeIn();
    void fadeOut();

    QIcon icon;
    QString text;
    QString toolTip;
    bool enabled;

private:
    QPropertyAnimation animator;
    QWidget *tabbar;
    float m_fader;
};

class FancyTabBar : public QWidget {
    Q_OBJECT

public:
    FancyTabBar(QWidget *parent = 0);
    ~FancyTabBar();

    bool event(QEvent *event);

    void paintEvent(QPaintEvent *event);
    void paintTab(QPainter *painter, int tabIndex) const;
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    bool validIndex(int index) const
    {
        return index >= 0 && index < m_tabs.count();
    }

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void setTabEnabled(int index, bool enable);
    bool isTabEnabled(int index) const;

    void insertTab(int index, const QIcon &icon, const QString &label)
    {
        FancyTab *tab = new FancyTab(this);

        tab->icon = icon;
        tab->text = label;
        m_tabs.insert(index, tab);
        updateGeometry();
    }
    void setEnabled(int index, bool enabled);
    void removeTab(int index)
    {
        FancyTab *tab = m_tabs.takeAt(index);

        delete tab;
        updateGeometry();
    }
    void setCurrentIndex(int index);
    int currentIndex() const
    {
        return m_currentIndex;
    }

    void setTabToolTip(int index, QString toolTip)
    {
        m_tabs[index]->toolTip = toolTip;
    }
    QString tabToolTip(int index) const
    {
        return m_tabs.at(index)->toolTip;
    }

    QIcon tabIcon(int index) const
    {
        return m_tabs.at(index)->icon;
    }
    QString tabText(int index) const
    {
        return m_tabs.at(index)->text;
    }
    int count() const
    {
        return m_tabs.count();
    }
    QRect tabRect(int index) const;

signals:
    void currentChanged(int);

public slots:
    void emitCurrentIndex();

private:
    static const int m_rounding;
    static const int m_textPadding;
    QRect m_hoverRect;
    int m_hoverIndex;
    int m_currentIndex;
    QList<FancyTab *> m_tabs;
    QTimer m_triggerTimer;
    QSize tabSizeHint(bool minimum = false) const;
};

class FancyTabWidget : public QWidget {
    Q_OBJECT

public:
    FancyTabWidget(QWidget *parent = 0);

    void insertTab(int index, QWidget *tab, const QIcon &icon, const QString &label);
    void removeTab(int index);
    void setBackgroundBrush(const QBrush &brush);
    void addCornerWidget(QWidget *widget);
    void insertCornerWidget(int pos, QWidget *widget);
    int cornerWidgetCount() const;
    void setTabToolTip(int index, const QString &toolTip);

    void paintEvent(QPaintEvent *event);

    int currentIndex() const;
    QStatusBar *statusBar() const;

    void setTabEnabled(int index, bool enable);
    bool isTabEnabled(int index) const;

    bool isSelectionWidgetVisible() const;

signals:
    void currentAboutToShow(int index);
    void currentChanged(int index);

public slots:
    void setCurrentIndex(int index);
    void setSelectionWidgetVisible(bool visible);

private slots:
    void showWidget(int index);

private:
    FancyTabBar *m_tabBar;
    QWidget *m_cornerWidgetContainer;
    QStackedLayout *m_modesStack;
    QWidget *m_selectionWidget;
    QStatusBar *m_statusBar;
};
} // namespace Internal
} // namespace Core

#endif // FANCYTABWIDGET_H
