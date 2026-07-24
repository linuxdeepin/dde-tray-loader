// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cardpluginitem.h"

CardPluginItem::CardPluginItem(PluginsItemInterfaceV3 *pluginInterface,
                               const QString &itemKey,
                               QObject *parent)
    : QObject(parent)
    , m_pluginInterface(pluginInterface)
    , m_itemKey(itemKey)
{
}

CardPluginItem::~CardPluginItem()
{
    if (m_window) {
        m_window->hide();
    }

    m_window = nullptr;
}

bool CardPluginItem::init()
{
    if (!m_pluginInterface) {
        return false;
    }

    auto window = m_pluginInterface->cardWindow();
    if (!window) {
        return false;
    }

    m_window = window;
    m_window->setTitle(QStringLiteral("%1:%2-card").arg(m_pluginInterface->pluginName(), m_itemKey));
    m_window->setFlag(Qt::FramelessWindowHint, true);

    return true;
}

void CardPluginItem::show()
{
    if (m_window) {
        m_window->show();
    }
}

void CardPluginItem::hide()
{
    if (m_window) {
        m_window->hide();
    }
}

QWindow *CardPluginItem::window() const
{
    return m_window;
}

void CardPluginItem::resize(const QSize &size)
{
    if (!m_window || !size.isValid() || size.isEmpty() || m_window->size() == size) {
        return;
    }

    m_window->resize(size);
}
