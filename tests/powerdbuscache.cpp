// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Manual integration test; run on an isolated bus (see README).
#include "dbuspower.h"

#include <QCoreApplication>
#include <QDBusVirtualObject>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QProcess>
#include <QThread>
#include <QTimer>

#include <cstdio>
#include <functional>
#include <stdexcept>

static const QString service = QStringLiteral("org.deepin.dde.Power1");
static const QString path = QStringLiteral("/org/deepin/dde/Power1");

static void require(bool success, const char *message)
{
    if (!success)
        throw std::runtime_error(message);
}

static bool waitUntil(const std::function<bool()> &condition)
{
    QElapsedTimer timer;
    timer.start();
    while (!condition() && timer.elapsed() < 4000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    return condition();
}

static void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

class PowerService : public QDBusVirtualObject
{
public:
    PowerService(double percentage, int failures) : m_percentage(percentage), m_failures(failures) {}

    QString introspect(const QString &) const override
    {
        return QStringLiteral("<interface name='org.deepin.dde.Power1'>"
                              "<property name='OnBattery' type='b' access='read'/>"
                              "<property name='BatteryPercentage' type='a{sd}' access='read'/>"
                              "<property name='BatteryState' type='a{su}' access='read'/>"
                              "</interface>");
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &bus) override
    {
        if (message.interface() == QLatin1String("org.freedesktop.DBus.Properties")) {
            const auto values = properties();
            QList<QVariant> reply;
            if (message.member() == QLatin1String("GetAll")) {
                ++m_getAllCount;
                if (m_failures > 0) {
                    --m_failures;
                    // Report the same error as a timed-out request while keeping
                    // the bus name and all non-percentage properties unchanged.
                    bus.send(message.createErrorReply(QDBusError::NoReply, "Transient startup failure"));
                    return true;
                }
                reply << values;
            } else if (message.member() == QLatin1String("Get")) {
                reply << QVariant::fromValue(QDBusVariant(values.value(message.arguments().at(1).toString())));
            } else {
                return false;
            }
            // A separate process keeps this delay realistic for synchronous callers.
            QTimer::singleShot(800, this, [this, bus, message, reply] {
                bus.send(message.createReply(reply));
                if (message.member() == QLatin1String("GetAll"))
                    ++m_getAllReplies;
            });
            return true;
        }
        if (message.member() == QLatin1String("GetAllCount")) {
            bus.send(message.createReply(QVariantList{m_getAllCount}));
            return true;
        } else if (message.member() == QLatin1String("GetAllReplies")) {
            bus.send(message.createReply(QVariantList{m_getAllReplies}));
            return true;
        } else if (message.member() == QLatin1String("PercentageOnly")) {
            m_percentage = 54;
            auto changed = QDBusMessage::createSignal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
            changed << service << QVariantMap{{"BatteryPercentage", properties().value("BatteryPercentage")}} << QStringList();
            bus.send(changed);
        } else if (message.member() == QLatin1String("Advance")) {
            m_percentage = 77;
            m_onBattery = false;
            m_state = 1;
            auto changed = QDBusMessage::createSignal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
            changed << service << properties() << QStringList();
            bus.send(changed);
        } else if (message.member() == QLatin1String("Invalidate")) {
            m_percentage = 66;
            auto changed = QDBusMessage::createSignal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
            changed << service << QVariantMap() << QStringList{QStringLiteral("BatteryPercentage")};
            bus.send(changed);
        } else if (message.member() == QLatin1String("Quit")) {
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        } else {
            return false;
        }
        bus.send(message.createReply());
        return true;
    }

private:
    QVariantMap properties() const
    {
        return {{QStringLiteral("OnBattery"), m_onBattery},
                {QStringLiteral("BatteryPercentage"), QVariant::fromValue(BatteryPercentageMap{{"Display", m_percentage}})},
                {QStringLiteral("BatteryState"), QVariant::fromValue(BatteryStateMap{{"Display", m_state}})}};
    }

    double m_percentage;
    int m_failures;
    int m_getAllCount = 0;
    int m_getAllReplies = 0;
    bool m_onBattery = true;
    quint32 m_state = 2;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    qDBusRegisterMetaType<BatteryPercentageMap>();
    qDBusRegisterMetaType<BatteryStateMap>();
    auto bus = QDBusConnection::sessionBus();
    if (app.arguments().value(1) == QLatin1String("--service")) {
        PowerService fixture(app.arguments().value(2).toDouble(), app.arguments().value(3).toInt());
        if (!bus.registerVirtualObject(path, &fixture) || !bus.registerService(service))
            return 2;
        std::puts("READY");
        std::fflush(stdout);
        return app.exec();
    }

    QProcess fixture;
    try {
        const auto start = [&](int percentage, int failures = 0) {
            fixture.start(app.applicationFilePath(), {"--service", QString::number(percentage), QString::number(failures)});
            require(fixture.waitForStarted(1000) && fixture.waitForReadyRead(3000), "fixture did not start");
            require(fixture.readAllStandardOutput().contains("READY"), "fixture did not acquire its bus name");
            require(bus.interface()->isServiceRegistered(service).value(), "fixture service is not registered");
        };
        const auto command = [&](const QString &member) {
            const auto reply = bus.call(QDBusMessage::createMethodCall(service, path, service, member), QDBus::Block, 1000);
            require(reply.type() == QDBusMessage::ReplyMessage, "fixture command failed");
            return reply;
        };
        const auto percentage = [](const DBusPower &power) {
            return power.batteryPercentage().value(QStringLiteral("Display"), -1);
        };

        start(55, 2);
        {
            DBusPower recovering;
            require(waitUntil([&] { return command("GetAllCount").arguments().at(0).toInt() == 1; }),
                    "initial GetAll failure was not injected");
            command("PercentageOnly");
            require(waitUntil([&] { return percentage(recovering) == 54; }), "partial update was not received");
            require(!recovering.onBattery() && recovering.batteryState().isEmpty(),
                    "partial update unexpectedly supplied the missing properties");

            int ticks = 0;
            QTimer heartbeat;
            QObject::connect(&heartbeat, &QTimer::timeout, &recovering, [&] { ++ticks; });
            heartbeat.start(50);
            require(waitUntil([&] { return recovering.onBattery()
                                          && recovering.batteryState().value("Display") == 2; }),
                    "cache did not recover after failed GetAll without a service restart");
            require(ticks >= 10, "retries did not preserve event-loop progress");
            require(percentage(recovering) == 54, "retry lost the current percentage");
            require(command("GetAllCount").arguments().at(0).toInt() == 3, "unexpected retry count");
            processFor(1300);
            require(command("GetAllCount").arguments().at(0).toInt() == 3, "retry continued after success");
        }
        command("Quit");
        require(fixture.waitForFinished(1000), "fixture did not quit");

        // Destruction must cancel a scheduled retry.
        start(55, 100);
        {
            DBusPower cancelled;
            require(waitUntil([&] { return command("GetAllCount").arguments().at(0).toInt() == 1; }),
                    "pending-retry failure was not injected");
            processFor(100);
        }
        processFor(1300);
        require(command("GetAllCount").arguments().at(0).toInt() == 1, "retry survived proxy destruction");
        command("Quit");
        require(fixture.waitForFinished(1000), "fixture did not quit");

        // The fixture captures 55% at request time, then replies 800 ms later.
        // Live updates must survive the old reply, including charging status.
        start(55);
        {
            DBusPower ordered;
            require(waitUntil([&] { return command("GetAllCount").arguments().at(0).toInt() == 1; }),
                    "snapshot was not requested");
            command("Advance");
            require(waitUntil([&] { return percentage(ordered) == 77 && !ordered.onBattery()
                                          && ordered.batteryState().value("Display") == 1; }),
                    "live update did not arrive before the snapshot");
            bool rolledBack = false;
            const auto check = [&] {
                rolledBack |= percentage(ordered) != 77 || ordered.onBattery()
                    || ordered.batteryState().value("Display") != 1;
            };
            QObject::connect(&ordered, &DBusPower::BatteryPercentageChanged, &ordered, check);
            QObject::connect(&ordered, &DBusPower::OnBatteryChanged, &ordered, check);
            QObject::connect(&ordered, &DBusPower::BatteryStateChanged, &ordered, check);
            require(waitUntil([&] { return command("GetAllReplies").arguments().at(0).toInt() == 1; }),
                    "delayed snapshot was not sent");
            processFor(100);
            require(!rolledBack && percentage(ordered) == 77 && !ordered.onBattery()
                    && ordered.batteryState().value("Display") == 1,
                    "delayed snapshot overwrote newer battery state");
            require(command("GetAllCount").arguments().at(0).toInt() == 1,
                    "live updates caused an unnecessary snapshot request");
        }
        command("Quit");
        require(fixture.waitForFinished(1000), "fixture did not quit");

        // Only the percentage changes: the snapshot must still supply the
        // other properties. An invalidation in flight needs another snapshot.
        start(55);
        {
            DBusPower partial;
            require(waitUntil([&] { return command("GetAllCount").arguments().at(0).toInt() == 1; }),
                    "partial-update snapshot was not requested");
            command("PercentageOnly");
            require(waitUntil([&] { return percentage(partial) == 54; }), "partial update was not received");
            require(waitUntil([&] { return partial.onBattery() && partial.batteryState().value("Display") == 2; }),
                    "snapshot did not fill properties missing from the live update");
            require(percentage(partial) == 54, "snapshot overwrote the live percentage");
            partial.getAllProperties();
            require(waitUntil([&] { return command("GetAllCount").arguments().at(0).toInt() == 2; }),
                    "invalidation snapshot was not requested");
            command("Invalidate");
            require(waitUntil([&] { return percentage(partial) == 66; }),
                    "in-flight invalidation was lost");
            require(command("GetAllCount").arguments().at(0).toInt() == 3,
                    "in-flight invalidation did not coalesce into one follow-up snapshot");
        }
        command("Quit");
        require(fixture.waitForFinished(1000), "fixture did not quit");

        // Also exercise the service appearing after a proxy was constructed.
        DBusPower lateService;
        start(55);
        QElapsedTimer elapsed;
        elapsed.start();
        DBusPower power;
        require(power.batteryPercentage().isEmpty() && power.batteryState().isEmpty() && !power.onBattery(),
                "initial reads should return empty cached values");
        require(elapsed.elapsed() < 500, "property getters waited for the delayed service");

        int changes = 0;
        QObject::connect(&power, &DBusPower::BatteryPercentageChanged, &app, [&] { ++changes; });
        const bool ready = waitUntil([&] { return percentage(power) == 55 && percentage(lateService) == 55
                                                 && power.onBattery() && power.batteryState().value("Display") == 2; });
        if (!ready)
            qWarning() << percentage(power) << percentage(lateService) << power.onBattery() << power.batteryState()
                       << fixture.readAllStandardError();
        require(ready,
                "initial asynchronous snapshot or late service discovery failed");
        command("Advance");
        require(waitUntil([&] { return percentage(power) == 77 && !power.onBattery()
                                       && power.batteryState().value("Display") == 1 && changes >= 2; }),
                "property change did not update the cache and notify the UI");
        command("Invalidate");
        require(waitUntil([&] { return percentage(power) == 66; }), "invalidated property was not refreshed");

        // Restart while another asynchronous snapshot is pending.
        power.getAllProperties();
        command("Quit");
        require(fixture.waitForFinished(1000), "fixture did not quit");
        start(90);
        require(waitUntil([&] { return percentage(power) == 90 && percentage(lateService) == 90; }),
                "cache did not recover after service restart");
        command("Quit");
        require(fixture.waitForFinished(1000), "fixture did not quit");
        std::puts("PASS: snapshot ordering, partial updates, failed GetAll recovery, cancellation, invalidation and restart");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        fixture.kill();
        fixture.waitForFinished(1000);
        return 1;
    }
}
