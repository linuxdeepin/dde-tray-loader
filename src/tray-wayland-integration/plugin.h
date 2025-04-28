// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QWindow>
#include <QString>

namespace Plugin {
class EmbedPluginPrivate;

class EmbedPlugin : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString itemKey READ itemKey WRITE setItemKey NOTIFY itemKeyChanged)
    Q_PROPERTY(QString pluginId READ pluginId WRITE setPluginId NOTIFY pluginIdChanged)
    Q_PROPERTY(int pluginType READ pluginType WRITE setPluginType NOTIFY pluginTypeChanged)
    Q_PROPERTY(int pluginFlags READ pluginFlags WRITE setPluginFlags NOTIFY pluginFlagsChanged)
    Q_PROPERTY(uint32_t pluginSizePolicy READ pluginSizePolicy WRITE setPluginSizePolicy NOTIFY pluginSizePolicyChanged)
    Q_PROPERTY(QPoint rawGlobalPos READ rawGlobalPos WRITE setRawGlobalPos NOTIFY rawGlobalPosChanged)
    Q_PROPERTY(QString dccIcon READ dccIcon WRITE setDccIcon NOTIFY dccIconChanged)

public:
    enum PluginType {
        Tray = 1,
        Fixed,
        Quick,
    };
    Q_ENUM(PluginType)

    ~EmbedPlugin();

    QString pluginId() const;
    void setPluginId(const QString& pluginid);

    QString itemKey() const;
    void setItemKey(const QString& itemKey);

    int pluginFlags() const;
    void setPluginFlags(int flags);

    int pluginType() const;
    void setPluginType(int type);

    int pluginSizePolicy() const;
    void setPluginSizePolicy(int sizePolicy);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    QPoint rawGlobalPos() const;
    void setRawGlobalPos(const QPoint& pos);

    QString dccIcon() const;
    void setDccIcon(const QString &dccIcon);

    QPoint pluginPos();
    void setPluginPos(const QPoint &pos);

    static EmbedPlugin *getWithoutCreating(QWindow *window);
    static EmbedPlugin* get(QWindow* window);
    static bool contains(QWindow* window);
    static bool contains(const QString &pluginId, int type, const QString &itemKey = QString());
    static QList<EmbedPlugin *> all();

Q_SIGNALS:
    void eventMessage(const QString &msg);
    void eventGeometry(const QRect &geometry);
    void contentMarginChanged(int32_t spacing);
    void dockPositionChanged(uint32_t position);
    void dockColorThemeChanged(uint32_t colorType);
    void pluginSupportFlagChanged(bool);
    void dccIconChanged(const QString &dccIcon);
    void pluginPosChanged(const QPoint &point);

Q_SIGNALS:
    void itemKeyChanged();
    void pluginIdChanged();
    void pluginTypeChanged();
    void pluginFlagsChanged();
    void pluginSizePolicyChanged();
    void requestMessage(const QString &msg);
    void pluginRecvMouseEvent(int type);
    void rawGlobalPosChanged();
    void pluginRequestShutdown(const QString &type);
    void closeQuickPanel();

private:
    explicit EmbedPlugin(QWindow* window);
    QScopedPointer<EmbedPluginPrivate> d;
};


class PluginPopupPrivate;

class PluginPopup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString itemKey READ itemKey WRITE setItemKey NOTIFY itemKeyChanged)
    Q_PROPERTY(QString pluginId READ pluginId WRITE setPluginId NOTIFY pluginIdChanged)
    Q_PROPERTY(int popupType READ popupType WRITE setPopupType NOTIFY popupTypeChanged)

    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)

public:
    enum PopupType {
        PopupTypePanel = 1,
        PopupTypeTooltip = 2,
        PopupTypeMenu = 3,
        PopupTypeEmbed = 4,
        PopupTypeSubPopup = 5
    };

public:
    ~PluginPopup();

    QString pluginId() const;
    void setPluginId(const QString& pluginid);

    QString itemKey() const;
    void setItemKey(const QString& itemKey);

    int popupType() const;
    void setPopupType(const int& type);

    int x() const;
    void setX(const int& x);

    int y() const;
    void setY(const int& y);

    QPoint pluginPos();
    void setPluginPos(const QPoint &pos);

    static PluginPopup *getWithoutCreating(QWindow *window);
    static PluginPopup* get(QWindow* window);
    static void remove(QWindow *window);
    static bool contains(QWindow* window);

Q_SIGNALS:
    void eventGeometry(const QRect &geometry);

Q_SIGNALS:
    void itemKeyChanged();
    void pluginIdChanged();
    void popupTypeChanged();

    void xChanged();
    void yChanged();
    void pluginPosChanged(const QPoint &point);

private:
    explicit PluginPopup(QWindow* window);
    QScopedPointer<PluginPopupPrivate> d;
};

class PlatformInterfaceProxy : public QObject
{
    Q_OBJECT
public:
    static PlatformInterfaceProxy *instance();

    QByteArray fontName() const;
    void setFontName(const QByteArray &newFontName);

    qreal fontPointSize() const;
    void setFontPointSize(qreal newFontPointSize);

    QColor activeColor() const;
    void setActiveColor(const QColor &newActiveColor);

    QColor darkActiveColor() const;
    void setDarkActiveColor(const QColor &newDarkActiveColor);

    QByteArray themeName() const;
    void setThemeName(const QByteArray &newThemeName);

    QByteArray iconThemeName() const;
    void setIconThemeName(const QByteArray &newIconThemeName);

Q_SIGNALS:
    void fontNameChanged(QByteArray fontName);
    void fontPointSizeChanged(qreal fontPointSize);
    void activeColorChanged(QColor activeColor);
    void darkActiveColorChanged(QColor activeColor);
    void themeNameChanged(QByteArray themeName);
    void iconThemeNameChanged(QByteArray iconThemeName);

private:
    explicit PlatformInterfaceProxy(QObject *parent = nullptr);

    QByteArray m_fontName;
    qreal m_fontPointSize {0};
    QColor m_activeColor;
    QColor m_darkActiveColor;
    QByteArray m_themeName;
    QByteArray m_iconThemeName;
};
}
