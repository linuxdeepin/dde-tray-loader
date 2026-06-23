// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef QUICKPANELWIDGET_H
#define QUICKPANELWIDGET_H

#include "commoniconbutton.h"
#include "mediacontroller.h"

#include <QGraphicsOpacityEffect>
#include <QWidget>

#include <DLabel>

DWIDGET_USE_NAMESPACE
class QuickPanelWidget : public QWidget
{
    Q_OBJECT
public:
    QuickPanelWidget(MediaController *controller, QWidget *parent = nullptr);
    ~QuickPanelWidget();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

Q_SIGNALS:
    void clicked();
    void requestHideApplet();

private:
    void init();
    void updateUI();

private:
    static constexpr qreal kNormalOpacity = 0.7;
    static constexpr qreal kHoverOpacity = 1.0;

    MediaController *m_controller;

    DLabel *m_pixmap;
    DLabel *m_titleLab;
    DLabel *m_artistLab;
    CommonIconButton *m_playButton;
    CommonIconButton *m_nextButton;
    QGraphicsOpacityEffect *m_opacityEffect;
};

#endif
