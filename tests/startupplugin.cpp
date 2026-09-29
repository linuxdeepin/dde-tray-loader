// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginsiteminterface.h"

#include <cstdlib>

static void checkSessionEnvironment()
{
    if (qgetenv("WAYLAND_DISPLAY") != "tray-test-outer"
        || qgetenv("QT_WAYLAND_SHELL_INTEGRATION") != "tray-test-shell"
        || qgetenv("DSG_APP_ID") != "tray-test-app"
        || qgetenv("QT_IM_MODULE") != "tray-test-im"
        || qEnvironmentVariableIsSet("QT_SCALE_FACTOR"))
        std::exit(44);
}

class StartupPlugin : public QObject, public PluginsItemInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ModuleInterface_iid)
    Q_INTERFACES(PluginsItemInterface)

public:
    const QString pluginName() const override
    {
        return QStringLiteral("startup-test-%1").arg(TEST_PLUGIN_STAGE);
    }

    QWidget *itemWidget(const QString &) override { return nullptr; }

    void init(PluginProxyInterface *) override
    {
        if (qgetenv("WAYLAND_DISPLAY") != "dockplugin"
            || qgetenv("QT_WAYLAND_SHELL_INTEGRATION") != "plugin-shell"
            || qgetenv("DSG_APP_ID") != "org.deepin.dde.tray-loader"
            || qgetenv("QT_IM_MODULE") != "wayland")
            std::exit(43);

        if (TEST_PLUGIN_STAGE == 1) {
            QTimer::singleShot(0, qApp, [] {
                checkSessionEnvironment();
                qApp->setProperty("firstPluginEventDispatched", true);
            });
        } else {
            // Exercise the real loader/plugin boundary: the first plugin's
            // queued work must run before the second plugin initializes.
            if (!qApp->property("firstPluginEventDispatched").toBool())
                std::exit(42);
            QTimer::singleShot(0, qApp, [] {
                checkSessionEnvironment();
                qApp->quit();
            });
        }
    }
};

#include "startupplugin.moc"
