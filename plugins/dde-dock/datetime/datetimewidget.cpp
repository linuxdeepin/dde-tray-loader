// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "datetimewidget.h"
#include "constants.h"

#include <QApplication>
#include <QPainter>
#include <QDebug>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>

#define PLUGIN_STATE_KEY    "enable"
#define TIME_FONT DFontSizeManager::instance()->t4()
#define DATE_FONT DFontSizeManager::instance()->t10()

DWIDGET_USE_NAMESPACE

DatetimeWidget::DatetimeWidget(RegionFormat* regionFormat, QWidget *parent)
    : QWidget(parent)
    , m_24HourFormat(false)
    , m_weekdayFormatType(0)
    , m_timedateInter(new Timedate("com.deepin.daemon.Timedate", "/com/deepin/daemon/Timedate", QDBusConnection::sessionBus(), this))
    , m_shortDateFormat("yyyy-MM-dd")
    , m_dockSize(QSize(1920, 40))
    , m_regionFormat(regionFormat)
{
    setMinimumSize(PLUGIN_BACKGROUND_MIN_SIZE, PLUGIN_BACKGROUND_MIN_SIZE);
    setWeekdayFormat(m_timedateInter->weekdayFormat());
    set24HourFormat(m_regionFormat->getShortTimeFormat().contains("AP"));
    updateDateTimeString();

    connect(m_timedateInter, &Timedate::WeekdayFormatChanged, this, &DatetimeWidget::setWeekdayFormat);
    //连接日期时间修改信号,更新日期时间插件的布局
    connect(m_timedateInter, &Timedate::TimeUpdate, this, [ = ]{
        if (isVisible()) {
            emit requestUpdateGeometry();
        }
    });

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
    update();

    if (isVisible()) {
        emit requestUpdateGeometry();
    }
}

/**
 * @brief DatetimeWidget::setShortDateFormat 根据类型设置时间显示格式
 * @param type 自定义类型
 */
void DatetimeWidget::setShortDateFormat(int type)
{
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();

    // 任务栏在左/右的时候不显示年份
    const bool removeY = (position == Dock::Top || position == Dock::Bottom) ? false : true;
    switch (type) {
    case 0: removeY ? m_shortDateFormat = "M/d" : m_shortDateFormat = "yyyy/M/d"; break;
    case 1: removeY ? m_shortDateFormat = "M-d" : m_shortDateFormat = "yyyy-M-d"; break;
    case 2: removeY ? m_shortDateFormat = "M.d" : m_shortDateFormat = "yyyy.M.d"; break;
    case 3: removeY ? m_shortDateFormat = "MM/dd" : m_shortDateFormat = "yyyy/MM/dd"; break;
    case 4: removeY ? m_shortDateFormat = "MM-dd" : m_shortDateFormat = "yyyy-MM-dd"; break;
    case 5: removeY ? m_shortDateFormat = "MM.dd" : m_shortDateFormat = "yyyy.MM.dd"; break;
    case 6: removeY ? m_shortDateFormat = "MM.dd" : m_shortDateFormat = "MM.dd.yyyy"; break;
    case 7: removeY ? m_shortDateFormat = "dd.MM" : m_shortDateFormat = "dd.MM.yyyy"; break;
    case 8: removeY ? m_shortDateFormat = "M/d" : m_shortDateFormat = "yy/M/d"; break;
    case 9: removeY ? m_shortDateFormat = "M-d" : m_shortDateFormat = "yy-M-d"; break;
    case 10: removeY ? m_shortDateFormat = "M.d" : m_shortDateFormat = "yy.M.d"; break;
    default: removeY ? m_shortDateFormat = "MM-dd" : m_shortDateFormat = "yyyy-MM-dd"; break;
    }

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
    const QDateTime currentDateTime = QDateTime::currentDateTime();
    auto dayOfWeek = currentDateTime.date().dayOfWeek();

    if (0 == m_weekdayFormatType) {
        switch (dayOfWeek) {
        case 1:
            m_weekFormat = tr("Monday"); //星期一
            break;
        case 2:
            m_weekFormat = tr("Tuesday"); //星期二
            break;
        case 3:
            m_weekFormat = tr("Wednesday"); //星期三
            break;
        case 4:
            m_weekFormat = tr("Thursday"); //星期四
            break;
        case 5:
            m_weekFormat = tr("Friday"); //星期五
            break;
        case 6:
            m_weekFormat = tr("Saturday"); //星期六
            break;
        case 7:
            m_weekFormat = tr("Sunday"); //星期天
            break;
        default:
            m_weekFormat = tr("Monday"); //星期一
            break;
        }
    } else {
        switch (dayOfWeek) {
        case 1:
            m_weekFormat = tr("monday"); //周一
            break;
        case 2:
            m_weekFormat = tr("tuesday"); //周二
            break;
        case 3:
            m_weekFormat = tr("wednesday"); //周三
            break;
        case 4:
            m_weekFormat = tr("thursday"); //周四
            break;
        case 5:
            m_weekFormat = tr("friday"); //周五
            break;
        case 6:
            m_weekFormat = tr("saturday"); //周六
            break;
        case 7:
            m_weekFormat = tr("sunday"); //周天
            break;
        default:
            m_weekFormat = tr("monday"); //周一
            break;
        }
    }
}

void DatetimeWidget::setRegionFormat(RegionFormat *newRegionFormat)
{
    m_regionFormat = newRegionFormat;
}

/**
 * @brief DatetimeWidget::updateWeekdayTimeString 更新任务栏时间标签的显示
 */
void DatetimeWidget::updateDateTimeString()
{
    QLocale locale(m_regionFormat->getLocaleName());
    m_dateTime = locale.toString(QDate::currentDate(), m_regionFormat->getLongDateFormat()) + QDateTime::currentDateTime().toString(m_regionFormat->getLongTimeFormat());
}

void DatetimeWidget::updateDateTime()
{
    set24HourFormat(m_regionFormat->getShortTimeFormat().contains("AP"));
    updateDateTimeString();
    update();

    if (isVisible()) {
        emit requestUpdateGeometry();
    }
}

/**
 * @brief DatetimeWidget::curTimeSize 调整时间日期字体大小
 * @return 返回时间和日期绘制的区域大小
 */
QSize DatetimeWidget::curTimeSize() const
{
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();

    m_timeFont = TIME_FONT;
    m_dateFont = DATE_FONT;
    QString timeFormat = m_regionFormat->getShortTimeFormat();
    QString dateFormat = m_regionFormat->getShortDateFormat();
    if ((position == Dock::Right || position == Dock::Left)) {
        int index = timeFormat.indexOf("ap");
        if (index != -1) {
            timeFormat.insert(index + 2, '\n');
        }
    }

    QString timeString = QDateTime::currentDateTime().toString(timeFormat);
    QString dateString = QDateTime::currentDateTime().toString(dateFormat);

    QSize timeSize = QFontMetrics(m_timeFont).boundingRect(timeString).size();
    int maxWidth = std::max(QFontMetrics(m_timeFont).boundingRect(timeString).size().width(), QFontMetrics(m_timeFont).horizontalAdvance(timeString));
    timeSize.setWidth(maxWidth);

    if (timeString.contains("\n")) {
        QStringList SL = timeString.split("\n");
        maxWidth = std::max(QFontMetrics(m_timeFont).boundingRect(SL.at(0)).size().width(), QFontMetrics(m_timeFont).horizontalAdvance(SL.at(0)));
        timeSize = QSize(maxWidth, QFontMetrics(m_timeFont).boundingRect(SL.at(0)).height() + QFontMetrics(m_timeFont).boundingRect(SL.at(1)).height());
    }

    QSize dateSize = QFontMetrics(m_dateFont).boundingRect(dateString).size();
    maxWidth = std::max(QFontMetrics(m_dateFont).boundingRect(dateString).size().width(), QFontMetrics(m_dateFont).horizontalAdvance(dateString));
    dateSize.setWidth(maxWidth);

    // 通过默认字体大小算出的控件Size，实际还需要结合任务栏size进行处理
    QSize widgetSize = QSize(std::max(timeSize.width(), dateSize.width()), timeSize.height() + dateSize.height());

    if (position == Dock::Bottom || position == Dock::Top) {
        // 有效高度，控件与任务栏要至少保持2px的间距（上/下），hover样式美观
        int vaildHeight = widgetSize.height() > (m_dockSize.height() - 4) ? m_dockSize.height() - 4 : widgetSize.height();
        widgetSize.setHeight(vaildHeight);
    
        // 更新有效高度下的字体大小
        while (QFontMetrics(m_timeFont).boundingRect(timeString).height() + QFontMetrics(m_dateFont).boundingRect(dateString).height() > vaildHeight && m_timeFont.pixelSize() > 1) {
            m_timeFont.setPixelSize(m_timeFont.pixelSize() - 1);
            maxWidth = std::max(QFontMetrics(m_timeFont).boundingRect(timeString).size().width(), QFontMetrics(m_timeFont).horizontalAdvance(timeString));
            timeSize.setWidth(maxWidth);
            timeSize.setHeight(QFontMetrics(m_timeFont).boundingRect(timeString).height());
            if (m_timeFont.pixelSize() - m_dateFont.pixelSize() == 1) {
                m_dateFont.setPixelSize(m_dateFont.pixelSize() - 1);
                maxWidth = std::max(QFontMetrics(m_dateFont).boundingRect(dateString).size().width(), QFontMetrics(m_dateFont).horizontalAdvance(dateString));
                dateSize.setWidth(maxWidth);
                dateSize.setHeight(QFontMetrics(m_dateFont).boundingRect(dateString).height());
            }
        }

        // 新字体大小更新控件的size，并将宽度增加12px，使文本不贴边显示，模拟出widget内左/右6px的间距
        widgetSize.setWidth(std::max(timeSize.width(), dateSize.width()) + 12);
    } else {
        // 有效宽度，控件与任务栏要至少保持2px的间距（左/右），hover样式美观
        int vaildWidth = widgetSize.width() > (m_dockSize.width() - 4) ? m_dockSize.width() - 4 : widgetSize.width();
        widgetSize.setWidth(vaildWidth);

        // 更新有效宽度下的字体大小
        // 任务栏宽度固定，控件宽度固定，如果要使文本不贴边显示，只能再将文本矩形宽度缩小6px，模拟出内左/右间距3px的效果
        int textWidth = vaildWidth;
        while (std::max(timeSize.width(), dateSize.width()) > vaildWidth - 6 && m_timeFont.pixelSize() > 1) {
            m_timeFont.setPixelSize(m_timeFont.pixelSize() - 1);
            if (m_24HourFormat) {
                timeSize.setHeight(QFontMetrics(m_timeFont).boundingRect(timeString).size().height());
            } else {
                timeSize.setHeight(QFontMetrics(m_timeFont).boundingRect(timeString).size().height() * 2);
            }
            if (timeString.contains("\n")) {
                QStringList SL = timeString.split("\n");
                textWidth = std::max(QFontMetrics(m_timeFont).boundingRect(SL.at(0)).size().width(),
                                    QFontMetrics(m_timeFont).horizontalAdvance(SL.at(0)));
            } else {
                textWidth = QFontMetrics(m_timeFont).boundingRect(timeString).width();
            }
            timeSize.setWidth(textWidth);
            if (m_timeFont.pixelSize() - m_dateFont.pixelSize() == 1) {
                m_dateFont.setPixelSize(m_dateFont.pixelSize() - 1);
                dateSize.setWidth(QFontMetrics(m_dateFont).boundingRect(dateString).width());
                dateSize.setHeight(QFontMetrics(m_dateFont).boundingRect(dateString).height());
            }
        }

        // 新字体大小更新控件的size
        widgetSize.setHeight(timeSize.height() + dateSize.height());
    }

    return widgetSize;
}

QSize DatetimeWidget::sizeHint() const
{
    return curTimeSize();
}

void DatetimeWidget::resizeEvent(QResizeEvent *event)
{
    if (isVisible())
        emit requestUpdateGeometry();

    QWidget::resizeEvent(event);
}

/**
 * @brief DatetimeWidget::paintEvent 绘制任务栏时间日期
 * @param e
 */
void DatetimeWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    const QDateTime current = QDateTime::currentDateTime();

    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(palette().brightText(), 1));

    // 时间文本所在行的矩形、绘制时间文本的矩形、时间文本紧凑矩形
    QRect timeRowRect = rect();
    QRect timeTextRect = rect();
    QRect timeTextTightRect = rect();

    QRect dateRowRect = rect();
    QRect dateTextRect = rect();
    QRect dateTextTightRect = rect();

    QString format = m_regionFormat->getShortTimeFormat();
    if ((position == Dock::Right || position == Dock::Left)) {
        int index = format.indexOf("ap");
        if (index != -1) {
            format.insert(index + 2, '\n');
        }
    }
    QString timeStr = current.toString(format);

    format = m_regionFormat->getShortDateFormat();
    QString dateStr = current.toString(format);

    // 处理绘制文本的矩形size
    QSize timeTextSize = QFontMetrics(m_timeFont).boundingRect(timeStr).size();
    QSize dateTextSize = QFontMetrics(m_dateFont).boundingRect(dateStr).size();
    timeTextSize.setWidth(std::max(timeTextSize.width(), QFontMetrics(m_timeFont).horizontalAdvance(timeStr)));
    dateTextSize.setWidth(std::max(dateTextSize.width(), QFontMetrics(m_dateFont).horizontalAdvance(dateStr)));
    if (position == Dock::Left || position == Dock::Right) {
        if (!m_24HourFormat) {
            timeTextSize.setHeight(timeTextSize.height() * 2);
        }
        if (timeStr.contains("\n")) {
            QStringList SL = timeStr.split("\n");
            timeTextSize.setWidth(std::max(QFontMetrics(m_timeFont).boundingRect(SL.at(0)).size().width(),
                                 QFontMetrics(m_timeFont).horizontalAdvance(SL.at(0))));
        }
    }

    // 一些字体得出的size异常，特别是藏语，这里补偿处理
    int timeMaxHeight = timeTextSize.height();
    int dateMaxHeight = dateTextSize.height();
    if (m_timeFont.family() == "STXinwei") { // 华文新魏
        dateMaxHeight -= 1;
    } else if (m_timeFont.family() == "Noto Sans Tibetan" || m_timeFont.family() == "Tibetan Machine Uni") {
        timeMaxHeight += 15;
        dateMaxHeight += 5;
    } else if (m_timeFont.family() == "Noto Serif Tibetan") {
        if (m_24HourFormat) {
            timeMaxHeight += 25;
            dateMaxHeight += 10;
        } else {
            timeMaxHeight += 15;
            dateMaxHeight += 6;
        }
    }
    timeTextRect = QRect(0, 0, timeTextSize.width(), timeMaxHeight);
    dateTextRect = QRect(0, timeTextRect.y() + timeTextSize.height(), dateTextSize.width(), dateMaxHeight);

    // 处理对应行的矩形
    timeRowRect = timeTextRect;
    timeRowRect.setWidth(width());
    dateRowRect = dateTextRect;
    dateRowRect.setWidth(width());

    // 将绘制文本矩形移动至对应行矩形的中间
    timeTextRect.moveCenter(timeRowRect.center());
    dateTextRect.moveCenter(dateRowRect.center());

    // 文本紧凑矩形，并将紧凑矩形移动到绘制文本矩形中间
    // 不能直接用文本紧凑矩形来绘制，否则会有截断，此处只用来协助判断上/下间距
    QSize timeTextTightSize = QFontMetrics(m_timeFont).tightBoundingRect(timeStr).size();
    QSize dateTextTightSize = QFontMetrics(m_dateFont).tightBoundingRect(dateStr).size();
    if (position == Dock::Left || position == Dock::Right) {
        if (!m_24HourFormat) {
            timeTextTightSize.setHeight(timeTextTightSize.height() * 2);
        }
    }
    timeTextTightSize.setWidth(timeTextRect.size().width());
    dateTextTightSize.setWidth(dateTextRect.size().width());
    timeTextTightRect = QRect(timeTextRect.x(), QFontMetrics(m_timeFont).tightBoundingRect(timeStr).y(), timeTextTightSize.width(), timeTextTightSize.height());
    dateTextTightRect = QRect(dateTextRect.x(), QFontMetrics(m_dateFont).tightBoundingRect(dateStr).y(), dateTextTightSize.width(), dateTextTightSize.height());
    timeTextTightRect.moveCenter(timeTextRect.center());
    dateTextTightRect.moveCenter(dateTextRect.center());

    // 判断文本紧凑矩形和行矩形的上/下间距，尽量确保实际绘制出来的文本与widget之间的上/下间距相等
    int dValue = timeTextTightRect.top() - timeRowRect.top();
    // 目前仅处理有上间距，且大于5的情况，使时间文本绘制到离widget上间距5px的位置
    int cnt = 10; // 最多改变10次，确保退出循环
    while (dValue >= 5 && cnt > 0) {
        timeTextRect.setTop(timeTextRect.top() - 1.0); // 向上移动1px
        timeTextTightRect.moveCenter(timeTextRect.center());
        dValue = timeTextTightRect.top() - timeRowRect.top();
        --cnt;
    }
    // 目前仅处理有下间距，且在0~topMargin px的情况，使日期文本绘制到离widget下间距topMargin px的位置
    const int topMargin = dValue;
    dValue = dateRowRect.bottom() - dateTextTightRect.bottom();
    cnt = 10;
    while ((dValue >= 0 && dValue <= topMargin) && cnt > 0) {
        dateTextRect.setTop(dateTextRect.top() - 1); // 向上移动1px
        dateTextTightRect.moveCenter(dateTextRect.center());
        dValue = dateRowRect.bottom() - dateTextTightRect.bottom();
        --cnt;
    }

    painter.setFont(m_timeFont);
    painter.drawText(timeTextRect, Qt::AlignCenter, timeStr);

    painter.setFont(m_dateFont);
    painter.drawText(dateTextRect, Qt::AlignCenter, dateStr);
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
        update();
    }
}

void DatetimeWidget::dockPositionChanged()
{
    // 等待位置变换完成后再更新
    QTimer::singleShot(300, this, [this]{
        updateDateTime();
    });
}

void DatetimeWidget::showEvent(QShowEvent *e)
{
    if (topLevelWidget()) {
        m_dockSize = topLevelWidget()->size();
    }
    QWidget::showEvent(e);
}
