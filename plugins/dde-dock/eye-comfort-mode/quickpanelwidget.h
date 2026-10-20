// Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef QUICKPANELWIDGET_H
#define QUICKPANELWIDGET_H

#include <DFloatingButton>
#include <DLabel>
#include <DStyleOption>

#include <QWidget>

DWIDGET_USE_NAMESPACE

class QuickButton : public DFloatingButton
{
public:

    enum ButtonMode {
        ClickButton,
        DisplayButton
    };

    QuickButton(QWidget *parent = nullptr)
        : DFloatingButton(parent)
        , m_mode(ButtonMode::ClickButton)
        , m_parentHover(false)
    {
    }

    void setButtonMode(ButtonMode mode) {
        if (m_mode == mode)
            return;

        m_mode = mode;
        DStyleOptionButton option;
        initStyleOption(&option);
    }

    void setParentHover(bool hover) {
        if (m_parentHover == hover)
            return;
        m_parentHover = hover;
        update();
    }

protected:
    void initStyleOption(DStyleOptionButton *option) const override;

private:
    ButtonMode m_mode;
    bool m_parentHover;
};

class QLabel;
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
    void setButtonMode(QuickButton::ButtonMode mode);

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
    void updateTextColor(bool hover);

private:
    QuickButton *m_iconWidget;
    Dtk::Widget::DLabel *m_nameLabel;
    Dtk::Widget::DLabel *m_stateLabel;
    Dtk::Widget::DIconButton *m_expandLabel;
    QPoint m_clickPoint;
    bool m_eyeComfortModeEnabled;
    bool m_hover;
};

#endif // QUICKPANELWIDGET_H
