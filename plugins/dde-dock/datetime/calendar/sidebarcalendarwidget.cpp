// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "sidebarcalendarwidget.h"
#include "constants.h"
#include "units.h"
#include "jumpcalendarbutton.h"
#include "pagebutton.h"
#include "regionFormat.h"

#include <DPaletteHelper>
#include <DGuiApplicationHelper>
#include <DFontSizeManager>
#include <DRegionMonitor>
#include <DToolTip>

#include <QMouseEvent>
#include <QScreen>
#include <QJsonObject>
#include <QFontMetrics>

// 初一的农历月名颜色（蓝色标记）
static const QColor LunarMonthBlue(0, 129, 255);
// 调休角标颜色：休-绿色，班-红色
static const QColor HolidayRestColor(46, 160, 67);
static const QColor HolidayWorkColor(233, 68, 68);

SidebarCalendarWidget::SidebarCalendarWidget(RegionFormat *regionFormat, QWidget* parent)
    : QWidget(parent)
    , m_regionFormat(regionFormat)
    , m_manager(CalendarManager::instance())
    , m_headWidget(new QWidget(this))
    , m_weekWidget(new CWeekWidget(this))
    , m_keyWidget(new QWidget(this))
    , m_keyLayout(new QGridLayout)
    , m_dateLabel(new QLabel(this))
    , m_nextPage(new PageButton(this))
    , m_previousPage(new PageButton(this))
    , m_firstday(Qt::Sunday)
    , m_dateTitleWidget(new DateTitleWidget(this))
    , m_weekLabel(new DLabel(this))
    , m_leftdateLabel(new DLabel(this))
    , m_lunarLabel(new DLabel(this))
    , m_lunarDetailLabel(new DLabel(this))
    , m_jumpCalendarButton(new JumpCalendarButton(this))
    , m_bakTodayBtn(new DCommandLinkButton(tr("Today"), this))
    , m_leftWidget(new QWidget(this))
    , m_rightWidget(new QWidget(this))
    , m_selectedData(QDate())
    , m_weekdayFormat(CalendarManager::WeekDayFormat_1)
    , m_deltaSum(0)
    , m_timer(new QTimer(this))
{
    initView();
    initConnection();
    initData();
}

void SidebarCalendarWidget::initView()
{
    setFocusPolicy(Qt::NoFocus);
    // 面板尺寸：左侧日期详情面板宽度 + 6 行 7 列日期网格，再加上右侧布局的边距。
    // 全部使用逻辑像素，缩放时由 Qt 按系统缩放比例自动放大
    const int gridW = DDECalendar::CalendarCellWidth * DAYS_INWEEK;
    const int gridH = DDECalendar::CalendarCellHeight * WEEKS_NUM;
    const int layoutHMargin = 5 + 15;   // rightLayout 左右边距
    const int layoutVMargin = 5 + 15;   // rightLayout 上下边距
    const int headHeight = 30;          // m_headWidget 最小高度
    const int weekHeight = 36;          // m_weekWidget 最小高度
    const int headSpacing = 6;          // 头部上方的间隔
    setFixedSize(DDECalendar::CalendarPanelLeftWidth + gridW + layoutHMargin,
                 layoutVMargin + headSpacing + headHeight + weekHeight + gridH);
    QVBoxLayout *leftLayout = new QVBoxLayout;
    QVBoxLayout *rightLayout = new QVBoxLayout;

    DFontSizeManager::instance()->bind(m_weekLabel, DFontSizeManager::T4, 70);
    DFontSizeManager::instance()->bind(m_leftdateLabel, DFontSizeManager::T4, 70);
    DFontSizeManager::instance()->bind(m_lunarLabel, DFontSizeManager::T4, 62);
    DFontSizeManager::instance()->bind(m_lunarDetailLabel, DFontSizeManager::T5, 62);

    m_jumpCalendarButton->setIcon(QIcon::fromTheme("open-calendar"));
    m_jumpCalendarButton->setDescription(tr("Open the calendar"));
    m_jumpCalendarButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_jumpCalendarButton->setFixedWidth(m_jumpCalendarButton->sizeHint().width());

    m_lunarDetailLabel->setWordWrap(true);

    leftLayout->setSpacing(0);
    leftLayout->addStretch(1);
    leftLayout->addWidget(m_dateTitleWidget);
    leftLayout->addStretch(2);
    leftLayout->addWidget(m_weekLabel);
    leftLayout->addWidget(m_leftdateLabel);
    leftLayout->addSpacing(10);
    leftLayout->addWidget(m_lunarLabel);
    leftLayout->addWidget(m_lunarDetailLabel);
    leftLayout->addSpacing(5);
    leftLayout->addWidget(m_jumpCalendarButton);
    leftLayout->setContentsMargins(20, 0, 10, 20);

    DFontSizeManager::instance()->bind(m_dateLabel, DFontSizeManager::T3, 65);
    m_nextPage->setIcon(DStyle().standardIcon(QStyle::SP_ArrowRight));
    m_previousPage->setIcon(DStyle().standardIcon(QStyle::SP_ArrowLeft));

    m_nextPage->setFocusPolicy(Qt::NoFocus);
    m_previousPage->setFocusPolicy(Qt::NoFocus);
    m_bakTodayBtn->setFocusPolicy(Qt::NoFocus);
    m_bakTodayBtn->setObjectName("BakTodayBtn");

    DFontSizeManager::instance()->bind(m_bakTodayBtn, DFontSizeManager::T6);

    QHBoxLayout* headLayout = new QHBoxLayout;
    headLayout->setContentsMargins(8, 0, 0, 0);
    headLayout->setAlignment(Qt::AlignCenter);
    headLayout->addWidget(m_dateLabel);
    headLayout->addStretch();
    headLayout->addWidget(m_bakTodayBtn);
    headLayout->addWidget(m_previousPage);
    headLayout->addWidget(m_nextPage);

    m_headWidget->setLayout(headLayout);
    m_headWidget->setMinimumHeight(30);

    m_weekWidget->setAutoFontSizeByWindow(false);
    m_weekWidget->setObjectName("WeekWidget");
    m_weekWidget->setFirstDay(Qt::Sunday);
    m_weekWidget->setMinimumHeight(weekHeight);
    DFontSizeManager::instance()->bind(m_weekWidget, DFontSizeManager::T5, 60);

    m_keyLayout->setContentsMargins(0, 0, 0, 0);
    m_keyLayout->setSpacing(0);
    m_keyWidget->setLayout(m_keyLayout);
    // 日期网格固定为 6 行 7 列，单元格尺寸由 constants.h 决定，行列等分
    m_keyWidget->setFixedSize(gridW, gridH);
    //循坏实例化6*7个日期按键
    for (int i = 0; i < MONTH_METRIC_NUM; ++i) {
        SidebarCalendarKeyButton* button = new SidebarCalendarKeyButton();
        button->setFocusPolicy(Qt::NoFocus);
        m_keyButtonList.push_back(button);
        m_keyLayout->addWidget(button, i / DAYS_INWEEK, i % DAYS_INWEEK);
        connect(button, &SidebarCalendarKeyButton::clicked, this, &SidebarCalendarWidget::onKeyButtonClicked);
    }

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addSpacing(6);
    rightLayout->addWidget(m_headWidget);
    rightLayout->addWidget(m_weekWidget, 1);
    rightLayout->addWidget(m_keyWidget, 6);
    rightLayout->setContentsMargins(5, 5, 15, 15);

    // 为设置左右比例和固定大小，使用widget包起来
    m_leftWidget->setLayout(leftLayout);
    m_rightWidget->setLayout(rightLayout);
    m_leftWidget->setMinimumWidth(DDECalendar::CalendarPanelLeftWidth);
    updateBackground();

    mainLayout->addWidget(m_leftWidget);
    mainLayout->addWidget(m_rightWidget);
    setLayout(mainLayout);

    m_timer->setInterval(1000);
    m_timer->start();
}

/**
 * @brief SidebarCalendarWidget::initConnection
 * 初始化信号链接
 */
void SidebarCalendarWidget::initConnection()
{
    connect(m_nextPage, &PageButton::clicked, this, &SidebarCalendarWidget::onNextPageClicked);
    connect(m_previousPage, &PageButton::clicked, this, &SidebarCalendarWidget::onPreviousPageClicked);
    connect(m_manager, &CalendarManager::sidebarFirstDayChanged, this, &SidebarCalendarWidget::onFirstDayChanged);
    // connect(m_manager, &CalendarManager::dateFormatChanged, this, &SidebarCalendarWidget::onDateFormatChanged);
    connect(m_regionFormat, &RegionFormat::shortDateFormatChanged, this, &SidebarCalendarWidget::onShortDateFormatChanged);
    connect(m_manager, &CalendarManager::weekDayFormatChanged, this, &SidebarCalendarWidget::onWeekDayFormatChanged);
    connect(m_bakTodayBtn, &DCommandLinkButton::clicked, this, &SidebarCalendarWidget::backToday);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &SidebarCalendarWidget::updateBackground);
    connect(m_jumpCalendarButton, &JumpCalendarButton::clicked, this, &SidebarCalendarWidget::jumpButtonClicked);
    connect(m_timer, &QTimer::timeout, this, [this] {
        if (QTime::currentTime().hour() != QTime(0, 0, 0).hour()
                || QTime::currentTime().minute() != QTime(0, 0, 0).minute()
                || QTime::currentTime().second() != QTime(0, 0, 0).second())
            return;

        backToday();
    });
    connect(LunarManager::instace(), &LunarManager::lunarInfoReady, [this](const CaHuangLiDayInfo &info) {
        m_lunarLabel->setText(tr("Lunar") + info.mLunarMonthName + info.mLunarDayName);
        m_lunarDetailLabel->setText(info.mGanZhiYear + tr("y") + "【"
                                    + info.mZodiac + tr("y") + "】"
                                    + info.mGanZhiMonth + tr("m") + " "
                                    + info.mGanZhiDay + tr("d"));
    });
    connect(LunarManager::instace(), &LunarManager::monthInfoReady, this, &SidebarCalendarWidget::onMonthInfoReady);
}

/**
 * @brief SidebarCalendarWidget::initData
 * 初始化数据
 */
void SidebarCalendarWidget::initData()
{
    m_firstday = m_manager->firstDayOfWeek();
    m_weekdayFormat = m_manager->weekDayFormat();
    QDate date = QDate::currentDate();
    setSelectedDate(date);
}

/**
 * @brief SidebarCalendarWidget::setDate
 * 设置显示的日期
 * @param date 将被选择显示的日期
 */
void SidebarCalendarWidget::setSelectedDate(const QDate &date)
{
    if(m_selectedData == date)
        return;

    m_selectedData = date;
    setDate(date);
}

/**
 * @brief SidebarCalendarWidget::setKeyDate
 * 根据选中的日期设置按钮显示的日期范围
 * @param date 待显示的月日期
 */
void SidebarCalendarWidget::setDate(const QDate &date)
{
    if (!withinTimeFrame(date)) {
        return;
    }

    m_bakTodayBtn->setVisible(date.month() != QDate::currentDate().month() || date.year() != QDate::currentDate().year());

    m_dateTitleWidget->setDateLabelText("/ " + formatedMonth(static_cast<Month>(date.month())), date.day());
    m_weekLabel->setText(formatedWeekDay(static_cast<WeekDay>(date.dayOfWeek()), m_weekdayFormat));
    m_leftdateLabel->setText(date.toString(m_regionFormat->originShortDateFormat()));
    m_dateLabel->setText(formatedMonth(static_cast<Month>(date.month())));
    LunarManager::instace()->asyncRequestLunar(date);

    if (m_displayedMonth != date) {
        m_displayedMonth = date;
    }

    //获取当月第一天
    QDate firstDateInMonth = QDate(date.year(), date.month(), 1);
    int day = firstDateInMonth.dayOfWeek();

    //计算当前月日历第一天该显示的时间
    QDate dateInMonth = firstDateInMonth.addDays(-(day - m_firstday + DAYS_INWEEK) % DAYS_INWEEK);

    //网格首尾可能落在前后月，把网格覆盖到的月份（最多 3 个）的农历/节气/节假日/调休数据一起请求
    m_gridMonths.clear();
    const QDate lastVisibleDate = dateInMonth.addDays(MONTH_METRIC_NUM - 1);
    for (QDate month(dateInMonth.year(), dateInMonth.month(), 1); month <= lastVisibleDate; month = month.addMonths(1)) {
        m_gridMonths.append(month);
        LunarManager::instace()->requestMonth(month);
    }
    for (SidebarCalendarKeyButton* btn : m_keyButtonList) {
        btn->setDate(dateInMonth);

        btn->setIsToday(dateInMonth == QDate::currentDate());
        btn->setIsThisMonth(dateInMonth.month() == m_displayedMonth.month());
        btn->setIsSelected(dateInMonth == m_selectedData);

        const auto infoIt = m_dayInfos.constFind(dateInMonth);
        if (infoIt != m_dayInfos.constEnd())
            btn->setDayInfo(infoIt.value());
        else
            btn->clearDayInfo();

        dateInMonth = dateInMonth.addDays(1);
    }
    update();
}

void SidebarCalendarWidget::onMonthInfoReady(const QDate &month, const QMap<QDate, CalendarDayInfo> &infos)
{
    // 已翻页、或不属于当前网格的月份，忽略过期数据
    if (!m_gridMonths.contains(month))
        return;

    for (auto it = infos.constBegin(); it != infos.constEnd(); ++it)
        m_dayInfos.insert(it.key(), it.value());

    for (SidebarCalendarKeyButton *btn : m_keyButtonList) {
        const auto infoIt = m_dayInfos.constFind(btn->date());
        if (infoIt != m_dayInfos.constEnd())
            btn->setDayInfo(infoIt.value());
        else
            btn->clearDayInfo();
    }
}

void SidebarCalendarWidget::updateBackground()
{
    auto pa = palette();
    auto backgroundColor = pa.window().color();
    backgroundColor.setAlphaF(0.02);
    pa.setColor(QPalette::Window, backgroundColor);
    m_leftWidget->setPalette(pa);
    m_leftWidget->setAutoFillBackground(true);
}

QString SidebarCalendarWidget::formatedDateType(CalendarManager::DateFormat format)
{
    switch (format) {
        case CalendarManager::DateFormat_1: return "yyyy/M/d";
        case CalendarManager::DateFormat_2: return "yyyy-M-d";
        case CalendarManager::DateFormat_3: return "yyyy.M.d";
        case CalendarManager::DateFormat_4: return "yyyy/MM/dd";
        case CalendarManager::DateFormat_5: return "yyyy-MM-dd";
        case CalendarManager::DateFormat_6: return "yyyy.MM.dd";
        case CalendarManager::DateFormat_7: return "MM.dd.yyyy";
        case CalendarManager::DateFormat_8: return "dd.MM.yyyy";
        case CalendarManager::DateFormat_9: return "yy/M/d";
        case CalendarManager::DateFormat_10: return "yy-M-d";
        case CalendarManager::DateFormat_11: return "yy.M.d";
        default: return "yyyy-MM-dd";
    }
}

QString SidebarCalendarWidget::formatedWeekType(CalendarManager::WeekDayFormat format)
{
    switch (format) {
        case CalendarManager::WeekDayFormat_1: return "dddd";
        default: return "ddd";
    }
}

/**
 * @brief SidebarCalendarWidget::onKeyButtonClicked
 * 日期按键点击事件
 * @param date 事件发送者的日期
 */
void SidebarCalendarWidget::onKeyButtonClicked(const QDate &date)
{
    const QDate &selectedDate = date;
    if (!withinTimeFrame(selectedDate)) {
        return;
    }
    setSelectedDate(selectedDate);
    if (selectedDate.year() == m_displayedMonth.year() && selectedDate.month() == m_displayedMonth.month()) {
        //未切换月份，只刷新界面显示
        update();
    } else {
        //已切换月份，重新设置选中的日期
        setSelectedDate(selectedDate);
    }
}

/**
 * @brief SidebarCalendarWidget::onNextPageClicked
 * 下一页切换事件
 */
void SidebarCalendarWidget::onNextPageClicked()
{
    //设置显示月份日期
    const QDate &date = m_displayedMonth.addMonths(+1);
    //设置显示的日期范围
    m_selectedData = date;
    setDate(date);
}

/**
 * @brief SidebarCalendarWidget::onPreviousPageClicked
 * 上一页切换事件
 */
void SidebarCalendarWidget::onPreviousPageClicked()
{
    //设置显示月份日期
    QDate date = m_displayedMonth.addMonths(-1);
    //设置显示的日期范围
    m_selectedData = date;
    setDate(date);
}

void SidebarCalendarWidget::onFirstDayChanged(int value)
{
    m_firstday = value;
    setDate(m_selectedData);
}

void SidebarCalendarWidget::onDateFormatChanged(CalendarManager::DateFormat format)
{
    m_leftdateLabel->setText(QDate::currentDate().toString(formatedDateType(format)));
}

void SidebarCalendarWidget::onWeekDayFormatChanged(CalendarManager::WeekDayFormat format)
{
    if (m_weekdayFormat == format)
        return;

    m_weekdayFormat = format;
    m_weekLabel->setText(formatedWeekDay(static_cast<WeekDay>(QDate::currentDate().dayOfWeek()), format));
}

void SidebarCalendarWidget::backToday()
{
    const QDate &date = QDate::currentDate();
    setSelectedDate(date);
}

void SidebarCalendarWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
}

void SidebarCalendarWidget::showEvent(QShowEvent *event)
{
    bool showLunar = QLocale::system().language() == QLocale::Language::Chinese;
    m_lunarLabel->setVisible(showLunar);
    m_lunarDetailLabel->setVisible(showLunar);
    backToday();
    QWidget::showEvent(event);
}

void SidebarCalendarWidget::wheelEvent(QWheelEvent *event)
{
    m_deltaSum += event->angleDelta().y();
    if (m_deltaSum >= TURNPAGE_DELTA) {
        Q_EMIT m_previousPage->clicked();
        m_deltaSum = 0;
    } else if (m_deltaSum <= -TURNPAGE_DELTA) {
        Q_EMIT m_nextPage->clicked();
        m_deltaSum = 0;
    }
    QWidget::wheelEvent(event);
}

bool SidebarCalendarWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationFontChange) {
        m_jumpCalendarButton->setFixedWidth(m_jumpCalendarButton->sizeHint().width());
        update();
    }

    return QWidget::event(event);
}

QString SidebarCalendarWidget::formatedWeekDay(WeekDay weekDay, CalendarManager::WeekDayFormat format)
{
    if (QLocale::system().language() == QLocale::English)
        format = CalendarManager::WeekDayFormat_1;

    if (format == CalendarManager::WeekDayFormat_1)
        switch (weekDay) {
            case Monday: return tr("Monday");
            case Tuesday: return tr("Tuesday");
            case Wednesday: return tr("Wednesday");
            case Thursday: return tr("Thursday");
            case Friday: return tr("Friday");
            case Saturday: return tr("Saturday");
            case Sunday: return tr("Sunday");
            default: return QString();
        }
    else
        switch (weekDay) {
            case Monday: return tr("monday");
            case Tuesday: return tr("tuesday");
            case Wednesday: return tr("wednesday");
            case Thursday: return tr("thursday");
            case Friday: return tr("friday");
            case Saturday: return tr("saturday");
            case Sunday: return tr("sunday");
            default: return QString();
        }
}

QString SidebarCalendarWidget::formatedMonth(Month month)
{
    switch (month) {
        case Jan: return tr("Jan");
        case Feb: return tr("Feb");
        case Mar: return tr("Mar");
        case Apr: return tr("Apr");
        case May: return tr("May");
        case Jun: return tr("Jun");
        case Jul: return tr("Jul");
        case Aug: return tr("Aug");
        case Sept: return tr("Sept");
        case Oct: return tr("Oct");
        case Nov: return tr("Nov");
        case Dec: return tr("Dec");
        default:return QString();
    }
}

void SidebarCalendarWidget::onShortDateFormatChanged()
{
    m_leftdateLabel->setText(QDate::currentDate().toString(m_regionFormat->originShortDateFormat()));
}

SidebarCalendarKeyButton::SidebarCalendarKeyButton(QWidget* parent)
    : QPushButton(parent)
    , m_pressed(false)
    , m_date(QDate())
    , m_isToday(false)
    , m_isThisMonth(false)
    , m_isSelected(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(40, 40);
}

void SidebarCalendarKeyButton::setDayInfo(const CalendarDayInfo &info)
{
    // 非中文环境不显示农历/节气/节日/调休（与左侧农历面板的显隐规则一致，数据本身也只有中文）
    if (QLocale::system().language() != QLocale::Language::Chinese) {
        clearDayInfo();
        return;
    }

    m_dayInfo = info;
    m_hasDayInfo = true;
    bool isLunarMonthName = false;
    setToolTip(resolveInfoText(isLunarMonthName));
    update();
}

void SidebarCalendarKeyButton::clearDayInfo()
{
    m_hasDayInfo = false;
    m_dayInfo = CalendarDayInfo();
    setToolTip(QString());
    update();
}

/**
 * @brief SidebarCalendarKeyButton::onClick
 * 点击事件
 */
void SidebarCalendarKeyButton::onClick()
{
    emit clicked(date());
}

void SidebarCalendarKeyButton::mousePressEvent(QMouseEvent* event)
{
    //判断鼠标左键按下事件
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
    }
    QWidget::mousePressEvent(event);
}

void SidebarCalendarKeyButton::mouseReleaseEvent(QMouseEvent* event)
{
    //判断鼠标左键释放事件
    if (m_pressed && event->button() == Qt::LeftButton && rect().contains(event->pos())) {
        onClick();
        m_pressed = false;
    } else if (event->button() == Qt::LeftButton) {
        m_pressed = false;
    }

    QWidget::mouseReleaseEvent(event);
}

void SidebarCalendarKeyButton::mouseMoveEvent(QMouseEvent* event)
{
    //判断鼠标左键移动事件
    if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
        m_pressed = true;
    } else if (event->button() == Qt::LeftButton) {
        m_pressed = false;
    }
}

bool SidebarCalendarKeyButton::event(QEvent *event)
{
    // 单元格信息（农历/节气/节日）显示不全时，悬停用 tooltip 显示完整文本
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QToolTip::showText(helpEvent->globalPos(), toolTip(), this);
        event->accept();
        return true;
    }
    return QPushButton::event(event);
}

void SidebarCalendarKeyButton::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal w = width();
    const qreal h = height();

    QStyleOption opt;
    opt.initFrom(this);
    auto pa = DPaletteHelper::instance()->palette(this);

    //单元格绘制区域，内缩 0.5px 避免圆角被裁切
    const QRectF cellRect(0.5, 0.5, w - 1, h - 1);
    const QMargins innerMargin(RECT_MARGINS, RECT_MARGINS, RECT_MARGINS, RECT_MARGINS);

    QColor dayColor = pa.windowText().color();
    QColor infoColor = dayColor;

    if (m_isToday) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(pa.highlight());
        //绘制高亮背景
        painter.drawRoundedRect(cellRect.marginsRemoved(innerMargin), ROUND_CORNER, ROUND_CORNER);
        //设置高亮下的字体颜色
        dayColor = pa.highlightedText().color();
        infoColor = dayColor;
    } else if (m_isSelected) {
        QPen pen;
        pen.setColor(pa.highlight().color());
        pen.setWidth(RECT_PENWIDTH);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        //绘制选中边框
        painter.drawRoundedRect(cellRect.adjusted(RECT_PENWIDTH,
                                                  RECT_PENWIDTH,
                                                  -RECT_PENWIDTH,
                                                  -RECT_PENWIDTH)
                                , ROUND_CORNER, ROUND_CORNER);
    } else if (opt.state & QStyle::StateFlag::State_MouseOver) {
        painter.setPen(Qt::NoPen);
        auto color = pa.windowText().color();
        color.setAlphaF(0.1);
        painter.setBrush(color);
        painter.drawRoundedRect(cellRect.marginsRemoved(innerMargin), ROUND_CORNER, ROUND_CORNER);
    }

    //非本月日期整体淡化，但农历信息仍然保持可读
    if (!m_isThisMonth && !m_isToday)
        painter.setOpacity(0.45);

    //上半区：公历日期，有休/班角标时和角标并排整体居中
    QFont dayFont;
    dayFont.setPixelSize(DDECalendar::FontSizeFourteen);
    const QFontMetricsF dayMetrics(dayFont);
    painter.setFont(dayFont);
    painter.setPen(dayColor);

    const QString dayStr = QString::number(m_date.day());
    const bool showBadge = (m_hasDayInfo && m_dayInfo.holidayStatus != 0);
    const qreal sideMargin = 4.0;      //农历/角标与单元格左右边缘的间距
    const qreal blockGap = 3.0;        //公历与农历之间的间距
    const qreal badgeWidth = 14.0;     //休/班角标宽度
    const qreal dayBlockHeight = dayMetrics.height();

    qreal dayBlockWidth = dayMetrics.horizontalAdvance(dayStr);
    if (showBadge)
        dayBlockWidth += badgeWidth;

    //下半区：农历/节气/节日/农历月名
    bool isLunarMonthName = false;
    const QString infoText = resolveInfoText(isLunarMonthName);

    //超过 3 个字按每行 3 个字折行，最多两行，超出部分省略
    QStringList infoLines;
    if (!infoText.isEmpty()) {
        infoLines << infoText.left(3);
        if (infoText.size() > 3)
            infoLines << (infoText.size() > 6 ? infoText.mid(3, 2) + "…" : infoText.mid(3));
    }

    //字号自适应：单元格放不下就逐级缩小
    const int minInfoFontPx = 4;
    int infoFontPx = DDECalendar::FontSizeTwelve;
    QFont infoFont;
    infoFont.setPixelSize(infoFontPx);
    QFontMetrics infoMetrics(infoFont);
    const qreal infoAvailWidth = w - sideMargin * 2;
    if (!infoLines.isEmpty()) {
        auto linesFit = [&infoLines, infoAvailWidth, h](const QFontMetrics &fm) {
            qreal maxLineWidth = 0;
            for (const QString &line : infoLines)
                maxLineWidth = qMax(maxLineWidth, qreal(fm.horizontalAdvance(line)));
            return maxLineWidth <= infoAvailWidth && fm.height() * infoLines.size() <= h;
        };
        while (infoFontPx > minInfoFontPx && !linesFit(infoMetrics)) {
            --infoFontPx;
            infoFont.setPixelSize(infoFontPx);
            infoMetrics = QFontMetrics(infoFont);
        }
    }

    //整体（公历 + 间隙 + 农历）在单元格内垂直居中
    const qreal infoLineHeight = infoLines.isEmpty() ? 0 : infoMetrics.height();
    const qreal contentHeight = dayBlockHeight
            + (infoLines.isEmpty() ? 0 : blockGap + infoLineHeight * infoLines.size());
    const qreal contentTop = (h - contentHeight) / 2.0;
    const QRectF dayBlockRect((w - dayBlockWidth) / 2.0, contentTop, dayBlockWidth, dayBlockHeight);

    //日期数字左对齐在自己的块里，剩余宽度留给角标
    QRectF dateRect = dayBlockRect;
    if (showBadge)
        dateRect.setRight(dateRect.right() - badgeWidth);
    painter.drawText(dateRect, Qt::AlignLeft | Qt::AlignVCenter, dayStr);

    //绘制休/班角标
    if (showBadge) {
        QFont badgeFont;
        badgeFont.setPixelSize(DDECalendar::FontSizeTwelve);
        painter.setFont(badgeFont);
        const bool isRest = (m_dayInfo.holidayStatus == 1);
        painter.setPen(m_isToday ? infoColor : (isRest ? HolidayRestColor : HolidayWorkColor));

        QRectF badgeRect = dayBlockRect;
        badgeRect.setLeft(badgeRect.right() - badgeWidth);
        painter.drawText(badgeRect, Qt::AlignCenter, isRest ? tr("休") : tr("班"));
    }

    //绘制农历/节气/节日/农历月名：公历下方水平居中
    if (!infoLines.isEmpty()) {
        painter.setFont(infoFont);
        painter.setPen(isLunarMonthName ? LunarMonthBlue : infoColor);
        for (int i = 0; i < infoLines.size(); ++i) {
            const QRectF lineRect(sideMargin,
                                  dayBlockRect.bottom() + blockGap + i * infoLineHeight,
                                  infoAvailWidth,
                                  infoLineHeight);
            painter.drawText(lineRect, Qt::AlignHCenter | Qt::AlignVCenter, infoLines.at(i));
        }
    }
}

QString SidebarCalendarKeyButton::resolveInfoText(bool &isLunarMonthName) const
{
    isLunarMonthName = false;
    if (!m_hasDayInfo)
        return QString();

    const CaHuangLiDayInfo &lunar = m_dayInfo.lunar;
    //节日优先：农历节日 > 公历节日（除夕显示"除夕"而非"春节"）
    if (!lunar.mLunarFestival.isEmpty())
        return lunar.mLunarFestival;
    if (!lunar.mSolarFestival.isEmpty())
        return lunar.mSolarFestival;
    //初一：只显示农历月名（如"八月"），用蓝色标记
    if (lunar.mLunarDayName == QStringLiteral("初一")) {
        isLunarMonthName = true;
        return lunar.mLunarMonthName;
    }
    //节气
    if (!lunar.mTerm.isEmpty())
        return lunar.mTerm;
    //农历日
    return lunar.mLunarDayName;
}
