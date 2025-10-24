// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "notification.h"
#include "constants.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QApplication>
#include <QIcon>
#include <QDBusInterface>
#include <QDBusReply>
#include <QtConcurrent/QtConcurrent>

#include <DStyle>
#include <DGuiApplicationHelper>
#include <DConfig>

Q_DECLARE_LOGGING_CATEGORY(qLcPluginNotification)

DWIDGET_USE_NAMESPACE;
DCORE_USE_NAMESPACE;
Notification::Notification(QWidget *parent)
    : QWidget(parent)
    , m_icon(QIcon::fromTheme("notification"))
    , m_notificationCount(0)
    , m_dbus(nullptr)
    , m_dndMode(false)
{
    setMinimumSize(PLUGIN_BACKGROUND_MIN_SIZE, PLUGIN_BACKGROUND_MIN_SIZE);
    connect(this, &Notification::dndModeChanged, this, &Notification::refreshIcon);
    connect(this, &Notification::notificationStatusChanged, this, &Notification::refreshIcon);
    connect(this, &Notification::notificationStatusChanged, this, &Notification::updateUnreadNotificationState);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &Notification::refreshIcon);
}

QIcon Notification::icon() const
{
    return m_icon;
}

void Notification::refreshIcon()
{
    //m_icon = QIcon::fromTheme(dndMode() ? "notification-off" : "notification");

    QString iconName;
    if (dndMode()) {
        iconName = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType ?
                ":/dsg/built-in-icons/notification-off-dark.svg" : ":/dsg/built-in-icons/notification-off.svg";
    } else {
        if (hasNewNotification()) {
            iconName = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType ?
                ":/dsg/built-in-icons/notification-unread-dark.svg" : ":/dsg/built-in-icons/notification-unread.svg";
        } else {
            iconName = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType ?
                ":/dsg/built-in-icons/notification-dark.svg" : ":/dsg/built-in-icons/notification.svg";
        }
    }

    m_icon = QIcon(iconName);
    Q_EMIT iconRefreshed();
}

bool Notification::dndMode() const
{
    return m_dndMode;
}

void Notification::setDndMode(bool dnd)
{
    if (m_dbus) {
        m_dbus->call(QLatin1String("SetSystemInfo"), QVariant::fromValue(0u), QVariant::fromValue(QDBusVariant(dnd)));
    }
}

uint Notification::notificationCount() const
{
    return m_notificationCount;
}

void Notification::watchNotification(bool newNotification)
{
    if (m_dndModeConfig)
        m_dndModeConfig->deleteLater();

    m_dndModeConfig = Dtk::Core::DConfig::create("org.deepin.dde.shell", "org.deepin.dde.shell.notification", QString(), this);
    if (!m_dndModeConfig->isValid()) {
        qCWarning(qLcPluginNotification) << "DndConfig is invalid.";
    }

    connect(m_dndModeConfig, &Dtk::Core::DConfig::valueChanged, this, [this] (const QString &key) {
        if (key == "dndMode") {
            updateDndModeState();
        }
    });

    auto ret = QtConcurrent::run([this, newNotification](){
        m_dbus.reset(new QDBusInterface("org.deepin.dde.Notification1", "/org/deepin/dde/Notification1", "org.deepin.dde.Notification1"));
        // Refresh icon for the first time, cause org.deepin.dde.Notification1 might depend on dock's DBus,
        // we should not call org.deepin.dde.Notification1 in the main thread before dock's dbus is initialized.
        // Just refresh icon in the other thread.
        updateDndModeState();

        Dtk::Core::DConfig config("org.deepin.dde.dock.plugin.notification");
        m_hasNewNotification = config.value("hasUnreadNotification", false).toBool();
        refreshIcon();

        auto recordCountVariant = m_dbus->property("recordCount");
        if (!recordCountVariant.isValid()) {
            qCWarning(qLcPluginNotification) << m_dbus->lastError();
        } else {
            setNotificationCount(recordCountVariant.toUInt());
        }

        const QString countChangedSlot = newNotification ?
                                             "RecordCountChanged" :
                                             "recordCountChanged";
        QDBusConnection::sessionBus().connect("org.deepin.dde.Notification1",
                                              "/org/deepin/dde/Notification1",
                                              "org.deepin.dde.Notification1",
                                              countChangedSlot,
                                              this,
                                              SLOT(setNotificationCount(uint))
                                              );

        if (newNotification) {
            QDBusConnection::sessionBus().connect("org.deepin.dde.Notification1",
                                                  "/org/deepin/dde/Notification1",
                                                  "org.deepin.dde.Notification1",
                                                  "NotificationStateChanged",
                                                  this,
                                                  SLOT(onNotificationStateChanged(qint64, int))
                                                  );
            auto ret = QDBusConnection::sessionBus().connect("org.deepin.dde.Widgets1",
                                                  "/org/deepin/dde/Widgets1",
                                                  "org.deepin.dde.Widgets1",
                                                  "VisibleChanged",
                                                  this,
                                                  SLOT(onNotificationCenterVisibleChanged(bool))
                                                  );
            if (!ret) {
                qWarning(qLcPluginNotification) << "The indicator of notification state changing with"
                                                   "notificationcenter's visibility doesn't work.";
            }
        } else {
            qDebug(qLcPluginNotification) << "The indicator of notification state doesn't work.";
        }
    });
    Q_UNUSED(ret)
}

void Notification::resetNotificationStatus()
{
    if (m_hasNewNotification == false && !m_notificationCenterVisible)
        return;

    m_hasNewNotification = false;
    Q_EMIT notificationStatusChanged();
}

bool Notification::hasNewNotification() const
{
    return m_hasNewNotification && m_notificationCount > 0;
}

void Notification::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QPainter p(this);
    m_icon.paint(&p, rect());
}

void Notification::setNotificationCount(uint count)
{
    if (m_notificationCount == count) {
        return;
    }
    m_notificationCount = count;
    Q_EMIT this->notificationCountChanged(count);
}

void Notification::onNotificationStateChanged(qint64 id, int processedType)
{
    Q_UNUSED(id)
    static const int Processed = 2;
    if (processedType == Processed) {
        if (!m_hasNewNotification && !m_notificationCenterVisible) {
            m_hasNewNotification = true;
            Q_EMIT notificationStatusChanged();
        }
    }
}

void Notification::updateDndModeState()
{
    m_dndMode = m_dndModeConfig->value("dndMode", false).toBool();
    Q_EMIT dndModeChanged(m_dndMode);
}

void Notification::onNotificationCenterVisibleChanged(bool visible)
{
    m_notificationCenterVisible = visible;
    resetNotificationStatus();
}

void Notification::updateUnreadNotificationState()
{
    Dtk::Core::DConfig config("org.deepin.dde.dock.plugin.notification");
    if (config.isValid()) {
        config.setValue("hasUnreadNotification", m_hasNewNotification);
    }
}
