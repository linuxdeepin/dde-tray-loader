// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cardpluginitem.h"
#include "dockcontextmenu.h"
#include "plugin.h"

#include <xdgactivation.h>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPalette>
#include <QTimer>
#include <QVBoxLayout>

namespace {
// Matches the hover delay of the tray items, see PluginItem.
constexpr int kToolTipDelay = 200;

void populateMenu(QMenu *menu, const QString &menuJson)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(menuJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "Invalid card context menu JSON:" << parseError.errorString();
        return;
    }

    const auto items = document.object().value(QStringLiteral("items")).toArray();
    for (const auto &item : items) {
        const auto object = item.toObject();
        auto *action = new QAction(object.value(QStringLiteral("itemText")).toString(), menu);
        action->setCheckable(object.value(QStringLiteral("isCheckable")).toBool());
        action->setChecked(object.value(QStringLiteral("checked")).toBool());
        action->setData(object.value(QStringLiteral("itemId")).toString());
        action->setEnabled(object.value(QStringLiteral("isActive")).toBool());
        action->setProperty("showReminder", object.value(QStringLiteral("showReminder")).toBool());
        menu->addAction(action);
    }
}
}

CardPluginItem::CardPluginItem(PluginsItemInterfaceV3 *pluginInterface,
                               const QString &itemKey,
                               QObject *parent)
    : QObject(parent)
    , m_pluginInterface(pluginInterface)
    , m_itemKey(itemKey)
    , m_tooltipTimer(new QTimer(this))
{
    m_tooltipTimer->setSingleShot(true);
    m_tooltipTimer->setInterval(kToolTipDelay);
    connect(m_tooltipTimer, &QTimer::timeout, this, &CardPluginItem::showToolTip);
}

CardPluginItem::~CardPluginItem()
{
    if (m_menu) {
        delete m_menu;
        m_menu = nullptr;
    }

    if (m_tipsContainer) {
        delete m_tipsContainer;
        m_tipsContainer = nullptr;
    }

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
    m_window->installEventFilter(this);
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
    closeToolTip();

    if (m_window) {
        m_window->hide();
    }
}

QWindow *CardPluginItem::window() const
{
    return m_window;
}

QString CardPluginItem::itemKey() const
{
    return m_itemKey;
}

void CardPluginItem::resize(const QSize &size)
{
    if (!m_window || !size.isValid() || size.isEmpty() || m_window->size() == size) {
        return;
    }

    m_window->resize(size);
}

bool CardPluginItem::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window) {
        switch (event->type()) {
        case QEvent::MouseButtonRelease: {
            auto *mouseEvent = dynamic_cast<QMouseEvent *>(event);
            if (mouseEvent && mouseEvent->button() == Qt::RightButton) {
                closeToolTip();
                if (showContextMenu(mouseEvent->position().toPoint())) {
                    event->accept();
                    return true;
                }
            }
            break;
        }
        case QEvent::Enter:
            m_tooltipTimer->start();
            break;
        case QEvent::Leave:
            closeToolTip();
            break;
        case QEvent::Hide:
            closeToolTip();
            break;
        default:
            break;
        }
    }

    return QObject::eventFilter(watched, event);
}

QWidget *CardPluginItem::tipsWidget()
{
    if (!m_pluginInterface) {
        return nullptr;
    }

    // The plugin owns the returned widget and may refresh its content on every
    // call, so it has to be queried again for each hover.
    QWidget *tips = m_pluginInterface->cardTipsWidget(m_itemKey);

    if (!tips) {
        // Fall back to the plugin name, so a card always has a tooltip even
        // when its plugin does not provide one.
        const QString name = m_pluginInterface->pluginDisplayName();
        if (name.isEmpty()) {
            return nullptr;
        }

        if (!m_defaultTipsLabel) {
            m_defaultTipsLabel = new QLabel;
        }
        qobject_cast<QLabel *>(m_defaultTipsLabel)->setText(name);
        tips = m_defaultTipsLabel;
    }

    if (!m_tipsContainer) {
        m_tipsContainer = new QWidget;
        auto *layout = new QVBoxLayout(m_tipsContainer);
        // Add the content margin here, a tooltip popup carries no padding.
        layout->setContentsMargins(8, 4, 8, 4);
        layout->setSizeConstraint(QLayout::SetFixedSize);
    }

    auto *layout = qobject_cast<QVBoxLayout *>(m_tipsContainer->layout());
    if (layout->indexOf(tips) < 0) {
        // The plugin may hand out a different widget than the previous hover,
        // e.g. when it switched between its own tooltip and our fallback.
        while (auto *item = layout->takeAt(0)) {
            if (auto *widget = item->widget()) {
                widget->setParent(nullptr);
                widget->hide();
            }
            delete item;
        }
        layout->addWidget(tips);
    }
    tips->setVisible(true);

    m_tipsContainer->setParent(nullptr);
    m_tipsContainer->setAttribute(Qt::WA_TranslucentBackground);
    m_tipsContainer->winId();

    auto *pluginPopup = Plugin::PluginPopup::get(m_tipsContainer->windowHandle());
    if (!pluginPopup) {
        return nullptr;
    }

    pluginPopup->setPluginId(m_pluginInterface->pluginName());
    pluginPopup->setItemKey(m_itemKey);
    pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeTooltip);
    return m_tipsContainer;
}

void CardPluginItem::showToolTip()
{
    if (!m_window || !m_window->isVisible()) {
        return;
    }

    // A menu opened from the card takes precedence over the tooltip.
    if (m_menu && m_menu->isVisible()) {
        return;
    }

    auto *tips = tipsWidget();
    if (!tips) {
        return;
    }

    auto *plugin = Plugin::EmbedPlugin::getWithoutCreating(m_window);
    if (!plugin) {
        return;
    }

    auto *pluginPopup = Plugin::PluginPopup::get(tips->windowHandle());
    if (!pluginPopup) {
        return;
    }

    // Anchor at the top center of the card, the dock places the tooltip above it.
    const auto position = plugin->pluginPos();
    pluginPopup->setX(position.x() + m_window->width() / 2);
    pluginPopup->setY(position.y());

    tips->show();
}

void CardPluginItem::closeToolTip()
{
    if (m_tooltipTimer->isActive()) {
        m_tooltipTimer->stop();
    }

    if (m_tipsContainer) {
        m_tipsContainer->hide();
    }
}

bool CardPluginItem::showContextMenu(const QPoint &position)
{
    if (!m_pluginInterface || !m_window) {
        return false;
    }

    if (m_menu) {
        m_menu->clear();
    } else {
        m_menu = new DockContextMenu;
        connect(m_menu, &QMenu::triggered, this, [this](QAction *action) {
            if (!m_pluginInterface) {
                return;
            }

            const QString menuId = action->data().toString();
            const bool checked = action->isCheckable() ? action->isChecked() : true;
            auto *activation = new tray::XdgActivation(this);
            connect(activation, &tray::XdgActivation::tokenReady, this,
                    [this, activation, menuId, checked](const QString &token) {
                if (!token.isEmpty()) {
                    qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
                }
                if (m_pluginInterface) {
                    m_pluginInterface->invokedCardMenuItem(m_itemKey, menuId, checked);
                }
                if (!token.isEmpty()) {
                    qunsetenv("XDG_ACTIVATION_TOKEN");
                }
                activation->deleteLater();
            }, Qt::SingleShotConnection);
            activation->requestToken();
        });
    }

    const QString menuJson = m_pluginInterface->cardContextMenu(m_itemKey);
    if (menuJson.isEmpty()) {
        return false;
    }
    populateMenu(m_menu, menuJson);
    if (m_menu->isEmpty()) {
        return false;
    }

    m_menu->setAttribute(Qt::WA_TranslucentBackground, true);
    auto palette = m_menu->palette();
    palette.setColor(QPalette::ColorRole::Window, Qt::transparent);
    m_menu->setPalette(palette);
    m_menu->winId();

    auto *plugin = Plugin::EmbedPlugin::get(m_window);
    if (!plugin) {
        return false;
    }
    auto *pluginPopup = Plugin::PluginPopup::get(m_menu->windowHandle());
    pluginPopup->setPluginId(m_pluginInterface->pluginName());
    pluginPopup->setItemKey(m_itemKey);
    pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeMenu);
    pluginPopup->setX(plugin->pluginPos().x() + position.x());
    pluginPopup->setY(plugin->pluginPos().y() + position.y());
    m_menu->setFixedSize(m_menu->sizeHint());
    m_menu->show();
    return true;
}
