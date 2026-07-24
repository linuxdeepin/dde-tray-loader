// Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "brightnesscontroller.h"
#include "monitor.h"

#include <DBlurEffectWidget>
#include <QGraphicsOpacityEffect>
#include <QWidget>
#include <QPointer>

class QDBusMessage;
class SliderContainer;
class QLabel;

DWIDGET_USE_NAMESPACE

class BrightnessQuickPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BrightnessQuickPanel(QWidget *parent = nullptr);
    ~BrightnessQuickPanel() override;

Q_SIGNALS:
    void requestShowApplet();

protected:
    void initUi();
    void initConnection();
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private Q_SLOTS:
    void refreshWidget();
    void UpdateDisplayStatus();

private:
    static constexpr qreal kNormalOpacity = 0.7;
    static constexpr qreal kHoverOpacity = 1.0;

    SliderContainer *m_sliderContainer;
    QPointer<Monitor> m_currentMonitor;
    QGraphicsOpacityEffect *m_opacityEffect;
};
