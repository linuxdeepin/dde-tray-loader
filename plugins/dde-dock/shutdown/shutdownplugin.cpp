// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "shutdownplugin.h"
#include "dbus/dbusaccount.h"
#include "utils.h"
#include "tipswidget.h"
#include "./dbus/dbuspowermanager.h"
#include "plugins-logging-category.h"

#include <DSysInfo>
#include <DDBusSender>

#include <QIcon>
#include <QSettings>

#define PLUGIN_STATE_KEY "enable"

const QString MENU_SHUTDOWN = "Shutdown";
const QString MENU_UPDATE_SHUTDOWN = "UpdateAndShutdown";
const QString MENU_REBOOT = "Reboot";
const QString MENU_UPDATE_REBOOT = "UpdateAndReboot";
const QString MENU_SUSPEND = "Suspend";
const QString MENU_HIBERNATE = "Hibernate";
const QString MENU_LOCK = "Lock";
const QString MENU_LOGOUT = "Logout";
const QString MENU_SWITCH_USER = "SwitchUser";
const QString MENU_POWER_SETTINGS = "PowerSettings";

Q_LOGGING_CATEGORY(SHUTDOWN, "org.deepin.dde.dock.shutdown")

// lastore-daemon状态
const int CAN_UPGRADE = 1 << 0;
const int DISABLE_UPDATE = 1 << 1;
const int FORCE_UPDATE = 1 << 2;

DCORE_USE_NAMESPACE
using namespace Dock;

ShutdownPlugin::ShutdownPlugin(QObject *parent)
    : QObject(parent)
    , m_pluginLoaded(false)
    , m_dockIcon(nullptr)
    , m_tipsLabel(new TipsWidget)
    , m_powerManagerInter(new DBusPowerManager("org.deepin.daemon.PowerManager1", "/org/deepin/daemon/PowerManager1", QDBusConnection::systemBus(), this))
    , m_dconfig(DConfig::create("org.deepin.dde.tray-loader", "org.deepin.dde.dock.plugin.shutdown", QString(), this))
    , m_lastoreDConfig(DConfig::create("org.deepin.lastore", "org.deepin.lastore", "", this))
{
    m_tipsLabel->setVisible(false);
    m_tipsLabel->setAccessibleName("shutdown");
}

const QString ShutdownPlugin::pluginName() const
{
    return "shutdown";
}

const QString ShutdownPlugin::pluginDisplayName() const
{
    return tr("Shut Down");
}

QWidget *ShutdownPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return m_dockIcon.data();
}

QWidget *ShutdownPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    // reset text every time to avoid size of LabelWidget not change after
    // font size be changed in ControlCenter
    m_tipsLabel->setText(tr("Shut Down"));

    return m_tipsLabel.data();
}

void ShutdownPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    // transfer config
    QSettings settings("deepin", "dde-dock-shutdown");
    if (QFile::exists(settings.fileName())) {
        QFile::remove(settings.fileName());
    }

    if (!pluginIsDisable()) {
        loadPlugin();
    }
}

void ShutdownPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, PLUGIN_STATE_KEY, !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool());

    refreshPluginItemsVisible();
}

bool ShutdownPlugin::pluginIsDisable()
{
    bool defaultValue = true;
    auto dconfig = Dtk::Core::DConfig::create("org.deepin.dde.tray-loader", "org.deepin.dde.dock.plugin.common", "");
    if (dconfig) {
        defaultValue = dconfig->value("defaultDockedPlugins", QStringList()).toStringList().contains(pluginName());
        dconfig->deleteLater();
    }
    return !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, defaultValue).toBool();
}

const QString ShutdownPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return REQUEST_SHUTDOWN;
}

const QString ShutdownPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    QStringList contextMenu = {
        MENU_SHUTDOWN,
        MENU_REBOOT,
        MENU_SUSPEND,
        MENU_HIBERNATE,
        MENU_LOCK,
        MENU_LOGOUT,
        MENU_SWITCH_USER,
        MENU_POWER_SETTINGS
    };

    if (m_dconfig.data()->isValid()) {
        contextMenu = m_dconfig->value("contextMenu", contextMenu).toStringList();
    }

    QList<QVariant> items;
    items.reserve(6);

#if 0 // v25逻辑已变，还未确定，暂时注释掉
    // 从配置文件中读取安装状态，第一位表示安装状态，1： 安装已就绪，0：安装未就绪
    if (m_lastoreDConfig && m_lastoreDConfig->isValid()) {
        bool ok;
        int value = m_lastoreDConfig->value("lastore-daemon-status", 0).toInt(&ok);
        qCInfo(SHUTDOWN) << "Lastore daemon status: " << value;
        if (ok) {
            switch (value & 0x07) { // 只处理3位
            case (CAN_UPGRADE):
                contextMenu.append(MENU_UPDATE_SHUTDOWN);
                contextMenu.append(MENU_UPDATE_REBOOT);
                break;
            case (CAN_UPGRADE | FORCE_UPDATE):
                contextMenu.removeOne(MENU_SHUTDOWN);
                contextMenu.removeOne(MENU_REBOOT);
                contextMenu.append(MENU_UPDATE_SHUTDOWN);
                contextMenu.append(MENU_UPDATE_REBOOT);
                break;
            default:
                break;
            }
        }
    }
#endif

    QMap<QString, QVariant> shutdown;
    if (contextMenu.contains(MENU_SHUTDOWN)) {
        shutdown["itemId"] = "Shutdown";
        shutdown["itemText"] = tr("Shut down");
        shutdown["isActive"] = true;
        items.push_back(shutdown);
    }

    if (contextMenu.contains(MENU_UPDATE_SHUTDOWN)) {
        QMap<QString, QVariant> map;
        map["itemId"] = "UpdateAndShutdown";
        map["itemText"] = tr("Update and Shut Down");
        map["isActive"] = true;
        map["showReminder"] = true;
        items.push_back(map);
    }

    if (contextMenu.contains(MENU_REBOOT)) {
        QMap<QString, QVariant> reboot;
        reboot["itemId"] = "Restart";
        reboot["itemText"] = tr("Reboot");
        reboot["isActive"] = true;
        items.push_back(reboot);
    }

    if (contextMenu.contains(MENU_UPDATE_REBOOT)) {
        QMap<QString, QVariant> map;
        map["itemId"] = "UpdateAndReboot";
        map["itemText"] = tr("Update and Reboot");
        map["isActive"] = true;
        map["showReminder"] = true;
        items.push_back(map);
    }

#ifndef DISABLE_POWER_OPTIONS

    QProcessEnvironment enviromentVar = QProcessEnvironment::systemEnvironment();
    bool can_sleep = enviromentVar.contains("POWER_CAN_SLEEP") ? QVariant(enviromentVar.value("POWER_CAN_SLEEP")).toBool()
                     : valueByQSettings<bool>("Power", "sleep", true) &&
                     m_powerManagerInter->CanSuspend();
    ;
    if (can_sleep) {
        QMap<QString, QVariant> suspend;
        if (contextMenu.contains(MENU_SUSPEND)) {
            suspend["itemId"] = "Suspend";
            suspend["itemText"] = tr("Suspend");
            suspend["isActive"] = true;
            items.push_back(suspend);
        }
    }

    bool can_hibernate = enviromentVar.contains("POWER_CAN_HIBERNATE") ?
        QVariant(enviromentVar.value("POWER_CAN_HIBERNATE")).toBool() : m_powerManagerInter->CanHibernate();

    if (can_hibernate) {
        QMap<QString, QVariant> hibernate;
        if (contextMenu.contains(MENU_HIBERNATE)) {
            hibernate["itemId"] = "Hibernate";
            hibernate["itemText"] = tr("Hibernate");
            hibernate["isActive"] = true;
            items.push_back(hibernate);
        }
    }

#endif

    QMap<QString, QVariant> lock;
    if (contextMenu.contains(MENU_LOCK)) {
        lock["itemId"] = "Lock";
        lock["itemText"] = tr("Lock");
        lock["isActive"] = true;
        items.push_back(lock);
    }

    if (contextMenu.contains(MENU_LOGOUT)) {
        QMap<QString, QVariant> logout;
        logout["itemId"] = "Logout";
        logout["itemText"] = tr("Log out");
        logout["isActive"] = true;
        items.push_back(logout);
    }

    if (!QFile::exists(ICBC_CONF_FILE)) {
        // 读取com.deepin.dde.session-shell切换用户配置项
        enum SwitchUserConfig {
            AlwaysShow = 0,
            OnDemand,
            Disabled
        } switchUserConfig = OnDemand;
#if 0 // TODO 等待v25 dde-session-shell重构添加配置为dconfig
        if (m_sessionShellGsettings && m_sessionShellGsettings->keys().contains("switchuser")) {
            switchUserConfig = SwitchUserConfig(m_sessionShellGsettings->get("switchuser").toInt());
        }
#endif
        // 和登录锁屏界面的逻辑保持一致
        if ((AlwaysShow == switchUserConfig ||
                 (OnDemand == switchUserConfig &&
                 (DBusAccount().userList().count() > 1 || DSysInfo::uosType() == DSysInfo::UosType::UosServer)))
                 && contextMenu.contains(MENU_SWITCH_USER)) {
            QMap<QString, QVariant> switchUser;
            switchUser["itemId"] = "SwitchUser";
            switchUser["itemText"] = tr("Switch user");
            switchUser["isActive"] = true;
            items.push_back(switchUser);
        }

#ifndef DISABLE_POWER_OPTIONS
        if (contextMenu.contains(MENU_POWER_SETTINGS)) {
            QMap<QString, QVariant> power;
            power["itemId"] = "power";
            power["itemText"] = tr("Power settings");
            power["isActive"] = true;
            items.push_back(power);
        }
#endif
    }

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

void ShutdownPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)

    // 使得下面逻辑代码延迟200ms执行，保证线程不阻塞
    QTime dieTime = QTime::currentTime().addMSecs(200);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 200);

    if (menuId == "power") {
        DDBusSender()
        .service("org.deepin.dde.ControlCenter1")
        .interface("org.deepin.dde.ControlCenter1")
        .path("/org/deepin/dde/ControlCenter1")
        .method(QString("ShowModule"))
        .arg(QString("power"))
        .call();
    } else if (menuId == "Lock") {
        if (QFile::exists(ICBC_CONF_FILE)) {
            QDBusMessage send = QDBusMessage::createMethodCall("org.deepin.dde.LockFront1", "/org/deepin/dde/LockFront1", "org.deepin.dde.LockFront1", "SwitchTTYAndShow");
            QDBusConnection conn = QDBusConnection::connectToBus("unix:path=/run/user/1000/bus", "unix:path=/run/user/1000/bus");
            QDBusMessage reply = conn.call(send);
#ifdef QT_DEBUG
            qCInfo(SHUTDOWN) << "Invoked menu item, call `SwitchTTYAndShow`, reply: " << reply;
#endif

        } else {
            QProcess::execute("bash -c \"originmap=$(setxkbmap -query | grep option | awk -F ' ' '{print $2}');/usr/bin/setxkbmap -option grab:break_actions&&/usr/bin/xdotool key XF86Ungrab&&dbus-send --print-reply --dest=org.deepin.dde.LockFront1 /org/deepin/dde/LockFront1 org.deepin.dde.LockFront1.Show&&setxkbmap -option $originmap\"");
        }
    } else
        DDBusSender()
        .service("org.deepin.dde.ShutdownFront1")
        .interface("org.deepin.dde.ShutdownFront1")
        .path("/org/deepin/dde/ShutdownFront1")
        .method(QString(menuId))
        .call();
}

void ShutdownPlugin::displayModeChanged(const Dock::DisplayMode displayMode)
{
    Q_UNUSED(displayMode);

    if (!pluginIsDisable() && !m_dockIcon.isNull()) {
        m_dockIcon->update();
    }
}

int ShutdownPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, 5).toInt();
}

void ShutdownPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

void ShutdownPlugin::pluginSettingsChanged()
{
    refreshPluginItemsVisible();
}

void ShutdownPlugin::loadPlugin()
{
    if (m_pluginLoaded) {
        qCDebug(SHUTDOWN) << "Shutdown plugin has been loaded";
        return;
    }

    m_pluginLoaded = true;

    m_dockIcon.reset(new CommonIconButton);
    m_dockIcon->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    m_dockIcon->setIcon("system-shutdown");

    m_proxyInter->itemAdded(this, pluginName());
    displayModeChanged(displayMode());
}

std::pair<bool, qint64> ShutdownPlugin::checkIsPartitionType(const QStringList &list)
{
    std::pair<bool, qint64> result{ false, -1 };

    if (list.length() != 5) {
        return result;
    }

    const QString type{ list[1] };
    const QString size{ list[2] };

    result.first  = type == "partition";
    result.second = size.toLongLong() * 1024.0f;

    return result;
}

qint64 ShutdownPlugin::get_power_image_size()
{
    qint64 size{ 0 };
    QFile  file("/sys/power/image_size");

    if (file.open(QIODevice::Text | QIODevice::ReadOnly)) {
        size = file.readAll().trimmed().toLongLong();
        file.close();
    }

    return size;
}

void ShutdownPlugin::refreshPluginItemsVisible()
{
    if (pluginIsDisable()) {
        m_proxyInter->itemRemoved(this, pluginName());
    } else {
        if (!m_pluginLoaded) {
            loadPlugin();
            return;
        }
        m_proxyInter->itemAdded(this, pluginName());
    }
}
