// SPDX-FileCopyrightText: 2019 - 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef PAGEBUTTON_H
#define PAGEBUTTON_H

#include "commoniconbutton.h"

#include <QFrame>

class PageButton : public QFrame
{
    Q_OBJECT
public:
    explicit PageButton(QWidget *parent = nullptr);
    ~PageButton();

    void setIcon(const QIcon &icon);

signals:
    void clicked();

protected:
    bool event(QEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void initUI();

private:
    bool m_hover;
    bool m_mousePress;
    CommonIconButton *m_iconButton;
};

#endif // PAGEBUTTON_H 
