// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGINSITEM_H
#define PLUGINSITEM_H

#include "pluginsiteminterface_v2.h"
#include "dockdbusproxy.h"

#include <QWidget>

class QMenu;
class PluginItem : public QWidget
{
    Q_OBJECT

public:
    explicit PluginItem(PluginsItemInterface *pluginsItemInterface, const QString &itemKey, QWidget *parent = nullptr);
    ~PluginItem() override;

    void updateItemWidgetSize(const QSize &size);

    int pluginFlags() const;
    void setPluginFlags(int flags);
    void init();

    QString pluginId() const { return m_pluginsItemInterface->pluginName(); }
    virtual QString itemKey() const { return m_itemKey; }

signals:
    void recvMouseEvent(int type);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

    virtual QWidget *centralWidget();
    virtual QMenu *pluginContextMenu();
    virtual QWidget *pluginTooltip();

    PluginsItemInterface * pluginsItemInterface();
    void initPluginMenu();
    QWidget *itemTooltip(const QString &itemKey);
    bool executeCommand();

private:
    QWidget *itemPopupApplet();
    bool panelPopupExisted() const;

private:
    void updatePopupSize(const QRect &rect);

protected:
    QString m_itemKey;
    QMenu *m_menu;
    QScopedPointer<DockDBusProxy> m_dbusProxy;

private:
    PluginsItemInterface *m_pluginsItemInterface;
    PluginsItemInterfaceV2 *m_pluginsItemInterfacev2;
    QTimer* m_tooltipTimer;

    int m_pluginFlags = 0;
};

#endif // PLUGINSITEM_H

