// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pluginsiteminterface_v3.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QWindow>

class CardPluginItem : public QObject
{
public:
    explicit CardPluginItem(PluginsItemInterfaceV3 *pluginInterface,
                            const QString &itemKey,
                            QObject *parent = nullptr);
    ~CardPluginItem() override;

    bool init();
    void show();
    void hide();

    QWindow *window() const;
    void resize(const QSize &size);

private:
    PluginsItemInterfaceV3 *m_pluginInterface = nullptr;
    QString m_itemKey;
    QPointer<QWindow> m_window;
};
