// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "quickpluginitem.h"
#include "constants.h"
#include "plugin.h"

#include <QMouseEvent>
#include <QMenu>

class Q_DECL_HIDDEN EventFilter : public QObject
{
public:
    explicit EventFilter(PluginItem *target)
        : QObject (target)
        , m_target(target)
        , m_timer (new QTimer(this))
    {
        m_timer->setSingleShot(true);
        m_timer->setInterval(0); // handle event in next EventLoop
        QObject::connect(m_timer, &QTimer::timeout, this, [this] () {
            onAcceptedMouseEvent();
        });
    }

public:
    bool eventFilter(QObject *watched, QEvent *event)
    {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            break;
        default:
            return false;
        }

        if (watched == m_target) {
            m_accepedEvent = QEvent::None;
            if (m_timer->isActive())
                m_timer->stop();
        } else {
            const auto children = m_target->findChildren<QWidget *>();
            if (children.contains(qobject_cast<QWidget *>(watched))) {
                m_accepedEvent = event->type();
                m_timer->start();
            }
        }
        return false;
    }
private:
    void onAcceptedMouseEvent()
    {
        if (m_accepedEvent == QEvent::None)
            return;

        Q_EMIT m_target->recvMouseEvent(m_accepedEvent);
    }
    QTimer *m_timer = nullptr;
    PluginItem *m_target = nullptr;
    QEvent::Type m_accepedEvent = {QEvent::None};
};

QuickPluginItem::QuickPluginItem(PluginsItemInterface *pluginInterface, const QString &itemKey, QWidget *parent)
    : PluginItem(pluginInterface, itemKey, parent)
    , m_onDockAction(nullptr)
{
    qApp->installEventFilter(new EventFilter(this));
}

QWidget *QuickPluginItem::centralWidget()
{
    if (pluginsItemInterface()) {
        auto quickItem = pluginsItemInterface()->itemWidget(Dock::QUICK_ITEM_KEY);
        quickItem->setVisible(true);
        return quickItem;
    }
    return nullptr;
}

void QuickPluginItem::mouseReleaseEvent(QMouseEvent *e)
{
     if (e->button() == Qt::RightButton) {
        if (auto menu = pluginContextMenu()) {
            if (auto pluginPopup = Plugin::PluginPopup::get(menu->windowHandle())) {
                auto geometry = windowHandle()->geometry();
                const auto offset = e->pos();
                pluginPopup->setX(geometry.x() + offset.x());
                pluginPopup->setY(geometry.y() + offset.y());
                menu->exec();
            }
        }
        e->accept();
    }

    QWidget::mouseReleaseEvent(e);
}

QMenu *QuickPluginItem::pluginContextMenu()
{
    if (m_menu->isEmpty()) {
        initPluginMenu();

        int flags = pluginFlags();
        if (flags & Dock::Attribute_CanSetting) {
            m_onDockAction = new QAction();
            m_onDockAction->setEnabled(true);
            m_menu->addAction(m_onDockAction);
        }
    }

    if (m_onDockAction) {
        bool onDock = true; // TODO 要找到对应插件是否在任务栏上显示，1070通过dbus获取
        QString itemText = onDock ? tr("Remove from dock") : tr("Pin to dock");
        m_onDockAction->setText(itemText);
        m_onDockAction->setData(onDock ? Dock::unDockMenuItemId : Dock::dockMenuItemId);
    }

    m_menu->setAttribute(Qt::WA_TranslucentBackground, true);
    // FIXME: qt5integration drawMenuItemBackground will draw a background event is's transparent
    auto pa = m_menu->palette();
    pa.setColor(QPalette::ColorRole::Window, Qt::transparent);
    m_menu->setPalette(pa);
    m_menu->winId();

    auto geometry = windowHandle()->geometry();
    auto pluginPopup = Plugin::PluginPopup::get(m_menu->windowHandle());
    pluginPopup->setPluginId(pluginsItemInterface()->pluginName());
    pluginPopup->setItemKey(Dock::QUICK_ITEM_KEY);
    pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeMenu);
    m_menu->setFixedSize(m_menu->sizeHint());
    return m_menu;
}

QWidget *QuickPluginItem::pluginTooltip()
{
    return nullptr;
}

QString QuickPluginItem::itemKey() const
{
    return Dock::QUICK_ITEM_KEY;
}
