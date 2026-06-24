# AI 使用指南：托盘插件代码生成

本指南面向 AI 助手，指导其根据用户的功能描述逐步生成 dde-tray-loader 托盘插件代码。每个章节对应一个接口/功能点，包含：接口说明 → 示例 prompt（中文）→ 代码骨架。

> **参考实现**：https://github.com/linuxdeepin/dde-tray-loader/tree/master/plugins/dde-dock/notification 下的 notification 插件是完整的托盘插件示例。

---

## 1. 创建插件骨架

### 接口说明

托盘插件类必须继承 `QObject` + `PluginsItemInterfaceV2`，使用 `Q_INTERFACES` 和 `Q_PLUGIN_METADATA` 宏注册。`flags()` 必须返回包含 `Type_Tray` 的标志。

### Prompt 示例

> 帮我创建一个名为 bluetooth 的托盘插件骨架

### 代码骨架

```cpp
// bluetoothplugin.h
#ifndef BLUETOOTHPLUGIN_H
#define BLUETOOTHPLUGIN_H

#include "pluginsiteminterface.h"
#include "pluginsiteminterface_v2.h"
#include <QObject>

class BluetoothPlugin : public QObject, public PluginsItemInterfaceV2
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV2)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "bluetooth.json")

public:
    explicit BluetoothPlugin(QObject *parent = nullptr);

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    Dock::PluginFlags flags() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;
};

#endif // BLUETOOTHPLUGIN_H
```

```cpp
// bluetoothplugin.cpp
#include "bluetoothplugin.h"

BluetoothPlugin::BluetoothPlugin(QObject *parent)
    : QObject(parent)
{
}

const QString BluetoothPlugin::pluginName() const
{
    return QStringLiteral("bluetooth");
}

const QString BluetoothPlugin::pluginDisplayName() const
{
    return tr("Bluetooth");
}

Dock::PluginFlags BluetoothPlugin::flags() const
{
    return Dock::PluginFlag::Type_Tray | Dock::PluginFlag::Attribute_CanSetting;
}

void BluetoothPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
}

QWidget *BluetoothPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return nullptr;
}
```

---

## 2. 实现插件初始化

### 接口说明

`init()` 是插件初始化入口，必须保存 `proxyInter` 到 `m_proxyInter`。推荐使用延迟加载模式（`m_pluginLoaded` 惯用法），仅在插件启用时才创建控件和建立连接。

### Prompt 示例

> 实现这个托盘插件的 init 函数，主控件显示蓝牙图标，使用延迟加载模式

### 代码骨架

```cpp
// bluetoothplugin.h 新增成员
private:
    void loadPlugin();
    void refreshPluginItemsVisible();

    bool m_pluginLoaded = false;
    QScopedPointer<QWidget> m_mainWidget;
```

```cpp
// bluetoothplugin.cpp
void BluetoothPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    if (!pluginIsDisable()) {
        loadPlugin();
    }
}

void BluetoothPlugin::loadPlugin()
{
    if (m_pluginLoaded) {
        return;
    }
    m_pluginLoaded = true;

    m_mainWidget.reset(new BluetoothWidget);  // Custom widget, see Ch3
    m_mainWidget->setFixedSize(Dock::TRAY_PLUGIN_ITEM_FIXED_SIZE);

    // Connect signals, init DBus, etc.
    // connect(...);

    m_proxyInter->itemAdded(this, pluginName());
}

void BluetoothPlugin::refreshPluginItemsVisible()
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
```

---

## 3. 实现主控件（itemWidget）

### 接口说明

`itemWidget()` 返回显示在任务栏托盘区的控件。托盘插件控件固定大小为 `TRAY_PLUGIN_ITEM_FIXED_SIZE (16x16)`。通常自定义 QWidget 子类，在 `paintEvent` 中绘制图标。

### Prompt 示例

> 插件的 itemWidget 显示一个 16x16 的蓝牙图标，根据连接状态变化，使用 paintEvent 绘制

### 代码骨架

```cpp
// bluetoothwidget.h
#ifndef BLUETOOTHWIDGET_H
#define BLUETOOTHWIDGET_H

#include <QWidget>
#include <QIcon>

class BluetoothWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BluetoothWidget(QWidget *parent = nullptr);
    void setIcon(const QIcon &icon);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    QIcon m_icon;
};

#endif // BLUETOOTHWIDGET_H
```

```cpp
// bluetoothwidget.cpp
#include "bluetoothwidget.h"
#include "constants.h"
#include <QPainter>

BluetoothWidget::BluetoothWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(Dock::TRAY_PLUGIN_ITEM_FIXED_SIZE);
}

void BluetoothWidget::setIcon(const QIcon &icon)
{
    m_icon = icon;
    update();
}

void BluetoothWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QPainter p(this);
    m_icon.paint(&p, rect());
}
```

```cpp
// bluetoothplugin.cpp 中更新 itemWidget 和 loadPlugin
QWidget *BluetoothPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return m_mainWidget.data();
}

// In loadPlugin():
m_mainWidget.reset(new BluetoothWidget);
m_mainWidget->setIcon(QIcon::fromTheme("bluetooth"));
```

---

## 4. 实现 Tooltip（itemTipsWidget）

### 接口说明

`itemTipsWidget()` 返回鼠标悬浮时显示的提示控件。推荐使用 `Dock::TipsWidget`，支持单行和多行文本。

> **TipsWidget 依赖说明**：`TipsWidget` 位于 `plugins/dde-dock/widgets/tipswidget.h`。使用时需在 CMakeLists.txt 中添加 `../widgets` 到 `include_directories` 并编译 `tipswidget.cpp`（参见 Ch11 的 CMakeLists.txt 示例）。也可自行实现简单的 QLabel 提示控件替代。

### Prompt 示例

> 鼠标悬停时显示蓝牙连接状态提示文字

### 代码骨架

```cpp
// bluetoothplugin.h 新增
#include "tipswidget.h"

public:
    QWidget *itemTipsWidget(const QString &itemKey) override;

private:
    QScopedPointer<Dock::TipsWidget> m_tipsLabel;
    void updateTipsText(bool connected, const QString &deviceName);
```

```cpp
// bluetoothplugin.cpp

// Init tips in constructor
BluetoothPlugin::BluetoothPlugin(QObject *parent)
    : QObject(parent)
    , m_pluginLoaded(false)
    , m_tipsLabel(new Dock::TipsWidget)
{
    m_tipsLabel->setText(tr("Bluetooth"));
    m_tipsLabel->setVisible(false);
}

QWidget *BluetoothPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return m_tipsLabel.data();
}

void BluetoothPlugin::updateTipsText(bool connected, const QString &deviceName)
{
    if (connected) {
        m_tipsLabel->setText(tr("Connected to %1").arg(deviceName));
    } else {
        m_tipsLabel->setText(tr("Not connected"));
    }
}
```

---

## 5. 实现弹窗面板（itemPopupApplet）

### 接口说明

`itemPopupApplet()` 返回左键点击后弹出的面板控件。可包含按钮、列表等交互元素。通过 `m_proxyInter->requestSetAppletVisible()` 控制面板显隐。

### Prompt 示例

> 点击蓝牙图标弹出蓝牙设备列表面板

### 代码骨架

```cpp
// bluetoothplugin.h 新增
public:
    QWidget *itemPopupApplet(const QString &itemKey) override;

private:
    QScopedPointer<QWidget> m_appletWidget;
```

```cpp
// bluetoothplugin.cpp

// 在 loadPlugin() 中创建 applet
m_appletWidget.reset(new QWidget);
m_appletWidget->setFixedWidth(Dock::DOCK_POPUP_WIDGET_WIDTH);
// Add layout and child widgets
// QVBoxLayout *layout = new QVBoxLayout(m_appletWidget.data());
// ...

QWidget *BluetoothPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return m_appletWidget.data();
}
```

> **注意**：如果同时实现了 `itemCommand`，则 `itemCommand` 优先，`itemPopupApplet` 不会被调用。

---

## 6. 实现点击命令（itemCommand）

### 接口说明

`itemCommand()` 返回左键点击时要执行的命令字符串。任务栏会直接执行该命令。返回空字符串则忽略。适用于简单的启动应用场景。

### Prompt 示例

> 左键点击打开蓝牙设置界面

### 代码骨架

```cpp
// bluetoothplugin.h 新增
public:
    const QString itemCommand(const QString &itemKey) override;
```

```cpp
// bluetoothplugin.cpp
const QString BluetoothPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return QString("dde-am org.deepin.dde.control-center -- bluetooth");
}
```

> **注意**：推荐使用 `dde-am` 打开应用，而非 `dbus-send`。在 Wayland 下需要 xdg-activation token 才能激活窗口，执行命令时框架已设置了所需环境变量，`dde-am` 会正确处理窗口激活。类似 `openControlCenterModule` 的方式：`dde-am org.deepin.dde.control-center -- bluetooth`。

> **注意**：如果同时实现了 `itemCommand` 和 `itemPopupApplet`，`itemCommand` 优先。

---

## 7. 实现右键菜单（itemContextMenu + invokedMenuItem）

### 接口说明

`itemContextMenu()` 返回 JSON 格式的右键菜单数据，`invokedMenuItem()` 处理菜单项点击。详细协议见 `context-menu.md`。

> **翻译说明**：涉及翻译的内容（如菜单文本、面板文字等），需要插件自行加载翻译文件处理，框架不处理翻译问题。使用 Qt 的翻译机制（`tr()` 函数 + `.ts`/`.qm` 文件）。

### Prompt 示例

> 右键菜单包含「打开蓝牙设置」和「开启/关闭蓝牙」两个选项

### 代码骨架

```cpp
// bluetoothplugin.h 新增
public:
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) override;
```

```cpp
// bluetoothplugin.cpp
#define BLUETOOTH_TOGGLE   "bluetooth-toggle"
#define BLUETOOTH_SETTINGS "bluetooth-settings"

const QString BluetoothPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    QList<QVariant> items;

    QMap<QString, QVariant> toggle;
    toggle["itemId"] = BLUETOOTH_TOGGLE;
    toggle["itemText"] = isBluetoothOn() ? tr("Turn off Bluetooth") : tr("Turn on Bluetooth");
    toggle["isCheckable"] = false;
    toggle["isActive"] = true;
    items.push_back(toggle);

    QMap<QString, QVariant> settings;
    settings["itemId"] = BLUETOOTH_SETTINGS;
    settings["itemText"] = tr("Bluetooth settings");
    settings["isCheckable"] = false;
    settings["isActive"] = true;
    items.push_back(settings);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

void BluetoothPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)

    if (menuId == BLUETOOTH_TOGGLE) {
        toggleBluetooth();
    } else if (menuId == BLUETOOTH_SETTINGS) {
        QStringList args {"--by-user", "org.deepin.dde.control-center", "--", "-p", "bluetooth"};
        QProcess::startDetached("dde-am", args);
    }
}
```

---

## 8. 实现 icon() 方法

### 接口说明

当 `flags()` 包含 `Attribute_CanSetting` 时，**必须**实现 `icon()` 方法。返回的图标显示在「控制中心 → 个性化 → 任务栏 → 插件区域」中。需根据 `ThemeType` 参数返回对应主题的图标。

### Prompt 示例

> 在控制中心-个性化-任务栏-插件区域显示蓝牙图标，支持亮暗色主题

### 代码骨架

```cpp
// bluetoothplugin.h 新增
public:
    QIcon icon(Dock::IconType dockPart, Dock::ThemeType themeType) const override;
```

```cpp
// bluetoothplugin.cpp
QIcon BluetoothPlugin::icon(Dock::IconType dockPart, Dock::ThemeType themeType) const
{
    Q_UNUSED(dockPart)

    if (themeType == Dock::ThemeType_Dark) {
        return QIcon(":/dsg/built-in-icons/bluetooth-dark.svg");
    } else {
        return QIcon(":/dsg/built-in-icons/bluetooth.svg");
    }
}
```

> **参考 notification 插件**：`icon()` 根据 `dockPart` 参数区分不同场景返回不同图标。当 `dockPart == DockPart::DCCSetting` 时返回控制中心展示图标，否则返回任务栏上的运行时图标。

> **图标使用主题图标**

> **控制中心图标安装**：当 `flags()` 包含 `Attribute_CanSetting` 时，还需安装图标文件（`.dci` 格式）到 `share/dde-dock/icons/dcc-setting/` 目录，供控制中心读取。参考 notification 插件的 CMakeLists.txt：
> ```cmake
> install(FILES "icons/dcc-notification.dci" DESTINATION share/dde-dock/icons/dcc-setting)
> ```

---

## 9. 实现消息通信（message + setMessageCallback）

### 接口说明

`setMessageCallback()` 保存任务栏传入的消息回调函数，`message()` 处理任务栏发来的请求。使用 JSON 格式通信。详见 `message-protocol.md`。

### Prompt 示例

> 插件需要监听蓝牙状态变化，通过消息协议与任务栏交互

### 代码骨架

```cpp
// bluetoothplugin.h 新增
public:
    void setMessageCallback(MessageCallbackFunc cb) override;
    QString message(const QString &msg) override;

private:
    MessageCallbackFunc m_messageCallback = nullptr;
    void notifyActiveState(bool active);
    void notifySupportFlag(bool supported);
```

```cpp
// bluetoothplugin.cpp
void BluetoothPlugin::setMessageCallback(MessageCallbackFunc cb)
{
    m_messageCallback = cb;
}

QString BluetoothPlugin::message(const QString &msg)
{
    QJsonObject msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();
    QString msgType = msgObj["msgType"].toString();

    if (msgType == Dock::MSG_GET_SUPPORT_FLAG) {
        QJsonObject data;
        data["supportFlag"] = isBluetoothAvailable();
        QJsonObject reply;
        reply["msgType"] = msgType;
        reply["data"] = data;
        return QJsonDocument(reply).toJson();
    }

    if (msgType == Dock::MSG_WHETHER_WANT_TO_BE_LOADED) {
        QJsonObject data;
        data["whetherWantToBeLoaded"] = true;
        QJsonObject reply;
        reply["msgType"] = msgType;
        reply["data"] = data;
        return QJsonDocument(reply).toJson();
    }

    if (msgType == Dock::MSG_PLUGIN_PROPERTY) {
        QJsonObject data;
        data[Dock::PLUGIN_PROP_NEED_CHAMELEON] = false;
        QJsonObject reply;
        reply["msgType"] = msgType;
        reply["data"] = data;
        return QJsonDocument(reply).toJson();
    }

    return "{}";
}

void BluetoothPlugin::notifyActiveState(bool active)
{
    if (!m_messageCallback) return;
    QJsonObject data;
    data["itemActiveState"] = active;
    QJsonObject msg;
    msg["msgType"] = Dock::MSG_ITEM_ACTIVE_STATE;
    msg["data"] = data;
    m_messageCallback(this, QJsonDocument(msg).toJson());
}

void BluetoothPlugin::notifySupportFlag(bool supported)
{
    if (!m_messageCallback) return;
    QJsonObject data;
    data["supportFlag"] = supported;
    QJsonObject msg;
    msg["msgType"] = Dock::MSG_SUPPORT_FLAG_CHANGED;
    msg["data"] = data;
    m_messageCallback(this, QJsonDocument(msg).toJson());
}
```

---

## 10. 编写 JSON 元数据

### 接口说明

每个插件必须包含一个 JSON 元数据文件，由 `Q_PLUGIN_METADATA(FILE "...")` 加载。

### Prompt 示例

> 生成这个插件的 JSON 元数据文件，依赖蓝牙 DBus 服务

### 代码骨架

```json
{
    "api": "2.0.0"
}
```

字段说明：
- `api`：**必填**，接口版本号，必须为 `"2.0.0"`

---

## 11. 编写 CMakeLists.txt

### 接口说明

插件构建为共享库（`.so`），安装到 `lib/dde-dock/plugins`。需链接 `dockpluginmanager-interface` target 和 DTK 库。

> **依赖说明**：插件依赖 `dde-tray-loader-dev` 包提供的接口头文件和 CMake 配置。安装方式：`sudo apt install dde-tray-loader-dev`。该包提供 `interfaces/` 目录下的头文件和 `dockpluginmanager-interface` CMake target。

### Prompt 示例

> 生成插件的 CMakeLists.txt，项目名为 bluetooth，依赖 Dtk6::Widget 和 Qt6::DBus

### 代码骨架

```cmake
set(PLUGIN_NAME "bluetooth")

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS DBus)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Gui)

add_library(${PLUGIN_NAME} SHARED
    bluetoothplugin.h
    bluetoothplugin.cpp
    bluetoothwidget.h
    bluetoothwidget.cpp
    bluetooth.qrc
    ../widgets/tipswidget.h
    ../widgets/tipswidget.cpp
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE QT_PLUGIN)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)

target_include_directories(${PLUGIN_NAME} PRIVATE
    "../../../interfaces"
    "../widgets"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    Dtk${DTK_VERSION_MAJOR}::Widget
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk${DTK_VERSION_MAJOR}::Gui
    dockpluginmanager-interface
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/plugins)
install(FILES "icons/dcc-bluetooth.dci" DESTINATION share/dde-dock/icons/dcc-setting)
```

---

## 12. 调试插件

### 接口说明

开发完成后需要调试插件。由于托盘插件由 `trayplugin-loader` 进程加载，而该进程由 dde-shell 启动，因此需要先找到并退出目标进程，再单独启动调试。

### 调试前提

- dde-shell 进程已启动并加载了 `org.deepin.ds.dock` 插件（如 systemd 中的 `dde-shell@DDE` 服务）

### 调试步骤

1. **查找插件进程**：

```bash
ps -aux | grep tray
```

找到加载了目标插件的 `trayplugin-loader` 进程及其 PID。

2. **退出目标进程**：

```bash
kill <PID>
```

3. **单独启动调试**：

```bash
/usr/libexec/trayplugin-loader -p /path/to/your/plugin.so --platform wayland
```

> **注意**：`-p` 参数指定插件的 `.so` 文件路径。调试时可直接在终端查看插件的日志输出（`qDebug` / `qWarning` 等）。

---

## 13. 完整示例：从描述到插件

本章节展示如何通过一系列 prompt 让 AI 生成一个完整的托盘插件。以 notification 插件为参考蓝本。

### 场景描述

> 我需要为 DDE 任务栏开发一个托盘插件，名为 "vpn"，显示 VPN 连接状态，支持点击打开 VPN 设置，右键菜单可以快速连接/断开，鼠标悬浮显示当前连接信息。

### Prompt 序列

**第 1 步：创建骨架**

> 帮我创建一个名为 vpn 的托盘插件骨架，需要继承 PluginsItemInterfaceV2

**第 2 步：实现初始化**

> 实现这个 VPN 托盘插件的 init 函数，主控件显示 VPN 图标，使用延迟加载模式

**第 3 步：实现主控件**

> 插件的 itemWidget 显示一个 16x16 的 VPN 图标，根据连接状态（已连接/未连接）切换图标，使用 paintEvent 绘制

**第 4 步：实现 Tooltip**

> 鼠标悬停时显示 VPN 连接状态提示文字，已连接时显示服务器名称，未连接时显示"未连接"

**第 5 步：实现右键菜单**

> 右键菜单包含「连接 VPN」和「VPN 设置」两个选项，连接状态时菜单文字变为「断开 VPN」

**第 6 步：实现 icon 方法**

> 在控制中心显示 VPN 图标，支持亮暗色主题

**第 7 步：实现消息通信**

> 插件需要通过消息协议报告 VPN 可用状态和激活状态

**第 8 步：生成构建文件**

> 生成这个插件的 JSON 元数据文件和 CMakeLists.txt

### 完整代码骨架

```cpp
// vpnplugin.h
#ifndef VPNPLUGIN_H
#define VPNPLUGIN_H

#include "pluginsiteminterface.h"
#include "pluginsiteminterface_v2.h"
#include "tipswidget.h"
#include <QObject>

class VpnWidget;

class VpnPlugin : public QObject, public PluginsItemInterfaceV2
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV2)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "vpn.json")

public:
    explicit VpnPlugin(QObject *parent = nullptr);

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    Dock::PluginFlags flags() const override;
    void init(PluginProxyInterface *proxyInter) override;

    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    const QString itemCommand(const QString &itemKey) override;
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) override;

    bool pluginIsAllowDisable() override;
    bool pluginIsDisable() override;
    void pluginStateSwitched() override;

    QIcon icon(Dock::IconType dockPart, Dock::ThemeType themeType) const override;

    void setMessageCallback(MessageCallbackFunc cb) override;
    QString message(const QString &msg) override;

private:
    void loadPlugin();
    void refreshPluginItemsVisible();
    void updateVpnState(bool connected, const QString &serverName = QString());

    bool m_pluginLoaded = false;
    QScopedPointer<VpnWidget> m_mainWidget;
    QScopedPointer<Dock::TipsWidget> m_tipsLabel;
    MessageCallbackFunc m_messageCallback = nullptr;
    bool m_vpnConnected = false;
    QString m_serverName;
};

#endif // VPNPLUGIN_H
```

```cpp
// vpnplugin.cpp
#include "vpnplugin.h"
#include "vpnwidget.h"
#include "constants.h"
#include <QProcess>
#include <QJsonDocument>

#define VPN_CONNECT    "vpn-connect"
#define VPN_DISCONNECT "vpn-disconnect"
#define VPN_SETTINGS   "vpn-settings"
#define PLUGIN_STATE_KEY "enable"

VpnPlugin::VpnPlugin(QObject *parent)
    : QObject(parent)
    , m_tipsLabel(new Dock::TipsWidget)
{
    m_tipsLabel->setText(tr("Not connected"));
    m_tipsLabel->setVisible(false);
}

const QString VpnPlugin::pluginName() const { return QStringLiteral("vpn"); }
const QString VpnPlugin::pluginDisplayName() const { return tr("VPN"); }
Dock::PluginFlags VpnPlugin::flags() const { return Dock::PluginFlag::Type_Tray | Dock::PluginFlag::Attribute_CanSetting; }
bool VpnPlugin::pluginIsAllowDisable() { return true; }
bool VpnPlugin::pluginIsDisable() { return !(m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool()); }

void VpnPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (!pluginIsDisable()) {
        loadPlugin();
    }
}

void VpnPlugin::loadPlugin()
{
    if (m_pluginLoaded) return;
    m_pluginLoaded = true;

    m_mainWidget.reset(new VpnWidget);
    // Connect DBus signals, init state, etc.
    m_proxyInter->itemAdded(this, pluginName());
}

void VpnPlugin::refreshPluginItemsVisible()
{
    if (pluginIsDisable()) {
        m_proxyInter->itemRemoved(this, pluginName());
    } else {
        if (!m_pluginLoaded) { loadPlugin(); return; }
        m_proxyInter->itemAdded(this, pluginName());
    }
}

void VpnPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, PLUGIN_STATE_KEY, pluginIsDisable());
    refreshPluginItemsVisible();
}

QWidget *VpnPlugin::itemWidget(const QString &itemKey) { Q_UNUSED(itemKey) return m_mainWidget.data(); }
QWidget *VpnPlugin::itemTipsWidget(const QString &itemKey) { Q_UNUSED(itemKey) return m_tipsLabel.data(); }

const QString VpnPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return QString("dde-am org.deepin.dde.control-center -- vpn");
}

const QString VpnPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    QList<QVariant> items;

    QMap<QString, QVariant> connectAction;
    connectAction["itemId"] = m_vpnConnected ? VPN_DISCONNECT : VPN_CONNECT;
    connectAction["itemText"] = m_vpnConnected ? tr("Disconnect VPN") : tr("Connect VPN");
    connectAction["isCheckable"] = false;
    connectAction["isActive"] = true;
    items.push_back(connectAction);

    QMap<QString, QVariant> settingsAction;
    settingsAction["itemId"] = VPN_SETTINGS;
    settingsAction["itemText"] = tr("VPN settings");
    settingsAction["isCheckable"] = false;
    settingsAction["isActive"] = true;
    items.push_back(settingsAction);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;
    return QJsonDocument::fromVariant(menu).toJson();
}

void VpnPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey) Q_UNUSED(checked)
    if (menuId == VPN_CONNECT || menuId == VPN_DISCONNECT) {
        // toggle VPN connection via DBus
    } else if (menuId == VPN_SETTINGS) {
        QStringList args {"--by-user", "org.deepin.dde.control-center", "--", "-p", "vpn"};
        QProcess::startDetached("dde-am", args);
    }
}

QIcon VpnPlugin::icon(Dock::IconType dockPart, Dock::ThemeType themeType) const
{
    Q_UNUSED(dockPart)
    if (themeType == Dock::ThemeType_Dark) {
        return QIcon(":/dsg/built-in-icons/vpn-dark.svg");
    }
    return QIcon(":/dsg/built-in-icons/vpn.svg");
}

void VpnPlugin::setMessageCallback(MessageCallbackFunc cb) { m_messageCallback = cb; }

QString VpnPlugin::message(const QString &msg)
{
    QJsonObject msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();
    QString msgType = msgObj["msgType"].toString();

    if (msgType == Dock::MSG_GET_SUPPORT_FLAG) {
        QJsonObject data;
        data["supportFlag"] = true;
        QJsonObject reply;
        reply["msgType"] = msgType;
        reply["data"] = data;
        return QJsonDocument(reply).toJson();
    }
    return "{}";
}

void VpnPlugin::updateVpnState(bool connected, const QString &serverName)
{
    m_vpnConnected = connected;
    m_serverName = serverName;
    if (connected) {
        m_tipsLabel->setText(tr("Connected to %1").arg(serverName));
    } else {
        m_tipsLabel->setText(tr("Not connected"));
    }
    // Notify dock of active state change
    if (m_messageCallback) {
        QJsonObject data;
        data["itemActiveState"] = connected;
        QJsonObject msg;
        msg["msgType"] = Dock::MSG_ITEM_ACTIVE_STATE;
        msg["data"] = data;
        m_messageCallback(this, QJsonDocument(msg).toJson());
    }
}
```

```json
{
    "api": "2.0.0"
}
```
