//SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
//SPDX-License-Identifier: GPL-3.0-or-later

#include <QMenu>

class DockContextMenu : public QMenu
{
public:
    explicit DockContextMenu(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent* e) override;
};
