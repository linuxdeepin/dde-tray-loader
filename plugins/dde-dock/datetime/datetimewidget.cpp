// SPDX-FileCopyrightText: 2011 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "datetimewidget.h"
#include "constants.h"
#include "regionFormat.h"

#include <QApplication>
#include <QPainter>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <DFontSizeManager>

#define PLUGIN_STATE_KEY    "enable"
#define DEFAULT_WEEK_FORMAT "dddd"
#define SIMPLE_WEEK_FORMAT "ddd"

// 任务栏给时间插件的上下边距，见 PluginItem::updatePluginContentMargin() 里 datetime 的特殊处理
static const int DOCK_ITEM_VERTICAL_MARGIN = 2;
// 可用高度（任务栏高度 - 上边距 - 下边距）小于等于该值时，时间和日期并排显示成一行。
// 上下边距各 2，对应任务栏高度 40 及以下走一行，41 及以上走两行。
static const int SINGLE_LINE_MAX_AVAILABLE_HEIGHT = 36;
// 一行显示时，时间和日期之间的间距
static const int SINGLE_LINE_SPACING = 4;
// 其他插件的 hover 高度：16 的图标 + 上下各 4 的边距，
// 见 PluginItem::centralWidget() 和 PluginItem::updatePluginContentMargin()
static const int TRAY_ITEM_HOVER_HEIGHT = 24;
// 一行显示时控件自身的最小高度：任务栏给时间插件的上下边距是 2，
// 控件撑到 24 - 2*2 后，插件项（也就是 hover）的高度就和其他插件一致
static const int SINGLE_LINE_MIN_HEIGHT = TRAY_ITEM_HOVER_HEIGHT - DOCK_ITEM_VERTICAL_MARGIN * 2;
// 两行显示时压掉的高度：label 的文本和边框之间本身有间距，压掉后上下两行更紧凑
static const int TWO_LINE_SQUEEZE_HEIGHT = 4;

DWIDGET_USE_NAMESPACE

DatetimeWidget::DatetimeWidget(RegionFormat* regionFormat, QWidget *parent)
    : QWidget(parent)
    , m_timeLabel(new QLabel(this))
    , m_dateLabel(new QLabel(this))
    , m_apLabel(new QLabel(this))
    , m_spacerItem(new QWidget(this))
    , m_layout(nullptr)
    , m_24HourFormat(false)
    , m_singleLineLayout(false)
    , m_fashionMode(false)
    , m_weekdayFormatType(0)
    , m_shortDateFormat("yyyy-MM-dd")
    , m_weekFormat(DEFAULT_WEEK_FORMAT)
    , m_dockSize(QSize(1920, 37))
    , m_timedateInter(new Timedate1Inter("org.deepin.dde.Timedate1", "/org/deepin/dde/Timedate1", QDBusConnection::sessionBus(), this))
    , m_regionFormat(regionFormat)
{
    initUI();

    setWeekdayFormat(m_timedateInter->weekdayFormat());
    connect(m_timedateInter, &Timedate1Inter::WeekdayFormatChanged, this, &DatetimeWidget::setWeekdayFormat);

    m_24HourFormat = m_regionFormat->is24HourFormat();
    adjustFontSize();
    updateDateTimeString();
    installEventFilter(this);

    connect(m_regionFormat, &RegionFormat::longDateFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::shortTimeFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::shortDateFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::longTimeFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::localeNameChanged, this, &DatetimeWidget::updateDateTime);
}

void DatetimeWidget::set24HourFormat(const bool value)
{
    if (m_24HourFormat == value) {
        return;
    }

    m_24HourFormat = value;
    m_regionFormat->sync24HourFormatConfig(value);
    update();

    if (isVisible()) {
        emit requestUpdateGeometry();
    }
}

/**
 * @brief DatetimeWidget::setWeekdayFormat 根据类型设置周显示格式
 * @param type 自定义类型
 */
void DatetimeWidget::setWeekdayFormat(int type)
{
    if (m_weekdayFormatType == type)
        return;

    m_weekdayFormatType = type;
    updateWeekdayFormat();
    updateDateTimeString();
}

/**
 * @brief DatetimeWidget::updateWeekdayFormat 更新周的显示格式
 */
void DatetimeWidget::updateWeekdayFormat()
{
    if (1 == m_weekdayFormatType) {
        m_weekFormat = SIMPLE_WEEK_FORMAT;
    } else {
        m_weekFormat = DEFAULT_WEEK_FORMAT;
    }
}

void DatetimeWidget::setRegionFormat(RegionFormat *newRegionFormat)
{
    m_regionFormat = newRegionFormat;
}

QSize DatetimeWidget::sizeHint() const
{
    QSize hint = QWidget::sizeHint();

    if (!m_singleLineLayout) {
        // 两行显示时把 label 自带的文本间距压掉，让时间和日期贴得更紧
        hint.setHeight(qMax(0, hint.height() - TWO_LINE_SQUEEZE_HEIGHT));
        return hint;
    }

    // 一行显示时控件只有一行文字高，插件项会比其他插件矮一圈、hover 也跟着变小，
    // 这里撑到和其他插件一致的 hover 高度
    hint.setHeight(qMax(hint.height(), SINGLE_LINE_MIN_HEIGHT));
    return hint;
}

/**
 * @brief DatetimeWidget::updateWeekdayTimeString 更新任务栏时间标签的显示
 */
void DatetimeWidget::updateDateTimeString()
{
    QLocale locale(m_regionFormat->getLocaleName());

    QString longDateFormat = m_regionFormat->getLongDateFormat();
    longDateFormat.replace(DEFAULT_WEEK_FORMAT, m_weekFormat, Qt::CaseInsensitive);
    m_dateTime = locale.toString(QDateTime::currentDateTime(), longDateFormat + " " + m_regionFormat->getLongTimeFormat());

    QDateTime current = QDateTime::currentDateTime();

    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    QString timeStr, dateString;
    if (position == Dock::Bottom || position == Dock::Top) {
        QString timeFormat = m_regionFormat->getShortTimeFormat();
        timeStr = locale.toString(current, timeFormat);
        dateString = locale.toString(current.date(), m_regionFormat->getShortDateFormat());

        m_timeLabel->setText(timeStr);
        m_dateLabel->setText(dateString);
    } else {
        if (!m_24HourFormat) {
            QString apText = locale.toString(current, "AP");
            m_apLabel->setText(apText);

            QString timeFormat = m_regionFormat->getShortTimeFormat();
            timeFormat.replace("AP", "");
            timeFormat.replace(" ", "");
            timeStr = locale.toString(current.time(), timeFormat);
        } else {
            timeStr = locale.toString(current.time(), m_regionFormat->getShortTimeFormat());
        }

        m_timeLabel->setText(timeStr);
        dateString = locale.toString(current.date(), m_regionFormat->getShortDateFormat());
        m_dateLabel->setText(dateString);
    }
}

void DatetimeWidget::updateDateTime()
{
    m_24HourFormat = m_regionFormat->is24HourFormat();
    adjustUI();
    adjustLayout();
    updateDateTimeString();
    update();

    if (isVisible()) {
        emit requestUpdateGeometry();
    }
}

void DatetimeWidget::adjustFontSize()
{
    const int MAX_DISTANCE = 999;
    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    int validDistance = m_dockSize.height() / devicePixelRatioF();
    if (position == Dock::Left || position == Dock::Right) {
        validDistance = m_dockSize.width() / devicePixelRatioF();
    }

    // dock position changed(from bottom to left), new dock size is not update, use bottom width to adjust font size,
    // then assert in timeFontSize != 0 && dateFontSize != 0
    if (validDistance > MAX_DISTANCE) {
        return;
    }

    // 根据时间和日期字体大小的跨度，将dock栏大小分为不同的区间，每个区域对应不同的字体大小，然后通过判断dock栏大小所在的区间来设置字体大小
    // 如果任务栏小于37，则字体始终取最小值；如果任务栏大于61，则字体始终取最大值；如果任务栏在37和61之间，则字体大小随任务栏大小线性变化
    static const QMap<int, QPair<int, int>> fontSizeMap {
        {0, {13, 10}},
        {37, {13, 10}},
        {40, {14, 10}},
        {43, {14, 10}},
        {46, {15, 11}},
        {49, {16, 11}},
        {52, {17, 12}},
        {55, {18, 12}},
        {58, {19, 13}},
        {61, {20, 14}},
        {MAX_DISTANCE, {20, 14}}
    };

    int timeFontSize = 0;
    int dateFontSize = 0;
    QList<int> distances = fontSizeMap.keys();
    for (int i = 0; i < distances.size() - 1; ++i) {
        if (validDistance >= distances.at(i) && validDistance < distances.at(i + 1)) {
            timeFontSize = fontSizeMap.value(distances.at(i)).first;
            dateFontSize = fontSizeMap.value(distances.at(i)).second;
            break;
        }
    }

    Q_ASSERT(timeFontSize != 0 && dateFontSize != 0);

    QFont timeFont = m_timeLabel->font();
    timeFont.setPixelSize(timeFontSize);
    m_timeLabel->setFont(timeFont);
    m_apLabel->setFont(timeFont);

    QFont dateFont = m_dateLabel->font();
    dateFont.setPixelSize(dateFontSize);
    m_dateLabel->setFont(dateFont);

    // 字号变了要重新按基线对齐
    alignSingleLineBaseline();
}

/**
 * @brief DatetimeWidget::alignSingleLineBaseline 一行显示时按基线对齐时间和日期
 *
 * Qt 的垂直居中是把各自的文本框居中，基线位置为「居中位置 +（ascent - descent）/ 2」，
 * 字号大的一方 ascent - descent 更大，基线会更靠下，看起来时间比日期低一点。
 * 这里给字号小的一方加一个顶部边距，把两者的基线拉平。
 */
void DatetimeWidget::alignSingleLineBaseline()
{
    m_timeLabel->setContentsMargins(0, 0, 0, 0);
    m_dateLabel->setContentsMargins(0, 0, 0, 0);

    if (!m_singleLineLayout) {
        return;
    }

    const QFontMetrics timeMetrics(m_timeLabel->font());
    const QFontMetrics dateMetrics(m_dateLabel->font());
    const int delta = (timeMetrics.ascent() - timeMetrics.descent())
        - (dateMetrics.ascent() - dateMetrics.descent());

    if (delta > 0) {
        m_dateLabel->setContentsMargins(0, delta, 0, 0);
    } else if (delta < 0) {
        m_timeLabel->setContentsMargins(0, -delta, 0, 0);
    }
}

void DatetimeWidget::resizeEvent(QResizeEvent *event)
{
    if (isVisible())
        emit requestUpdateGeometry();

    QWidget::resizeEvent(event);
}

bool DatetimeWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::DevicePixelRatioChange && watched == this) {
        adjustFontSize();
    }

    return QWidget::eventFilter(watched, event);
}

void DatetimeWidget::setDockPanelSize(const QSize &dockSize)
{
    // 任务栏高度最小是37，小于37说明在隐藏和显示动画中
    if (dockSize.width() < 37) {
        return;
    }
    if (dockSize.height() < 37) {
        return;
    }

    if (m_dockSize != dockSize) {
        m_dockSize = dockSize;
        adjustFontSize();
        adjustLayout();
        update();
    }

    Q_EMIT requestUpdateGeometry();
}

void DatetimeWidget::setFashionMode(bool fashionMode)
{
    if (m_fashionMode == fashionMode) {
        return;
    }

    m_fashionMode = fashionMode;
    adjustLayout();
}

void DatetimeWidget::dockPositionChanged()
{
    // 等待位置变换完成后再更新
    QTimer::singleShot(300, this, [this]{
        updateDateTime();
        adjustFontSize();
    });

    adjustUI();
    adjustLayout();
}

void DatetimeWidget::initUI()
{
    setContentsMargins(0, 0, 0, 0);

    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_dateLabel->setAlignment(Qt::AlignCenter);
    m_apLabel->setAlignment(Qt::AlignCenter);

    m_timeLabel->setContentsMargins(0, 0, 0, 0);
    m_dateLabel->setContentsMargins(0, 0, 0, 0);
    m_apLabel->setContentsMargins(0, 0, 0, 0);

    // 当任务栏在左/右时，时间文本和日期的间距
    m_spacerItem->setFixedSize(10, 5);

    m_timeLabel->setForegroundRole(QPalette::BrightText);
    m_apLabel->setForegroundRole(QPalette::BrightText);

    auto *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_apLabel);
    layout->addWidget(m_timeLabel);
    layout->addWidget(m_spacerItem);
    layout->addWidget(m_dateLabel);

    m_layout = layout;
    setLayout(layout);

    adjustUI();
    adjustLayout();
}

void DatetimeWidget::adjustUI()
{
    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    if (position == Dock::Left || position == Dock::Right) {
        if (!m_24HourFormat) {
            m_spacerItem->setVisible(true);
            m_apLabel->setVisible(true);
            return;
        }
    }

    m_spacerItem->setVisible(false);
    m_apLabel->setVisible(false);
}

int DatetimeWidget::availableHeight() const
{
    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    const int dockLength = (position == Dock::Left || position == Dock::Right)
        ? m_dockSize.width()
        : m_dockSize.height();

    return qMax(0, qRound(dockLength / devicePixelRatioF()) - DOCK_ITEM_VERTICAL_MARGIN * 2);
}

bool DatetimeWidget::shouldUseSingleLine() const
{
    // 一行显示只在时尚模式下生效
    if (!m_fashionMode) {
        return false;
    }

    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    // 左/右任务栏受限的是宽度，并排显示只会把任务栏撑宽，因此只在上/下任务栏生效
    if (position != Dock::Top && position != Dock::Bottom) {
        return false;
    }

    return availableHeight() <= SINGLE_LINE_MAX_AVAILABLE_HEIGHT;
}

/**
 * @brief DatetimeWidget::adjustLayout 按可用高度在两行（时间在上、日期在下）
 * 和一行（时间在前，间隔 SINGLE_LINE_SPACING，日期在后）之间切换
 */
void DatetimeWidget::adjustLayout()
{
    const bool singleLine = shouldUseSingleLine();
    if (m_singleLineLayout == singleLine) {
        return;
    }

    m_singleLineLayout = singleLine;
    // AP 和竖排用的间隔占位在 adjustUI() 里已经隐藏，QBoxLayout 会跳过隐藏的部件，
    // 所以 spacing 就是时间和日期之间的实际间距。
    m_layout->setDirection(singleLine ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    m_layout->setSpacing(singleLine ? SINGLE_LINE_SPACING : 0);
    alignSingleLineBaseline();

    updateGeometry();
    // 不判断 isVisible()：模式或尺寸变化时任务栏可能处于隐藏状态，
    // 这里必须把新的尺寸同步出去，否则再显示出来时插件项还是旧的大小
    Q_EMIT requestUpdateGeometry();
}