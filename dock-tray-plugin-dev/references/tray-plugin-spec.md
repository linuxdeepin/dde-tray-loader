# 托盘插件接口规范

本文档合并了 dde-tray-loader 托盘插件开发所需的全部接口定义，包括 V1/V2 接口、PluginFlags、PluginProxyInterface。

## 1. 插件继承关系

托盘插件必须继承 `QObject` + `PluginsItemInterfaceV2`：

```
PluginsItemInterface          (V1 基类)
  └── PluginsItemInterfaceV2  (V2 扩展，托盘插件使用此类)
```

IID 声明：
- V1 IID: `"com.deepin.dock.PluginsItemInterface"` — 定义为 `ModuleInterface_iid`
- V2 IID: `"com.deepin.dock.PluginsItemInterface_V2"` — 定义为 `ModuleInterface_iid_V2`

> **注意**：任务栏插件加载器通过 V1 IID `"com.deepin.dock.PluginsItemInterface"` 发现插件。若 `Q_PLUGIN_METADATA` 使用 V2 IID，加载器将无法识别该插件。因此新插件必须使用 V1 IID，并通过 `Q_INTERFACES(PluginsItemInterfaceV2)` 声明 V2 接口支持。

## 2. PluginsItemInterface (V1 基类)

源文件：`interfaces/pluginsiteminterface.h`

### 2.1 必须实现的接口

| 接口 | 签名 | 说明 |
|------|------|------|
| `pluginName` | `virtual const QString pluginName() const = 0` | 返回插件唯一标识名，不可与其他插件冲突 |
| `init` | `virtual void init(PluginProxyInterface *proxyInter) = 0` | 插件初始化入口，必须将 `proxyInter` 保存到 `m_proxyInter`，**不要 delete 此指针** |
| `itemWidget` | `virtual QWidget *itemWidget(const QString &itemKey) = 0` | 返回插件主控件，用于显示在任务栏托盘区 |

### 2.2 可选实现的接口

| 接口 | 签名 | 说明 |
|------|------|------|
| `pluginDisplayName` | `virtual const QString pluginDisplayName() const` | 返回插件显示名称，用于界面展示 |
| `itemTipsWidget` | `virtual QWidget *itemTipsWidget(const QString &itemKey)` | 返回鼠标悬浮时的提示控件，返回 `nullptr` 则不显示 |
| `itemPopupApplet` | `virtual QWidget *itemPopupApplet(const QString &itemKey)` | 返回左键点击后弹出的面板控件 |
| `itemCommand` | `virtual const QString itemCommand(const QString &itemKey)` | 返回左键点击后要执行的命令字符串，空字符串则忽略 |
| `itemContextMenu` | `virtual const QString itemContextMenu(const QString &itemKey)` | 返回右键菜单的 JSON 数据，详见 `context-menu.md` |
| `invokedMenuItem` | `virtual void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)` | 右键菜单项被点击的回调 |

> **翻译说明**：涉及翻译的内容（如菜单文本、面板文字等），需要插件自行加载翻译文件处理，框架不处理翻译问题。使用 Qt 的翻译机制（`tr()` 函数 + `.ts`/`.qm` 文件）。
| `itemSortKey` | `virtual int itemSortKey(const QString &itemKey)` | 返回控件排序位置，1 为默认，0 为左侧，-1 为右侧 |
| `setSortKey` | `virtual void setSortKey(const QString &itemKey, const int order)` | 用户拖拽后保存新的排序位置 |
| `pluginIsAllowDisable` | `virtual bool pluginIsAllowDisable()` | 插件是否允许被禁用，默认 `false` |
| `pluginIsDisable` | `virtual bool pluginIsDisable()` | 插件当前是否被禁用，默认 `false` |
| `pluginStateSwitched` | `virtual void pluginStateSwitched()` | 插件启用/禁用状态被切换时调用 |
| `displayModeChanged` | `virtual void displayModeChanged(const Dock::DisplayMode displayMode)` | 任务栏显示模式变化时调用 |
| `positionChanged` | `virtual void positionChanged(const Dock::Position position)` | 任务栏位置变化时调用 |
| `refreshIcon` | `virtual void refreshIcon(const QString &itemKey)` | 系统图标主题变化时调用，用于刷新图标 |
| `pluginSettingsChanged` | `virtual void pluginSettingsChanged()` | 插件设置变化时调用 |
| `itemAllowContainer` | `virtual bool itemAllowContainer(const QString &itemKey)` | 是否允许收纳，默认 `false` |
| `itemIsInContainer` | `virtual bool itemIsInContainer(const QString &itemKey)` | 是否处于收纳模式 |
| `setItemIsInContainer` | `virtual void setItemIsInContainer(const QString &itemKey, const bool container)` | 更新收纳模式状态 |
| `pluginSizePolicy` | `virtual PluginSizePolicy pluginSizePolicy() const` | 返回插件尺寸策略，默认 `System`。`Custom` 时需自行处理控件大小 |

### 2.3 辅助方法

| 方法 | 说明 |
|------|------|
| `displayMode()` | 获取当前任务栏显示模式（Fashion/Efficient） |
| `position()` | 获取当前任务栏位置（Top/Right/Bottom/Left） |

### 2.4 PluginSizePolicy 枚举

| 值 | 含义 |
|----|------|
| `System = 1 << 0` | 跟随系统 |
| `Custom = 1 << 1` | 自定义 |

### 2.5 保护成员

```cpp
PluginProxyInterface *m_proxyInter = nullptr;  // 永远不要 delete
```

## 3. PluginsItemInterfaceV2 (V2 扩展)

源文件：`interfaces/pluginsiteminterface_v2.h`

V2 继承自 V1，新增以下接口：

### 3.1 flags — 插件标识（必须覆写）

```cpp
virtual Dock::PluginFlags flags() const { return Dock::Type_System | Dock::Attribute_Normal; }
```

**托盘插件必须覆写**，典型返回值：

```cpp
Dock::PluginFlags flags() const override {
    return Dock::PluginFlag::Type_Tray | Dock::PluginFlag::Attribute_CanSetting;
}
```

- `Type_Tray (0x10)`：标识插件类型为托盘区
- `Attribute_CanSetting (0x800)`：允许在控制中心设置显示/隐藏，设置此属性时**必须**实现 `icon()` 方法

### 3.2 icon — 控制中心图标

```cpp
virtual QIcon icon(Dock::IconType, Dock::ThemeType) const { return QIcon(); }
```

当 `flags()` 包含 `Attribute_CanSetting` 时，此方法**必须实现**。返回的图标显示在「控制中心 → 个性化 → 任务栏 → 插件区域」中。

参数说明：
- `Dock::IconType`：图标类型，当前仅有 `IconType_None`
- `Dock::ThemeType`：主题类型，`ThemeType_Light`（亮色）/ `ThemeType_Dark`（暗色）

示例（参考 notification 插件）：

```cpp
QIcon icon(Dock::IconType dockPart, Dock::ThemeType themeType) const override {
    if (themeType == Dock::ThemeType_Dark) {
        return QIcon(":/dsg/built-in-icons/myplugin-dark.svg");
    } else {
        return QIcon(":/dsg/built-in-icons/myplugin.svg");
    }
}
```

> **图标使用主题图标**

> **控制中心图标安装**：当 `flags()` 包含 `Attribute_CanSetting` 时，还需安装图标文件（`.dci` 格式）到 `share/dde-dock/icons/dcc-setting/` 目录，供控制中心读取。参考 notification 插件的 CMakeLists.txt：
> ```cmake
> install(FILES "icons/dcc-notification.dci" DESTINATION share/dde-dock/icons/dcc-setting)
> ```

### 3.3 setMessageCallback — 设置消息回调

```cpp
virtual void setMessageCallback(MessageCallbackFunc) {}
```

设置消息回调函数，插件通过此回调向任务栏发送请求。详见 `message-protocol.md`。

### 3.4 message — 处理任务栏消息

```cpp
virtual QString message(const QString &) { return "{}"; }
```

任务栏向插件发送请求，使用 JSON 格式字符串传输数据。详见 `message-protocol.md`。

### 3.5 addPlugin — 承载子插件指针

```cpp
virtual void addPlugin(QPluginLoader*) {}
```

用于托盘容器插件接收子插件指针，实现一个托盘区容纳多个子插件的场景。单个托盘插件无需实现此接口。

## 4. MessageCallbackFunc 类型定义

```cpp
using MessageCallbackFunc = QString (*)(PluginsItemInterfaceV2 *, const QString&);
```

- 参数 1：`PluginsItemInterfaceV2 *` — 插件实例指针
- 参数 2：`const QString&` — 发送给任务栏的 JSON 数据
- 返回值：`QString` — 任务栏返回的 JSON 数据

## 5. PluginFlags 详解

源文件：`interfaces/constants.h`

### 5.1 类型标志（Type）

| 标志 | 值 | 说明 |
|------|----|------|
| `Type_None` | `0x00` | 默认值 |
| `Type_Unadapted` | `0x01` | 未适配，请勿使用 |
| `Type_Quick` | `0x02` | 快捷插件区 |
| `Type_Tool` | `0x04` | 工具插件（如回收站） |
| `Type_System` | `0x08` | 系统插件（如关机） |
| **`Type_Tray`** | **`0x10`** | **托盘区插件（本 skill 关注的类型）** |
| `Type_Fixed` | `0x20` | 固定区域插件 |

### 5.2 快捷面板布局（仅 Quick 类型使用）

| 标志 | 值 | 说明 |
|------|----|------|
| `Quick_Panel_Single` | `0x40` | 单列快捷插件 |
| `Quick_Panel_Multi` | `0x80` | 双列快捷插件 |
| `Quick_Panel_Full` | `0x100` | 四列整行快捷插件 |

### 5.3 属性标志（Attribute）

| 标志 | 值 | 说明 |
|------|----|------|
| `Attribute_CanDrag` | `0x200` | 支持拖动 |
| `Attribute_CanInsert` | `0x400` | 支持在前方插入其他插件 |
| **`Attribute_CanSetting`** | **`0x800`** | **允许在控制中心设置显隐，设置时必须实现 `icon()`** |
| `Attribute_ForceDock` | `0x1000` | 强制显示在任务栏 |
| `Attribute_Normal` | `CanDrag \| CanInsert \| CanSetting` | 普通插件属性组合 |

### 5.4 托盘插件常用 flags 组合

| 组合 | 适用场景 |
|------|----------|
| `Type_Tray \| Attribute_CanSetting` | 可在控制中心显隐的托盘插件（**最常见**） |
| `Type_Tray \| Attribute_CanDrag \| Attribute_CanInsert \| Attribute_CanSetting` | 完整属性的可拖拽托盘插件 |
| `Type_Tray \| Attribute_ForceDock` | 强制显示的托盘插件 |

## 6. PluginProxyInterface 详解

源文件：`interfaces/pluginproxyinterface.h`

插件通过 `m_proxyInter` 调用以下方法主动与任务栏交互：

> **简化说明**：以下方法签名省略了部分 `const` 限定符，完整签名请参考 `interfaces/pluginproxyinterface.h` 源文件。

### 6.1 控件管理

| 方法 | 签名 | 说明 |
|------|------|------|
| `itemAdded` | `void itemAdded(PluginsItemInterface *itemInter, const QString &itemKey)` | 向任务栏添加控件，`itemKey` 需在本插件内唯一 |
| `itemUpdate` | `void itemUpdate(PluginsItemInterface *itemInter, const QString &itemKey)` | 通知任务栏刷新指定控件 |
| `itemRemoved` | `void itemRemoved(PluginsItemInterface *itemInter, const QString &itemKey)` | 从任务栏移除控件，不会删除控件对象 |

### 6.2 窗口控制

| 方法 | 签名 | 说明 |
|------|------|------|
| `requestWindowAutoHide` | `void requestWindowAutoHide(PluginsItemInterface *itemInter, const QString &itemKey, const bool autoHide)` | 设置任务栏是否允许自动隐藏 |
| `requestRefreshWindowVisible` | `void requestRefreshWindowVisible(PluginsItemInterface *itemInter, const QString &itemKey)` | 通知任务栏刷新隐藏状态 |
| `requestSetAppletVisible` | `void requestSetAppletVisible(PluginsItemInterface *itemInter, const QString &itemKey, const bool visible)` | 控制弹出面板的显隐 |

### 6.3 配置存储

| 方法 | 签名 | 说明 |
|------|------|------|
| `saveValue` | `void saveValue(PluginsItemInterface *itemInter, const QString &key, const QVariant &value)` | 保存配置到 `.config/deepin/dde-dock.conf`，按 `pluginName()` 分组 |
| `getValue` | `const QVariant getValue(PluginsItemInterface *itemInter, const QString &key, const QVariant &fallback = QVariant())` | 读取配置值 |
| `removeValue` | `void removeValue(PluginsItemInterface *itemInter, const QStringList &keyList)` | 删除指定 key 的配置，空列表则删除该插件所有配置 |

## 7. 常量与尺寸

源文件：`interfaces/constants.h`

### 7.1 托盘插件关键尺寸

| 常量 | 值 | 说明 |
|------|----|------|
| `TRAY_PLUGIN_ITEM_FIXED_WIDTH` | `16` | 托盘插件固定宽度 |
| `TRAY_PLUGIN_ITEM_FIXED_HEIGHT` | `16` | 托盘插件固定高度 |
| `TRAY_PLUGIN_ITEM_FIXED_SIZE` | `QSize(16, 16)` | 托盘插件固定大小 |
| `DOCK_PLUGIN_ITEM_FIXED_SIZE` | `QSize(16, 16)` | 任务栏插件固定大小 |

### 7.2 API 版本

| 常量 | 值 |
|------|----|
| `DOCK_PLUGIN_API_VERSION` | `"2.0.0"` |
| `DOCK_API_VERSION_MAJOR` | `2` |
| `DOCK_API_VERSION_MINOR` | `0` |
| `DOCK_API_VERSION_PATCH` | `0` |

### 7.3 其他常量

| 常量 | 说明 |
|------|------|
| `DOCK_PLUGIN_MIME` | `"dock/plugin"` |
| `PROP_DISPLAY_MODE` | `"DisplayMode"` |
| `PROP_POSITION` | `"Position"` |
| `PLUGIN_MIN_ICON_NAME` | `"-dark"` — 最小尺寸图标的后缀 |

## 8. JSON 元数据格式

插件必须包含一个 JSON 元数据文件，通过 `Q_PLUGIN_METADATA(FILE "...")` 宏加载：

```json
{
    "api": "2.0.0"
}
```

字段说明：
- `api`：**必填**，接口版本号，必须为 `"2.0.0"`

## 9. DisplayMode 与 Position 枚举

### DisplayMode

| 值 | 名称 | 说明 |
|----|------|------|
| 0 | `Fashion` | 时尚模式 |
| 1 | `Efficient` | 高效模式 |

### Position

| 值 | 名称 | 说明 |
|----|------|------|
| 0 | `Top` | 顶部 |
| 1 | `Right` | 右侧 |
| 2 | `Bottom` | 底部 |
| 3 | `Left` | 左侧 |
