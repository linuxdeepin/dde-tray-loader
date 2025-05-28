//SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
//SPDX-License-Identifier: GPL-3.0-or-later

#include "dockcontextmenu.h"

#include <QPainter>
#include <QWindow>

static const bool IS_WAYLAND_DISPLAY = !qgetenv("WAYLAND_DISPLAY").isEmpty();

DockContextMenu::DockContextMenu(QWidget *parent)
    : QMenu(parent)
{
    // 解决键盘上下键不能操作右键菜单
    if (IS_WAYLAND_DISPLAY) {
        setAttribute(Qt::WA_NativeWindow);
        windowHandle()->setProperty("_d_dwayland_window-type", "focusmenu");
    }
}

void DockContextMenu::paintEvent(QPaintEvent* e)
{
    QMenu::paintEvent(e);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    for (auto action : actions()) {
        if (action->property("showReminder").toBool()) {
            auto geo = actionGeometry(action);
            QColor color("#FF3B30");
            p.setPen(color);
            p.setBrush(color);
            p.drawEllipse(geo.x() + geo.width() - 26, geo.y() + (geo.height() - 6) / 2, 6, 6);
        }
    }

    p.end();
}
