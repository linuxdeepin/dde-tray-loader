// Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef QUICKPANELWIDGET_H
#define QUICKPANELWIDGET_H

#include <QGraphicsOpacityEffect>

#include <DFloatingButton>
#include <DLabel>

#include <QWidget>

class QLabel;
class QuickButton;

class QuickPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuickPanelWidget(QWidget *parent = nullptr);
    ~QuickPanelWidget() override;

public Q_SLOTS:
    void setIcon(const QIcon &icon);
    void setText(const QString &text);
    void setDescription(const QString &description);
    void setActive(bool active);

Q_SIGNALS:
    void panelClicked();
    void iconClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void initUi();
    void initConnection();
    void updateOpacity(bool hover);

private:
    static constexpr qreal kNormalOpacity = 0.7;
    static constexpr qreal kHoverOpacity = 1.0;

    QuickButton *m_iconWidget;
    Dtk::Widget::DLabel *m_nameLabel;
    Dtk::Widget::DLabel *m_stateLabel;
    Dtk::Widget::DIconButton *m_expandLabel;
    QPoint m_clickPoint;
    bool m_active = false;
    bool m_hovered = false;
    QGraphicsOpacityEffect *m_iconEffect;
    QGraphicsOpacityEffect *m_nameEffect;
    QGraphicsOpacityEffect *m_stateEffect;
};

#endif // QUICKPANELWIDGET_H
