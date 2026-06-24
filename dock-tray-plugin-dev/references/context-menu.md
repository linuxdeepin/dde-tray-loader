# 右键菜单协议

本文档描述 dde-tray-loader 托盘插件的右键菜单实现协议。

> **相关文档**：消息通信协议请参考 `message-protocol.md`；完整接口规范请参考 `tray-plugin-spec.md`。

## 1. 概述

托盘插件的右键菜单通过两个接口配合实现：

1. **`itemContextMenu`**：返回菜单数据的 JSON 字符串
2. **`invokedMenuItem`**：菜单项被点击后的回调

## 2. 菜单数据格式

`itemContextMenu` 返回的 JSON 结构如下：

```json
{
    "items": [
        {
            "itemId": "menu-id-1",
            "itemText": "菜单项文本",
            "isCheckable": false,
            "isActive": true,
            "checked": false
        },
        {
            "itemId": "menu-id-2",
            "itemText": "可勾选项",
            "isCheckable": true,
            "isActive": true,
            "checked": true
        }
    ],
    "checkableMenu": false,
    "singleCheck": false
}
```

### 2.1 顶层字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `items` | `array` | 菜单项列表 |
| `checkableMenu` | `bool` | 整个菜单是否为可勾选菜单 |
| `singleCheck` | `bool` | 是否单选模式（仅一个菜单项可被选中） |

### 2.2 菜单项字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `itemId` | `string` | 菜单项唯一标识，用于 `invokedMenuItem` 中区分点击了哪个菜单项 |
| `itemText` | `string` | 菜单项显示文本 |
| `isCheckable` | `bool` | 该菜单项是否可勾选 |
| `isActive` | `bool` | 该菜单项是否激活（灰色不可点击为 `false`） |
| `checked` | `bool` | 可勾选项的当前选中状态（仅 `isCheckable` 为 `true` 时有效） |

## 3. 预定义菜单项 ID

任务栏保留了以下菜单项 ID，插件不应使用：

| 常量 | 值 | 说明 |
|------|----|------|
| `dockMenuItemId` | `"dock-item-id"` | 驻留任务栏 |
| `unDockMenuItemId` | `"undock-item-id"` | 移除驻留 |

## 4. 实现示例

### 4.1 构建菜单数据

参考 notification 插件的实现：

```cpp
const QString MyPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    QList<QVariant> items;

    // 菜单项 1：普通按钮
    QMap<QString, QVariant> action1;
    action1["itemId"] = "toggle-feature";
    action1["itemText"] = tr("Toggle feature");
    action1["isCheckable"] = false;
    action1["isActive"] = true;
    items.push_back(action1);

    // 菜单项 2：打开设置
    QMap<QString, QVariant> action2;
    action2["itemId"] = "open-settings";
    action2["itemText"] = tr("Settings");
    action2["isCheckable"] = false;
    action2["isActive"] = true;
    items.push_back(action2);

    // 菜单项 3：可勾选项
    QMap<QString, QVariant> toggleAction;
    toggleAction["itemId"] = "enable-mode";
    toggleAction["itemText"] = tr("Enable mode");
    toggleAction["isCheckable"] = true;
    toggleAction["isActive"] = true;
    toggleAction["checked"] = isModeEnabled();
    items.push_back(toggleAction);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}
```

### 4.2 处理菜单项点击

```cpp
void MyPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)

    if (menuId == "toggle-feature") {
        // 处理功能切换
        toggleFeature();
    } else if (menuId == "open-settings") {
        // 打开设置界面
        QStringList args {"--by-user", "org.deepin.dde.control-center", "--", "-p", "myplugin"};
        QProcess::startDetached("dde-am", args);
    } else if (menuId == "enable-mode") {
        // 处理可勾选项
        setModeEnabled(checked);
    }
}
```

## 5. 常见模式

### 5.1 动态菜单文本

菜单文本可以根据状态动态变化：

```cpp
const QString MyPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    QList<QVariant> items;

    QMap<QString, QVariant> toggleDnd;
    toggleDnd["itemId"] = "toggle-dnd";
    toggleDnd["itemText"] = dndMode() ? tr("Turn off DND mode") : tr("Turn on DND mode");
    toggleDnd["isCheckable"] = false;
    toggleDnd["isActive"] = true;
    items.push_back(toggleDnd);

    // ...
}
```

### 5.2 通过 dde-am 启动控制中心模块

```cpp
// 打开控制中心的指定模块
void MyPlugin::openControlCenterModule(const QString &module)
{
    QStringList args {"--by-user", "org.deepin.dde.control-center", "--", "-p", module};
    QProcess::startDetached("dde-am", args);
}
```

### 5.3 通过 dde-am 打开应用

> **注意**：推荐使用 `dde-am` 打开应用，而非 `dbus-send`。在 Wayland 下需要 xdg-activation token 才能激活窗口，执行命令时框架已设置了所需环境变量，`dde-am` 会正确处理窗口激活。

```cpp
// 通过 itemCommand 打开控制中心模块
const QString MyPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return QString("dde-am org.deepin.dde.control-center -- mymodule");
}
```
