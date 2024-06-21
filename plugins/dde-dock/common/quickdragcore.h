// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef QUICKDRAGCORE_H
#define QUICKDRAGCORE_H

#include "plugin-wrapper.h"

#include <QMimeData>
#include <QDrag>
#include <QPixmap>

#include <DPlatformWindowHandle>

class PluginsItemInterface;
class QTimer;

class QuickPluginMimeData : public QMimeData
{
    Q_OBJECT

public:
    explicit QuickPluginMimeData(PluginWrapper *item, QDrag *drag);
    ~QuickPluginMimeData();
    PluginWrapper *pluginWrapper() const;
    QDrag *drag() const;

private:
     PluginWrapper *m_item;
     QDrag *m_drag;
};

class QuickIconDrag : public QDrag
{
    Q_OBJECT

public:
    explicit QuickIconDrag(QObject *dragSource, const QPixmap &pixmap, int radius, bool enableBlurWindow);
    ~QuickIconDrag();
    void setDragHotPot(QPoint point);
    void setTargetPixmap(QPixmap pixmap);
    void setShowAnimation(bool show);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void activateDragWidget(int count = 10);

private:
    QPoint currentPoint() const;

    static int point2DockDist(QPoint currentP);
    static QPixmap roundPixmap(const QPixmap &origin, qreal xRadius, qreal yRadius, double percent = 1.0, bool setOpacity = true);

private Q_SLOTS:
    void onDragMove();

private:
    QWidget *m_imageWidget;
    QTimer *m_timer;
    QPixmap m_sourcePixmap;
    QPixmap m_currentPixmap;
    QPixmap m_targetPixmap;
    QPoint m_hotPoint;
    int m_originDist;
    int m_radius;
    double m_percent;
    bool m_showAnimation;
    Dtk::Widget::DPlatformWindowHandle *m_handle;
};

#endif // QUICKDRAGCORE_H
