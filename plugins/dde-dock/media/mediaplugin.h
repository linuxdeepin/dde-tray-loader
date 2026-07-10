// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef MEDIAPLUGIN_H
#define MEDIAPLUGIN_H

#include "mediacontroller.h"
#include "pluginsiteminterface_v3.h"
#include "quickpanelwidget.h"

#include "dtkcore_global.h"

#include <QPointer>

class QQuickView;

class MediaPlugin : public QObject, public PluginsItemInterfaceV3
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV3)
    Q_PLUGIN_METADATA(IID ModuleInterface_iid_V3 FILE "media.json")

public:
    explicit MediaPlugin(QObject *parent = nullptr);
    ~MediaPlugin() override;

    virtual const QString pluginName() const Q_DECL_OVERRIDE;
    virtual const QString pluginDisplayName() const Q_DECL_OVERRIDE;
    virtual void init(PluginProxyInterface *proxyInter) Q_DECL_OVERRIDE;
    virtual void pluginStateSwitched() Q_DECL_OVERRIDE;
    virtual bool pluginIsAllowDisable() Q_DECL_OVERRIDE { return true; }
    virtual bool pluginIsDisable() Q_DECL_OVERRIDE;
    virtual QWidget *itemWidget(const QString &itemKey) Q_DECL_OVERRIDE;
    virtual QWidget *itemPopupApplet(const QString &itemKey) Q_DECL_OVERRIDE;
    virtual QWidget *itemTipsWidget(const QString &itemKey) Q_DECL_OVERRIDE;
    virtual const QString itemContextMenu(const QString &itemKey) Q_DECL_OVERRIDE;
    virtual void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) Q_DECL_OVERRIDE;
    virtual int itemSortKey(const QString &itemKey) Q_DECL_OVERRIDE;
    virtual void setSortKey(const QString &itemKey, const int order) Q_DECL_OVERRIDE;
    virtual void refreshIcon(const QString &itemKey) Q_DECL_OVERRIDE;
    virtual Dock::PluginFlags flags() const override { return Dock::Type_Quick | Dock::Quick_Panel_Full | Dock::Attribute_HasCard; }
    virtual void pluginSettingsChanged() override;
    QString cardItemKey() const override;
    QWindow *cardWindow() const override;

private:
    void refreshPluginItemsVisible();

private:
    QScopedPointer<MediaController> m_controller;
    QScopedPointer<QuickPanelWidget> m_quickPanelWidget;
    mutable QPointer<QQuickView> m_cardView;
};

#endif // MEDIAPLUGIN_H
