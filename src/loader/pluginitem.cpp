// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "constants.h"
#include "pluginitem.h"
#include "plugin.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMenu>

const static QString DockQuickPlugins = "Dock_Quick_Plugins";

PluginItem::PluginItem(PluginsItemInterface *pluginItemInterface, const QString &itemKey, QWidget *parent)
    : QWidget(parent)
    , m_pluginsItemInterface(pluginItemInterface)
    , m_pluginsItemInterfacev2(dynamic_cast<PluginsItemInterfaceV2*>(pluginItemInterface))
    , m_itemKey(itemKey)
    , m_menu(new QMenu(this))
    , m_tooltipTimer(new QTimer(this))
{
    m_tooltipTimer->setSingleShot(true);
    m_tooltipTimer->setInterval(200);

    connect(m_menu, &QMenu::triggered, this, [this](QAction *action){
        QString actionStr = action->data().toString();
        if (actionStr == Dock::dockMenuItemId || actionStr == Dock::unDockMenuItemId) {
            m_dbusProxy->setItemOnDock(DockQuickPlugins, m_itemKey, actionStr == Dock::dockMenuItemId);
        } else {
            m_pluginsItemInterface->invokedMenuItem(m_itemKey, action->data().toString(), action->isCheckable() ? action->isChecked() : true);
        }
    });
}

PluginItem::~PluginItem() = default;

QWidget *PluginItem::itemPopupApplet()
{
    auto setPluginMsg = [this]  {
        auto pluginsItemInterfaceV2 = dynamic_cast<PluginsItemInterfaceV2 *>(m_pluginsItemInterface);
        if (!pluginsItemInterfaceV2)
            return;

        QJsonObject obj;
        obj[Dock::MSG_TYPE] = Dock::MSG_APPLET_CONTAINER;
        obj[Dock::MSG_DATA] = Dock::APPLET_CONTAINER_DOCK;

        QJsonDocument msg;
        msg.setObject(obj);

        pluginsItemInterfaceV2->message(msg.toJson());
    };

    if (auto popup = m_pluginsItemInterface->itemPopupApplet(m_itemKey)) {
        bool existed = panelPopupExisted() && !embedPanelPopupExisted();
        if (!existed && popup->isVisible()) {
            popup->windowHandle()->hide();
        }

        // hided, when clicked again
        if (existed) {
            popup->windowHandle()->hide();
            return nullptr;
        }

        setPluginMsg();
        popup->setParent(nullptr);
        popup->setAttribute(Qt::WA_TranslucentBackground);
        popup->winId();

        auto pluginPopup = Plugin::PluginPopup::get(popup->windowHandle());
        connect(pluginPopup, &Plugin::PluginPopup::eventGeometry, this, &PluginItem::updatePopupSize);

        pluginPopup->setPluginId(m_pluginsItemInterface->pluginName());
        pluginPopup->setItemKey(m_itemKey);
        pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypePanel);
        return popup;
    }
    return nullptr;
}

QMenu *PluginItem::pluginContextMenu()
{
    if (!m_menu->actions().isEmpty()) {
        m_menu->clear();
    }

    initPluginMenu();

    qDebug() << "mouseRightButtonClicked:" << m_itemKey << m_menu->actions().size();

    if (m_menu->isEmpty())
        return nullptr;

    m_menu->setAttribute(Qt::WA_TranslucentBackground, true);
    // FIXME: qt5integration drawMenuItemBackground will draw a background event is's transparent
    auto pa = this->palette();
    pa.setColor(QPalette::ColorRole::Window, Qt::transparent);
    m_menu->setPalette(pa);
    m_menu->winId();

    auto geometry = windowHandle()->geometry();
    auto pluginPopup = Plugin::PluginPopup::get(m_menu->windowHandle());
    pluginPopup->setPluginId(m_pluginsItemInterface->pluginName());
    pluginPopup->setItemKey(m_itemKey);
    pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeMenu);
    m_menu->setFixedSize(m_menu->sizeHint());
    return m_menu;
}

void PluginItem::mousePressEvent(QMouseEvent *e)
{
    closeToolTip();
    QWidget::mousePressEvent(e);
}

void PluginItem::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if (executeCommand())
            return;

        if (auto popup = itemPopupApplet()) {
            if (auto pluginPopup = Plugin::PluginPopup::get(popup->windowHandle())) {
                auto geometry = windowHandle()->geometry();
                const auto offset = QPoint(0, 0);
                pluginPopup->setX(geometry.x() + offset.x());
                pluginPopup->setY(geometry.y() + offset.y());
                popup->show();
            }
        }
    } else if (e->button() == Qt::RightButton) {
        if (auto menu = pluginContextMenu()) {
            if (auto pluginPopup = Plugin::PluginPopup::get(menu->windowHandle())) {
                auto geometry = windowHandle()->geometry();
                const auto offset = e->pos();
                pluginPopup->setX(geometry.x() + offset.x());
                pluginPopup->setY(geometry.y() + offset.y());
                menu->show();
            }
        }
    }
    QWidget::mouseReleaseEvent(e);
}

void PluginItem::enterEvent(QEvent *event)
{
    // panel popup existed, not show tooltip
    if (panelPopupExisted()) {
        return QWidget::enterEvent(event);
    }

    if (auto toolTip = pluginTooltip()) {
        if (auto pluginPopup = Plugin::PluginPopup::get(toolTip->windowHandle())) {
            auto geometry = windowHandle()->geometry();
            auto e = dynamic_cast<QEnterEvent *>(event);
            const auto offset = QPoint(0, 0);
            pluginPopup->setX(geometry.x() + offset.x());
            pluginPopup->setY(geometry.y() + offset.y());

            connect(m_tooltipTimer, &QTimer::timeout, toolTip, [this, toolTip] {
                toolTip->show(); 
                m_tooltipTimer->disconnect(toolTip);
            });
            m_tooltipTimer->start();
        }
    }

    QWidget::enterEvent(event);
}

void PluginItem::leaveEvent(QEvent *event)
{
    closeToolTip();
}

QWidget* PluginItem::centralWidget()
{
    auto trayItemWidget = m_pluginsItemInterface->itemWidget(m_itemKey);
    auto policy = m_pluginsItemInterface->pluginSizePolicy();
    if (policy == PluginsItemInterface::System) {
        trayItemWidget->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    }

    return trayItemWidget;
}

PluginsItemInterface * PluginItem::pluginsItemInterface()
{
    return m_pluginsItemInterface;
}

void PluginItem::updateItemWidgetSize(const QSize &size)
{
    centralWidget()->setFixedSize(size);
    update();
}

int PluginItem::pluginFlags() const
{
    return m_pluginFlags;
}

void PluginItem::setPluginFlags(int flags)
{
    m_pluginFlags = flags;
}

void PluginItem::updatePopupSize(const QRect &rect)
{
    if (auto popup = m_pluginsItemInterface->itemPopupApplet(m_itemKey)) {
        popup->setFixedSize(rect.size());
        popup->update();
    }
}

void PluginItem::init()
{
    winId();
    setAttribute(Qt::WA_TranslucentBackground);

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(centralWidget());
    hLayout->setMargin(0);
    hLayout->setSpacing(0);
    hLayout->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(hLayout);
}

void PluginItem::initPluginMenu()
{
    const QString menuJson = m_pluginsItemInterface->itemContextMenu(m_itemKey);
    if (menuJson.isEmpty()) {
        qWarning() << "itemContextMenu is empty!";
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(menuJson.toLocal8Bit().data());
    if (jsonDocument.isNull()) {
        qWarning() << "jsonDocument is null!";
        return;
    }

    QJsonObject jsonMenu = jsonDocument.object();

    QJsonArray jsonMenuItems = jsonMenu.value("items").toArray();
    for (auto item : jsonMenuItems) {
        QJsonObject itemObj = item.toObject();
        QAction *action = new QAction(itemObj.value("itemText").toString());
        action->setCheckable(itemObj.value("isCheckable").toBool());
        action->setChecked(itemObj.value("checked").toBool());
        action->setData(itemObj.value("itemId").toString());
        action->setEnabled(itemObj.value("isActive").toBool());
        m_menu->addAction(action);
    }
}

QWidget *PluginItem::pluginTooltip()
{
    auto popup = m_pluginsItemInterface->itemPopupApplet(m_itemKey);
    if (popup && popup->isVisible())
        popup->windowHandle()->hide();

    return itemTooltip(m_itemKey);
}

QWidget * PluginItem::itemTooltip(const QString &itemKey)
{
    auto toolTip = m_pluginsItemInterface->itemTipsWidget(itemKey);
    if (!toolTip) {
        qDebug() << "no tooltip";
        return nullptr;
    }

    toolTip->setParent(nullptr);
    toolTip->setAttribute(Qt::WA_TranslucentBackground);
    toolTip->winId();

    auto geometry = windowHandle()->geometry();
    auto pluginPopup = Plugin::PluginPopup::get(toolTip->windowHandle());
    pluginPopup->setPluginId(m_pluginsItemInterface->pluginName());
    pluginPopup->setItemKey(itemKey);
    pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeTooltip);
    if (!toolTip->sizeHint().isEmpty()) {
        toolTip->resize(toolTip->sizeHint());
    }
    return toolTip;
}

bool PluginItem::executeCommand()
{
    const QString command = m_pluginsItemInterface->itemCommand(m_itemKey);
    if (!command.isEmpty()) {
        qInfo() << "command: " << command;
        QStringList commandList = command.split(" ");
        QString program = commandList.takeFirst(); // 剩下是参数

        QProcess::startDetached(program, commandList);
        return true;
    }
    return false;
}

bool PluginItem::panelPopupExisted() const
{
    if (auto popup = m_pluginsItemInterface->itemPopupApplet(m_itemKey)) {
        if (Plugin::PluginPopup::contains(popup->windowHandle())) {
            return true;
        }
    }

    return false;
}

bool PluginItem::embedPanelPopupExisted() const
{
    if (panelPopupExisted())
        return false;

    if (auto popup = m_pluginsItemInterface->itemPopupApplet(m_itemKey)) {
        if(auto pluginPopup = Plugin::PluginPopup::getWithoutCreating(popup->windowHandle())) {
            return pluginPopup->popupType() == Plugin::PluginPopup::PopupTypeEmbed;
        }
    }
    return false;
}

void PluginItem::closeToolTip()
{
    if (m_tooltipTimer->isActive()) {
        m_tooltipTimer->stop();
    }

    auto tooltip = m_pluginsItemInterface->itemTipsWidget(m_itemKey);
    if (tooltip && tooltip->windowHandle() && tooltip->windowHandle()->isVisible())
        tooltip->windowHandle()->hide();
}

void PluginItem::updatePluginContentMargin(int spacing)
{
    if (spacing > 0) {
        setContentsMargins(spacing, spacing, spacing, spacing);
        setFixedSize(sizeHint());
    }
}
