// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sniprotocolhandler.h"
#include "abstracttrayprotocol.h"
#include "statusnotifieriteminterface.h"

#include "util.h"
#include "plugin.h"

#include "dbusmenuimporter.h"

#include <QMouseEvent>
#include <QWindow>
#include <QThreadPool>
#include <QRunnable>

#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

// Q_LOGGING_CATEGORY(sniTrayLod, "dde.shell.tray.sni")

namespace tray {
static QString sniPfrefix = QStringLiteral("SNI:");

class DBusMenu : public DBusMenuImporter {
public:
    DBusMenu(const QString &service, const QString &path, QObject *parent = 0)
        : DBusMenuImporter(service, path, parent)
    {
        QObject::connect(this, &DBusMenuImporter::menuUpdated, this, [this] (QMenu *menu) {
            menu->setFixedSize(menu->sizeHint());
        }, Qt::QueuedConnection);
    }
    virtual QMenu *createMenu(QWidget *parent) override
    {
        auto menu = DBusMenuImporter::createMenu(parent);
        menu->setAttribute(Qt::WA_TranslucentBackground);
        auto pa = menu->palette();
        pa.setColor(QPalette::ColorRole::Window, Qt::transparent);
        menu->setPalette(pa);
        return menu;
    }

    virtual QIcon iconForName(const QString &name) override
    {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            QIcon darkIcon = QIcon::fromTheme(name + "-dark");
            if (!darkIcon.isNull())
                return darkIcon;
        }
        return QIcon::fromTheme(name);
    }
};

SniTrayProtocol::SniTrayProtocol(QObject *parent)
    : AbstractTrayProtocol(parent)
    , m_trayManager(new OrgKdeStatusNotifierWatcherInterface("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher", QDBusConnection::sessionBus(), this))
{
    connect(m_trayManager, &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemRegistered, this, &SniTrayProtocol::registedItemChanged);
    connect(m_trayManager, &OrgKdeStatusNotifierWatcherInterface::StatusNotifierItemUnregistered, this, &SniTrayProtocol::registedItemChanged);
    QMetaObject::invokeMethod(this, &SniTrayProtocol::registedItemChanged, Qt::QueuedConnection);
}

SniTrayProtocol::~SniTrayProtocol()
{
    m_registedItem.clear();
}

void SniTrayProtocol::registedItemChanged()
{
    auto currentRegistedItems = m_trayManager->registeredStatusNotifierItems();

    if (currentRegistedItems == m_registedItem.keys()) {
        return;
    }

    // 处理新增的items：在子线程创建 Handler（init() 阻塞在此）
    for (const auto &currentRegistedItem : currentRegistedItems) {
        if (!m_registedItem.contains(currentRegistedItem) && !m_pendingItems.contains(currentRegistedItem)) {
            m_pendingItems.insert(currentRegistedItem);

            QThreadPool::globalInstance()->start([this, currentRegistedItem]() {
                auto pair = SniTrayProtocolHandler::serviceAndPath(currentRegistedItem);
                const uint pid = QDBusConnection::sessionBus().interface()->servicePid(pair.first).value();

                auto trayHandler = QSharedPointer<SniTrayProtocolHandler>(
                    new SniTrayProtocolHandler(currentRegistedItem));
                trayHandler->moveToThread(this->thread());

                QMetaObject::invokeMethod(this, [this, currentRegistedItem, pid, trayHandler]() {
                    if (!m_pendingItems.contains(currentRegistedItem)) {
                        return;  // item 已被取消，不添加
                    }
                    m_pendingItems.remove(currentRegistedItem);

                    m_item2Pid[currentRegistedItem] = pid;
                    if (registeredMap[pid] != SNI)
                        emit removeXEmbedItemByPid(pid);
                    registeredMap[pid] = SNI;
                    m_registedItem.insert(currentRegistedItem, trayHandler);
                    Q_EMIT AbstractTrayProtocol::trayCreated(trayHandler.get());
                }, Qt::QueuedConnection);
            });
        }
    }

    // 处理移除的items
    for (const auto &alreadyRegistedItem : m_registedItem.keys()) {
        if (!currentRegistedItems.contains(alreadyRegistedItem)) {
            if (auto value = m_registedItem.value(alreadyRegistedItem, nullptr)) {
                uint pid = m_item2Pid[alreadyRegistedItem];
                m_item2Pid.remove(alreadyRegistedItem);
                registeredMap.remove(pid);
                m_registedItem.remove(alreadyRegistedItem);
            }
        }
    }
    
    // 移除pending中但不在当前列表的items（取消异步创建）
    QSet<QString> toRemoveFromPending;
    for (const auto &pendingItem : m_pendingItems) {
        if (!currentRegistedItems.contains(pendingItem)) {
            toRemoveFromPending.insert(pendingItem);
        }
    }
    for (const auto &item : toRemoveFromPending) {
        m_pendingItems.remove(item);
    }
}

SniTrayProtocolHandler::SniTrayProtocolHandler(const QString &sniServicePath, QObject *parent)
    : AbstractTrayProtocolHandler(parent)
    , m_tooltip(nullptr)  // 延迟创建，在tooltip()方法中创建
    , m_ignoreFirstAttention(true)
    , m_dbusMenuImporter(nullptr)
{
    auto pair = serviceAndPath(sniServicePath);
    m_service = pair.first;
    // will get a unique dbus name (number like x.xxxx) and dbus path
    m_dbusUniqueName = pair.first.mid(1);
    m_sniInter = new StatusNotifierItem(pair.first, pair.second, QDBusConnection::sessionBus(), this);
    m_sniInter->setTimeout(2000);
    init();

    connect(m_sniInter, &StatusNotifierItem::NewIcon, this, &SniTrayProtocolHandler::iconChanged);
    connect(m_sniInter, &StatusNotifierItem::NewOverlayIcon, this, &SniTrayProtocolHandler::overlayIconChanged);
    connect(m_sniInter, &StatusNotifierItem::NewAttentionIcon, this, [this] {
        if (m_ignoreFirstAttention) {
            m_ignoreFirstAttention = false;
            return;
        }

        Q_EMIT attentionIconChanged();
    });

    connect(m_sniInter, &StatusNotifierItem::NewTitle, this, &SniTrayProtocolHandler::titleChanged);
    connect(m_sniInter, &StatusNotifierItem::NewStatus, this, &SniTrayProtocolHandler::statusChanged);
    connect(m_sniInter, &StatusNotifierItem::NewToolTip, this, &SniTrayProtocolHandler::tooltiChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this](DGuiApplicationHelper::ColorType themeType) {
        Q_UNUSED(themeType)
        menuImporter()->updateMenu(true);
    });
}

SniTrayProtocolHandler::~SniTrayProtocolHandler()
{
    UTIL->removeUniqueId(m_id);
}

void SniTrayProtocolHandler::init()
{
    generateId();
    m_menuPath = m_sniInter->menu().path();
}

void SniTrayProtocolHandler::generateId()
{
    auto id = sniPfrefix + m_sniInter->id();
    m_id = UTIL->generateUniqueId(id);
}

DBusMenuImporter *SniTrayProtocolHandler::menuImporter() const
{
    if (!m_dbusMenuImporter) {
        auto that = const_cast<SniTrayProtocolHandler *>(this);
        that->m_dbusMenuImporter = new DBusMenu(m_service, m_menuPath, that);
    }
    return m_dbusMenuImporter;
}

uint32_t SniTrayProtocolHandler::windowId() const
{
    return m_sniInter->windowId();
}

QString SniTrayProtocolHandler::id() const
{
    return m_id;
}
    
QString SniTrayProtocolHandler::title() const
{
    return m_sniInter->title();
}

QString SniTrayProtocolHandler::status() const
{
    return m_sniInter->status();
}

QWidget* SniTrayProtocolHandler::tooltip() const
{
    // 延迟创建label，确保在主线程创建
    if (!m_tooltip) {
        const_cast<SniTrayProtocolHandler*>(this)->m_tooltip = new QLabel();
        m_tooltip->setForegroundRole(QPalette::BrightText);
    }
    
    if (!m_sniInter->toolTip().title.isEmpty()) {
        m_tooltip->setText(m_sniInter->toolTip().title);
        return m_tooltip;
    }

    return nullptr;
}

QString SniTrayProtocolHandler::category() const
{
    return m_sniInter->category();
}

QIcon SniTrayProtocolHandler::overlayIcon() const
{
    auto iconName = m_sniInter->overlayIconName();
    if (!iconName.isEmpty()) {
        return QIcon::fromTheme(iconName);
    }

    auto icon = dbusImageList2QIcon(m_sniInter->overlayIconPixmap());
    return icon;
}

QIcon SniTrayProtocolHandler::attentionIcon() const
{
    auto iconName = m_sniInter->attentionIconName();
    if (!iconName.isEmpty()) {
        return QIcon::fromTheme(iconName);
    }

    auto icon = dbusImageList2QIcon(m_sniInter->attentionIconPixmap());
    return icon;
}

QIcon SniTrayProtocolHandler::icon() const
{
    const QString iconName = m_sniInter->iconName();
    const QString iconThemePath = m_sniInter->iconThemePath();
    QIcon resIcon;

    if (!iconName.isEmpty()) {
        resIcon = QIcon::fromTheme(iconName);
        if (!resIcon.isNull()) {
            return resIcon;
        }
    }

    if (!iconThemePath.isEmpty()) {
        resIcon = QIcon(iconThemePath + QDir::separator() + iconName);
        if (!resIcon.isNull()) {
            return resIcon;
        }
    }

    return dbusImageList2QIcon(m_sniInter->iconPixmap());
}

bool SniTrayProtocolHandler::enabled() const
{
    return m_sniInter->isValid();
}

bool SniTrayProtocolHandler::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == window()) {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_sniInter->Activate(0, 0);
            } else if (mouseEvent->button() == Qt::RightButton) {
                auto menu = menuImporter()->menu();
                Q_CHECK_PTR(menu);
                if (menu->isEmpty()) {
                    m_sniInter->ContextMenu(0, 0);
                    return false;
                }

                menu->setFixedSize(menu->sizeHint());
                menu->winId();

                auto widget = static_cast<QWidget*>(parent());
                auto plugin = Plugin::EmbedPlugin::get(widget->window()->windowHandle());
                auto geometry = plugin->pluginPos();
                auto pluginPopup = Plugin::PluginPopup::get(menu->windowHandle());
                pluginPopup->setPluginId("application-tray");
                pluginPopup->setItemKey(id());
                pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeMenu);
                const auto offset = mouseEvent->pos();
                pluginPopup->setX(geometry.x() + offset.x());
                pluginPopup->setY(geometry.y() + offset.y());
                menu->show();
            }
        }
    }

    return false;
}

QPair<QString, QString> SniTrayProtocolHandler::serviceAndPath(const QString &servicePath)
{
    QStringList list = servicePath.split("/");
    QPair<QString, QString> pair;
    pair.first = list.takeFirst();

    for (auto i : list) {
        pair.second.append("/");
        pair.second.append(i);
    }

    return pair;
}

QIcon SniTrayProtocolHandler::dbusImageList2QIcon(const DBusImageList &dbusImageList)
{
    QIcon res;
    if (!dbusImageList.isEmpty() && !dbusImageList.first().pixels.isEmpty()) {
        for (auto image = dbusImageList.begin(); image < dbusImageList.end(); image++) {
            const char *image_data = image->pixels.data();
            if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
                for (int i = 0; i < image->pixels.size(); i += 4) {
                    *(qint32 *)(image_data + i) = qFromBigEndian(*(qint32 *)(image_data + i));
                }
            }

            QImage qimage((const uchar *)image->pixels.constData(), image->width, image->height, QImage::Format_ARGB32);
            res.addPixmap(QPixmap::fromImage(qimage));
        }
    }

    return res;
}


}
