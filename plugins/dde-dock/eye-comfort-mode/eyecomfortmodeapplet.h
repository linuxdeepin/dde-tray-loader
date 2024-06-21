// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef EYECOMFORTMODEAPPLET_H
#define EYECOMFORTMODEAPPLET_H

#include "jumpsettingbutton.h"
#include "pluginlistview.h"

#include <QWidget>

#include <DSwitchButton>
#include <DLabel>

const QString DEEPIN_LIGHT = "deepin";
const QString DEEPIN_DARK = "deepin-dark";
const QString DEEPIN_AUTO = "deepin-auto";

class EyeComfortmodeApplet : public QWidget
{
    Q_OBJECT

public:
    explicit EyeComfortmodeApplet(QWidget *parent = nullptr);
    void setEnabled(bool enable);
    void setTitle(const QString &title);
    void setIcon(const QIcon &icon);
    void setDescription(const QString &description);
    void setDccPage(const QString &first, const QString &second);
    const QString gtkTheme();
    void setGtkTheme(const QString & value);
    void setEyeComfortVisible(bool visible);
    void setHeight(int height);

signals:
    void enableChanged(bool enable);
    void requestHideApplet();
    void gtkThemeChanged(const QString & value);

public slots:
    void onGtkThemeChanged(const QString & value);

private slots:
    void onThemeListClicked(const QModelIndex &index);

private:
    void initUi();
    void initConnect();

private:
    Dtk::Widget::DLabel *m_title;
    Dtk::Widget::DSwitchButton *m_switchBtn;
    JumpSettingButton *m_settingButton;
    PluginListView *m_themeView;
    QStandardItemModel *m_themeItemModel;
    PluginItem *m_lightTheme;
    PluginItem *m_darkTheme;
    PluginItem *m_autoTheme;
    QString m_gtkTheme;
};

#endif // EYECOMFORTMODEAPPLET_H
