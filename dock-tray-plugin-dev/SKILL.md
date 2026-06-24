---
name: dock-tray-plugin-dev
description: >
  辅助为 dde-tray-loader 开发托盘（Tray）插件。当需要为 DDE 任务栏编写新的托盘插件、
  修改现有托盘插件、或理解托盘插件接口时使用此 skill。仅支持 Tray 类型插件，
  不涉及 Dock/Quick 等其他插件类型。
---

# dock-tray-plugin-dev

辅助为 dde-tray-loader 开发托盘（Tray）插件。

## 概述

本 skill 辅助开发 DDE 任务栏托盘区插件。托盘插件是遵循 Qt 插件标准的共享库（`.so`），安装路径为 `lib/dde-dock/plugins`，通过实现 `PluginsItemInterfaceV2` 接口与任务栏交互。

## 前置条件

- Qt 6 + C++17 开发环境
- `dde-tray-loader-dev` 包（提供接口头文件和 CMake 配置）：`sudo apt install dde-tray-loader-dev`
- DTK 开发库（Dtk6::Widget, Dtk6::Gui）

## 托盘插件核心知识

### 插件类结构

- 必须继承 `QObject` + `PluginsItemInterfaceV2`
- `flags()` 必须返回含 `Dock::Type_Tray` 的标志
- `flags()` 典型返回 `Dock::Type_Tray | Dock::Attribute_CanSetting`

### 必须实现的接口

| 接口 | 说明 |
|------|------|
| `pluginName()` | 返回插件唯一标识名 |
| `init(PluginProxyInterface *)` | 初始化入口，保存 proxy 到 `m_proxyInter` |
| `itemWidget(itemKey)` | 返回托盘区显示的主控件 |

### 推荐实现的接口

| 接口 | 说明 |
|------|------|
| `icon(IconType, ThemeType)` | 控制中心插件区域图标（`Attribute_CanSetting` 时必须实现） |
| `itemTipsWidget(itemKey)` | 鼠标悬浮提示 |
| `itemContextMenu(itemKey)` / `invokedMenuItem(...)` | 右键菜单 |
| `pluginIsAllowDisable()` / `pluginIsDisable()` / `pluginStateSwitched()` | 插件禁用/启用 |
| `setMessageCallback()` / `message()` | 消息通信 |

### JSON 元数据

```json
{
    "api": "2.0.0"
}
```

## 典型托盘插件结构

```
my-plugin/
├── myplugin.h            # 插件类声明
├── myplugin.cpp          # 插件类实现
├── mywidget.h            # 主控件声明
├── mywidget.cpp          # 主控件实现
├── myplugin.json         # 元数据文件
├── myplugin.qrc          # Qt 资源文件（图标等）
├── icons/                # 图标资源目录
│   ├── myplugin.svg
│   └── myplugin-dark.svg
└── CMakeLists.txt        # 构建配置
```

## 开发流程指引

1. **创建插件类**：继承 `QObject` + `PluginsItemInterfaceV2`，声明 `Q_INTERFACES`、`Q_PLUGIN_METADATA` 宏
2. **实现 `flags()`**：返回 `Type_Tray | Attribute_CanSetting` 等标志
3. **实现 `init()`**：保存 proxy，使用延迟加载模式创建主控件
4. **实现交互功能**：tips / applet / context menu / command
5. **编写 JSON 元数据和 CMakeLists.txt**

> 快速上手？直接阅读 references/ai-usage-guide.md，按章节逐步生成插件代码。

## 参考实现

notification 插件路径：https://github.com/linuxdeepin/dde-tray-loader/tree/master/plugins/dde-dock/notification

## References 加载指引

按需加载以下参考文档：

1. **`tray-plugin-spec.md`** — 托盘插件接口规范（V1/V2 + flags + proxy 合并文档）。**首次开发时必须阅读。**
2. **`message-protocol.md`** — 消息协议。当插件需要与任务栏进行消息通信时阅读。
3. **`context-menu.md`** — 右键菜单协议。当插件需要实现右键菜单时阅读。
4. **`ai-usage-guide.md`** — AI 使用指南。按章节指导 AI 逐步生成托盘插件代码，每章含 prompt 示例和代码骨架。
