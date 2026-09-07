// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGINSITEMINTERFACE_V3_H
#define PLUGINSITEMINTERFACE_V3_H

#include "pluginsiteminterface_v2.h"

#include <QString>
#include <QWindow>

#include <limits>

class PluginsItemInterfaceV3 : public PluginsItemInterfaceV2
{
public:
    /**
     * @brief Item key that should be exported as a card surface.
     *
     * The item key is still scoped by pluginName(), so the compositor can use
     * "pluginName::itemKey" as a stable surface id.
     */
    virtual QString cardItemKey() const { return {}; }

    /**
     * @brief Native window for the card item.
     *
     * The loader exposes this window to the dock compositor as a Wayland
     * surface. QML or QWidget based UI should be handled by plugin itself.
     * The plugin keeps ownership of the returned window.
     */
    virtual QWindow *cardWindow() const { return nullptr; }

    /**
     * @brief Sort order of the card surface.
     *
     * Cards are sorted by this value in ascending order, so the card with the
     * smallest value is shown first.  Cards sharing a value keep the order
     * their surfaces were created in.  The dock attaches no meaning to the
     * concrete numbers and does not know any plugin id, which keeps the card
     * area open to plugins shipped separately from the dock.
     *
     * Plugins are expected to take the value from their own configuration
     * rather than hard-coding it, so that it stays adjustable per product.
     * The default places the card after every card that specifies an order.
     */
    virtual int cardOrder() const { return std::numeric_limits<int>::max(); }

    /**
     * @brief Context menu for the card surface.
     *
     * The returned value uses the same JSON format as itemContextMenu().
     * The default implementation keeps cards compatible with plugins that
     * already provide a menu for the corresponding item key.
     */
    virtual const QString cardContextMenu(const QString &itemKey)
    {
        return itemContextMenu(itemKey);
    }

    /**
     * @brief Tooltip shown when the card is hovered.
     *
     * The returned widget follows the same contract as itemTipsWidget(): the
     * plugin keeps ownership and may update the content on every call.  The
     * default implementation reuses the item tooltip, and the dock falls back
     * to pluginDisplayName() when neither is provided.
     */
    virtual QWidget *cardTipsWidget(const QString &itemKey)
    {
        return itemTipsWidget(itemKey);
    }

    /**
     * @brief Called when an item in the card context menu is activated.
     *
     * The default implementation forwards to invokedMenuItem() for backward
     * compatibility.
     */
    virtual void invokedCardMenuItem(const QString &itemKey,
                                     const QString &menuId,
                                     const bool checked)
    {
        invokedMenuItem(itemKey, menuId, checked);
    }
};

QT_BEGIN_NAMESPACE

#define ModuleInterface_iid_V3 "com.deepin.dock.PluginsItemInterface_V3"

Q_DECLARE_INTERFACE(PluginsItemInterfaceV3, ModuleInterface_iid_V3)
QT_END_NAMESPACE

#endif // PLUGINSITEMINTERFACE_V3_H
