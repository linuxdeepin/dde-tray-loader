// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "mediaplugin.h"
#include "mediacontroller.h"
#include "quickpanelwidget.h"
#include "plugins-logging-category.h"

#include <QQmlContext>
#include <QQuickView>

#define MEDIA_KEY "media-key"
#define STATE_KEY  "enable"

// 卡片在卡片区域中的排序值，可通过插件配置调整
#define CARD_ORDER_KEY "card-order"
#define CARD_ORDER_DEFAULT 30

Q_LOGGING_CATEGORY(MEDIA, "org.deepin.dde.dock.media")

MediaPlugin::MediaPlugin(QObject *parent)
    : QObject(parent)
    , m_quickPanelWidget(nullptr)
{

}

MediaPlugin::~MediaPlugin()
{
    if (m_cardView) {
        delete m_cardView;
        m_cardView = nullptr;
    }
}

void MediaPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    m_controller.reset(new MediaController);
    m_quickPanelWidget.reset(new QuickPanelWidget(m_controller.data()));
    m_quickPanelWidget->setFixedHeight(60);

    connect(m_controller.data(), &MediaController::mediaAcquired, this, &MediaPlugin::refreshPluginItemsVisible);
    connect(m_controller.data(), &MediaController::mediaLosted, this, &MediaPlugin::refreshPluginItemsVisible);
    connect(m_quickPanelWidget.data(), &QuickPanelWidget::requestHideApplet, this, [this] {
        if (m_proxyInter)
            m_proxyInter->requestSetAppletVisible(this, MEDIA_KEY, false);
    });

    refreshPluginItemsVisible();
}

const QString MediaPlugin::pluginName() const
{
    return "media";
}

const QString MediaPlugin::pluginDisplayName() const
{
    return "Media";
}

void MediaPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, STATE_KEY, pluginIsDisable());
}

bool MediaPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, STATE_KEY, true).toBool();
}

QWidget *MediaPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == Dock::QUICK_ITEM_KEY) {
        return m_quickPanelWidget.data();
    }

    return nullptr;
}

QWidget *MediaPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return nullptr;
}

QWidget *MediaPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return nullptr;
}

void MediaPlugin::refreshIcon(const QString &itemKey)
{
    Q_UNUSED(itemKey)
}

const QString MediaPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return QString();
}

void MediaPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(menuId)
    Q_UNUSED(checked)
}

int MediaPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);

    return m_proxyInter->getValue(this, key, -1).toInt();
}

void MediaPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);

    m_proxyInter->saveValue(this, key, order);
}

void MediaPlugin::pluginSettingsChanged()
{
    refreshPluginItemsVisible();
}

void MediaPlugin::refreshPluginItemsVisible()
{
    if (!m_proxyInter) {
        return;
    }

    if (pluginIsDisable() || !m_controller || !m_controller->isWorking()) {
        m_proxyInter->itemRemoved(this, MEDIA_KEY);
        return;
    }

    m_proxyInter->itemAdded(this, MEDIA_KEY);
}

QString MediaPlugin::cardItemKey() const
{
    return QStringLiteral("media-card");
}

int MediaPlugin::cardOrder() const
{
    // Read from the plugin configuration so the position of the music card can
    // be adjusted per product without touching the dock.
    if (!m_proxyInter) {
        return CARD_ORDER_DEFAULT;
    }

    bool ok = false;
    const int order = m_proxyInter->getValue(const_cast<MediaPlugin *>(this),
                                             CARD_ORDER_KEY,
                                             CARD_ORDER_DEFAULT).toInt(&ok);
    return ok ? order : CARD_ORDER_DEFAULT;
}

QWidget *MediaPlugin::cardTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    if (!m_controller || !m_controller->isWorking()) {
        return nullptr;
    }

    // "歌名 - 播放软件"
    const QString title = m_controller->titleText();
    const QString player = m_controller->appName();

    QString text = title;
    if (!player.isEmpty() && player != title) {
        text = QStringLiteral("%1 - %2").arg(title, player);
    }

    if (text.isEmpty()) {
        return nullptr;
    }

    if (!m_cardTips) {
        m_cardTips.reset(new Dock::TipsWidget);
    }

    // Reset the text on every call so the tooltip picks up track changes and
    // resizes after the font size changed in the control center.
    m_cardTips->setText(text);
    return m_cardTips.data();
}

QWindow *MediaPlugin::cardWindow() const
{
    if (m_cardView) {
        return m_cardView;
    }

    auto view = new QQuickView;
    view->setColor(Qt::transparent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->rootContext()->setContextProperty(QStringLiteral("mediaController"), m_controller.data());
    view->setSource(QUrl(QStringLiteral("qrc:/media/card")));
    if (view->status() == QQuickView::Error) {
        delete view;
        return nullptr;
    }

    m_cardView = view;
    return m_cardView;
}
