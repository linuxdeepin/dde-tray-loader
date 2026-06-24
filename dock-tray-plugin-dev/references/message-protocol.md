# 消息协议

本文档描述 dde-tray-loader 托盘插件与任务栏之间的消息通信协议。基于 `PluginsItemInterfaceV2` 的 `message()` 和 `setMessageCallback()` 接口，使用 JSON 格式字符串传输数据。

> **相关文档**：右键菜单协议请参考 `context-menu.md`；完整接口规范请参考 `tray-plugin-spec.md`。

## 1. 消息格式

所有消息均使用 JSON 格式，包含两个固定字段：

```json
{
    "msgType": "消息类型",
    "data": { ... }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `msgType` | `string` | 消息类型标识，对应常量 `Dock::MSG_TYPE` (`"msgType"`) |
| `data` | `object` | 消息数据，对应常量 `Dock::MSG_DATA` (`"data"`) |

## 2. MessageCallbackFunc

### 2.1 函数签名

```cpp
using MessageCallbackFunc = QString (*)(PluginsItemInterfaceV2 *, const QString&);
```

- **参数 1**：`PluginsItemInterfaceV2 *` — 当前插件实例指针
- **参数 2**：`const QString&` — 发送给任务栏的 JSON 消息
- **返回值**：`QString` — 任务栏返回的 JSON 数据

### 2.2 使用模式

插件在 `setMessageCallback` 中保存回调函数，后续通过此回调向任务栏发送请求：

```cpp
// 声明静态回调函数
static QString onMessage(PluginsItemInterfaceV2 *plugin, const QString &msg);

// 保存回调
void MyPlugin::setMessageCallback(MessageCallbackFunc cb) {
    m_messageCallback = cb;
}

// 使用回调发送消息
QString MyPlugin::sendMessage(const QString &msgType, const QJsonObject &data) {
    if (!m_messageCallback) return "{}";
    QJsonObject msg;
    msg["msgType"] = msgType;
    msg["data"] = data;
    return m_messageCallback(this, QJsonDocument(msg).toJson());
}
```

> **注意**：`MessageCallbackFunc` 是 C 函数指针，不是 `std::function`。不能使用 lambda 捕获，必须使用静态函数或全局函数。

## 3. 托盘插件常用消息类型

### 3.1 MSG_GET_SUPPORT_FLAG — 查询插件是否可用

插件向任务栏报告自身是否可用。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_GET_SUPPORT_FLAG` (`"getSupportFlag"`) |
| 方向 | 任务栏 → 插件（`message()` 中处理） |
| data | 无 |
| 返回 | `{ "supportFlag": true/false }` |

**插件处理示例**：

```cpp
QString MyPlugin::message(const QString &msg) {
    QJsonObject msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();
    QString msgType = msgObj["msgType"].toString();

    if (msgType == Dock::MSG_GET_SUPPORT_FLAG) {
        QJsonObject data;
        data["supportFlag"] = isAvailable();
        QJsonObject reply;
        reply["msgType"] = msgType;
        reply["data"] = data;
        return QJsonDocument(reply).toJson();
    }
    return "{}";
}
```

### 3.2 MSG_SUPPORT_FLAG_CHANGED — 插件可用状态变化通知

当插件可用状态变化时，主动通知任务栏。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_SUPPORT_FLAG_CHANGED` (`"supportFlagChanged"`) |
| 方向 | 插件 → 任务栏（通过 `MessageCallbackFunc` 发送） |

**发送示例**：

```cpp
void MyPlugin::notifySupportChanged(bool supported) {
    if (!m_messageCallback) return;
    QJsonObject data;
    data["supportFlag"] = supported;
    QJsonObject msg;
    msg["msgType"] = Dock::MSG_SUPPORT_FLAG_CHANGED;
    msg["data"] = data;
    m_messageCallback(this, QJsonDocument(msg).toJson());
}
```

### 3.3 MSG_ITEM_ACTIVE_STATE — 插件激活状态

插件向任务栏报告自身是否处于激活状态。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_ITEM_ACTIVE_STATE` (`"itemActiveState"`) |
| 方向 | 插件 → 任务栏 |
| data | `{ "itemActiveState": true/false }` |

- `true`：插件处于激活状态（如蓝牙已连接）
- `false`：插件处于失活状态

### 3.4 MSG_WHETHER_WANT_TO_BE_LOADED — 插件是否希望被加载

插件自行决定是否要被任务栏加载。不发送此消息则默认被加载。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_WHETHER_WANT_TO_BE_LOADED` (`"whetherWantToBeLoaded"`) |
| 方向 | 任务栏 → 插件（`message()` 中处理） |
| 返回 | `{ "whetherWantToBeLoaded": true/false }` |

- `true`：希望被加载
- `false`：不希望被加载

### 3.5 MSG_UPDATE_TOOLTIPS_VISIBLE — 更新 Tooltip 显隐

请求任务栏更新插件的 tooltip 显示状态。一般用于一个插件含多个图标、鼠标 hover 到不同图标时显示不同 tooltip 的场景。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_UPDATE_TOOLTIPS_VISIBLE` (`"updateTooltipsVisible"`) |
| 方向 | 插件 → 任务栏 |

### 3.6 MSG_UPDATE_OVERFLOW_STATE — 任务栏溢出状态

任务栏通知插件当前的溢出状态。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_UPDATE_OVERFLOW_STATE` (`"updateOverflowState"`) |
| 方向 | 任务栏 → 插件 |
| data | `{ "overflowState": 0/1/2 }` |

| 值 | 常量 | 说明 |
|----|------|------|
| 0 | `OVERFLOW_STATE_NOT_EXIST` | 没有溢出区 |
| 1 | `OVERFLOW_STATE_EXIST` | 有溢出区 |
| 2 | `OVERFLOW_STATE_ALL` | 所有应用都在溢出区 |

### 3.7 MSG_DOCK_PANEL_SIZE_CHANGED — 任务栏面板尺寸变化

任务栏通知插件面板尺寸发生了变化，插件可据此调整自身大小。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_DOCK_PANEL_SIZE_CHANGED` (`"dockPanelSizeChanged"`) |
| 方向 | 任务栏 → 插件 |

### 3.8 MSG_APPLET_CONTAINER — 弹窗显示位置

告知插件弹窗是在任务栏上直接显示还是在快捷面板的二级页面显示，插件可据此调整样式。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_APPLET_CONTAINER` (`"appletContainer"`) |
| 方向 | 任务栏 → 插件 |
| data | `{ "appletContainer": 0/1 }` |

| 值 | 常量 | 说明 |
|----|------|------|
| 0 | `APPLET_CONTAINER_DOCK` | 任务栏 |
| 1 | `APPLET_CONTAINER_QUICK_PANEL` | 快捷面板 |

### 3.9 MSG_PLUGIN_PROPERTY — 插件属性

任务栏获取插件属性，用于确定插件状态（如是否需要变色龙效果）。

| 字段 | 值 |
|------|----|
| 常量 | `Dock::MSG_PLUGIN_PROPERTY` (`"pluginProperty"`) |
| 方向 | 任务栏 → 插件 |
| 返回 | `QMap<QString, QVariant>` 转为 JSON |

#### 变色龙相关属性

| 属性键 | 说明 |
|--------|------|
| `PLUGIN_PROP_NEED_CHAMELEON` (`"needChameleon"`) | 是否需要变色龙效果（hover/press 样式），`true`/`false` |
| `PLUGIN_PROP_CHAMELEON_MARGIN` (`"chameleonMargin"`) | 变色龙边距，`QMargin` 值 |

## 4. 消息处理完整示例

以下是一个托盘插件处理消息的完整示例：

```cpp
// myplugin.h
class MyPlugin : public QObject, public PluginsItemInterfaceV2
{
    // ...
    void setMessageCallback(MessageCallbackFunc cb) override;
    QString message(const QString &msg) override;

private:
    MessageCallbackFunc m_messageCallback = nullptr;
};

// myplugin.cpp
void MyPlugin::setMessageCallback(MessageCallbackFunc cb) {
    m_messageCallback = cb;
}

QString MyPlugin::message(const QString &msg) {
    QJsonObject msgObj = QJsonDocument::fromJson(msg.toUtf8()).object();
    QString msgType = msgObj["msgType"].toString();

    if (msgType == Dock::MSG_GET_SUPPORT_FLAG) {
        return QStringLiteral("{\"msgType\":\"getSupportFlag\",\"data\":{\"supportFlag\":true}}");
    }

    if (msgType == Dock::MSG_WHETHER_WANT_TO_BE_LOADED) {
        return QStringLiteral("{\"msgType\":\"whetherWantToBeLoaded\",\"data\":{\"whetherWantToBeLoaded\":true}}");
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

// 主动通知状态变化
void MyPlugin::onAvailabilityChanged(bool available) {
    if (!m_messageCallback) return;
    QJsonObject data;
    data["supportFlag"] = available;
    QJsonObject msg;
    msg["msgType"] = Dock::MSG_SUPPORT_FLAG_CHANGED;
    msg["data"] = data;
    m_messageCallback(this, QJsonDocument(msg).toJson());
}

// 通知激活状态
void MyPlugin::onActiveStateChanged(bool active) {
    if (!m_messageCallback) return;
    QJsonObject data;
    data["itemActiveState"] = active;
    QJsonObject msg;
    msg["msgType"] = Dock::MSG_ITEM_ACTIVE_STATE;
    msg["data"] = data;
    m_messageCallback(this, QJsonDocument(msg).toJson());
}
```
