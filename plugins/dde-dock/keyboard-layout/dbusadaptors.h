// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DBUSADAPTORS_H
#define DBUSADAPTORS_H

#include "fcitxinterface.h"

#include <QMenu>
#include <QtDBus/QtDBus>

#include "inputdevice1keyboardinterface.h"

using Keyboard = org::deepin::dde::inputdevice1::Keyboard;

class DBusAdaptors : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Keyboard1")
//    Q_CLASSINFO("D-Bus Introspection", ""
//                "  <interface name=\"com.deepin.dde.Keyboard\">\n"
//                "    <property access=\"read\" type=\"s\" name=\"layout\"/>\n"
//                "    <signal name=\"layoutChanged\">"
//                "        <arg name=\"layout\" type=\"s\"/>"
//                "    </signal>"
//                "  </interface>\n"
//                "")

public:
     explicit DBusAdaptors(QObject *parent = nullptr);
    ~DBusAdaptors();

public:
    Q_PROPERTY(QString layout READ layout WRITE setLayout NOTIFY layoutChanged)
    Q_PROPERTY(bool fcitxRunning READ isFcitxRunning NOTIFY fcitxStatusChanged)
    QString layout() const;
    void setLayout(const QString &str);
    bool isFcitxRunning() const;

    Keyboard *getCurrentKeyboard();

public slots:
    void onClicked(int button, int x, int y);

signals:
    void layoutChanged(QString text);
    void fcitxStatusChanged(bool running);

private slots:
    void onCurrentLayoutChanged(const QString & value);
    void onUserLayoutListChanged(const QStringList & value);
    void initAllLayoutList();
    void refreshMenu();
    void refreshMenuSelection();
    void handleActionTriggered(QAction *action);

private slots:
    void onGSettingsChanged(const QString &key);
    void onFcitxConnected(const QString &service);
    void onFcitxDisconnected(const QString &service);
    void onPropertyChanged(QString name, QVariantMap map, QStringList list);

private:
    QString duplicateCheck(const QString &kb);
    void setKeyboardLayoutGsettings();
    void initFcitxWatcher();

private:
    Keyboard *m_keyboard;
    bool m_fcitxRunning;
    FcitxInputMethodProxy *m_inputmethod;
    QDBusServiceWatcher *m_fcitxWatcher;
    QMenu *m_menu;
    QAction *m_addLayoutAction;

    QString m_currentLayoutRaw;
    QString m_currentLayout;
    QStringList m_userLayoutList;
    KeyboardLayoutList m_allLayoutList;
};

#endif
