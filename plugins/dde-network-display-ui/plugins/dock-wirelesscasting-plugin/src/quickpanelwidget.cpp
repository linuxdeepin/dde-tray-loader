// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "quickpanelwidget.h"

#include <DFontSizeManager>
#include <DStyle>
#include <DToolTip>

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

const QSize IconSize(24, 24); // 图标大小

namespace dde {
namespace wirelesscasting {
class QuickButton : public DFloatingButton
{
public:
    explicit QuickButton(QWidget *parent = nullptr)
        : DFloatingButton(parent)
    {
    }

protected:
    void initStyleOption(DStyleOptionButton *option) const override
    {
        DFloatingButton::initStyleOption(option);
        QColor bgColor = option->dpalette.color(backgroundRole());
        QColor textColor = option->dpalette.color(foregroundRole());
        if (backgroundRole() == QPalette::Highlight) {
            textColor = Qt::white;
            if (!option->state.testFlag(QStyle::State_Raised)) { // press
                bgColor.setHslF(bgColor.hslHueF(), bgColor.hslSaturationF(), bgColor.lightnessF() * 0.9);
                textColor.setAlphaF(0.8);
            } else if (option->state.testFlag(QStyle::State_MouseOver)) { // hover
                bgColor.setHslF(bgColor.hslHueF(), bgColor.hslSaturationF(), bgColor.lightnessF() * 1.1);
            }
        } else {
            bgColor.setAlphaF(0);
            textColor.setAlphaF(1);
        }
        option->palette.setBrush(QPalette::Button, bgColor);
        option->palette.setBrush(QPalette::ButtonText, textColor);
        option->state.setFlag(QStyle::State_MouseOver, false);
        option->state.setFlag(QStyle::State_Sunken, false);
        option->state.setFlag(QStyle::State_Raised, true);
    }
};

QuickPanelWidget::QuickPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_iconWidget(new QuickButton(this))
    , m_nameLabel(new DLabel(this))
    , m_stateLabel(new DLabel(this))
    , m_expandLabel(new DIconButton(this))
    , m_iconEffect(new QGraphicsOpacityEffect(m_iconWidget))
    , m_nameEffect(new QGraphicsOpacityEffect(m_nameLabel))
    , m_stateEffect(new QGraphicsOpacityEffect(m_stateLabel))
{
    initUi();
    initConnection();
}

QuickPanelWidget::~QuickPanelWidget() = default;

void QuickPanelWidget::setIcon(const QIcon &icon)
{
    m_iconWidget->setIcon(icon);
}

void QuickPanelWidget::setText(const QString &text)
{
    m_nameLabel->setText(text);
}

void QuickPanelWidget::setDescription(const QString &description)
{
    m_stateLabel->setText(description);
}

void QuickPanelWidget::setActive(bool active)
{
    m_active = active;
    m_iconWidget->setBackgroundRole(active ? QPalette::Highlight : QPalette::BrightText);
    updateOpacity(m_hovered);
}

void QuickPanelWidget::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        m_clickPoint = event->pos();
        break;
    default:
        break;
    }
    QWidget::mousePressEvent(event);
}

void QuickPanelWidget::mouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        if (m_clickPoint == event->pos())
            Q_EMIT panelClicked();
        break;
    default:
        break;
    }
    m_clickPoint = QPoint();
    return QWidget::mouseReleaseEvent(event);
}

void QuickPanelWidget::initUi()
{
    QWidget *labelWidget = new QWidget(this);

    DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, QFont::Normal);
    DToolTip::setToolTipShowMode(m_nameLabel, DToolTip::ShowWhenElided);
    m_nameLabel->setElideMode(Qt::ElideRight);
    m_nameLabel->setContentsMargins(0, 2, 0, 0);
    m_nameLabel->setForegroundRole(QPalette::BrightText);
    m_nameEffect->setOpacity(kNormalOpacity);
    m_nameLabel->setGraphicsEffect(m_nameEffect);

    DFontSizeManager::instance()->bind(m_stateLabel, DFontSizeManager::T10);
    DToolTip::setToolTipShowMode(m_stateLabel, DToolTip::ShowWhenElided);
    m_stateLabel->setElideMode(Qt::ElideRight);
    m_stateEffect->setOpacity(kNormalOpacity);
    m_stateLabel->setGraphicsEffect(m_stateEffect);

    QVBoxLayout *layout = new QVBoxLayout(labelWidget);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(0);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_stateLabel);

    // 图标
    m_iconWidget->setEnabledCircle(true);
    m_iconWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_iconWidget->setIconSize(IconSize);
    m_iconEffect->setOpacity(kNormalOpacity);
    m_iconWidget->setGraphicsEffect(m_iconEffect);
    m_iconWidget->setCheckable(false);
    m_iconWidget->setFixedSize(QSize(40, 40));
    m_iconWidget->setFocusPolicy(Qt::NoFocus);

    // 进入图标
    m_expandLabel->setIcon(DStyle::standardIcon(style(), DStyle::SP_ArrowEnter));
    m_expandLabel->setFlat(true);
    m_expandLabel->setFocusPolicy(Qt::NoFocus);

    m_expandLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 0, 10, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_iconWidget);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(labelWidget);
    mainLayout->addStretch(1);
    mainLayout->addWidget(m_expandLabel);

    setFixedSize(150, 60);
}

void QuickPanelWidget::initConnection()
{
    connect(m_iconWidget, &DFloatingButton::clicked, this, &QuickPanelWidget::iconClicked);
}

void QuickPanelWidget::updateOpacity(bool hover)
{
    // icon: when active, always 1.0; otherwise follow hover
    m_iconEffect->setOpacity((m_active || hover) ? kHoverOpacity : kNormalOpacity);
    // text: always follow hover regardless of active state
    m_nameEffect->setOpacity(hover ? kHoverOpacity : kNormalOpacity);
    m_stateEffect->setOpacity(hover ? kHoverOpacity : kNormalOpacity);
}

void QuickPanelWidget::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    updateOpacity(true);
    QWidget::enterEvent(event);
}

void QuickPanelWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    updateOpacity(false);
}

} // namespace wirelesscasting
} // namespace dde
