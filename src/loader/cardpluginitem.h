// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pluginsiteminterface_v3.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QWindow>

class DockContextMenu;
class QTimer;
class QWidget;

class CardPluginItem : public QObject
{
    Q_OBJECT

public:
    explicit CardPluginItem(PluginsItemInterfaceV3 *pluginInterface,
                            const QString &itemKey,
                            QObject *parent = nullptr);
    ~CardPluginItem() override;

    bool init();
    void show();
    void hide();

    QString itemKey() const;
    QWindow *window() const;
    void resize(const QSize &size);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool showContextMenu(const QPoint &position);
    QWidget *tipsWidget();
    void showToolTip();
    void closeToolTip();

private:
    PluginsItemInterfaceV3 *m_pluginInterface = nullptr;
    QString m_itemKey;
    QPointer<QWindow> m_window;
    QPointer<DockContextMenu> m_menu;
    QTimer *m_tooltipTimer = nullptr;
    QPointer<QWidget> m_tipsContainer;
    QPointer<QWidget> m_defaultTipsLabel;
};
