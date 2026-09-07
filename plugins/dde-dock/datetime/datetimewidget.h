// SPDX-FileCopyrightText: 2011 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DATETIMEWIDGET_H
#define DATETIMEWIDGET_H

#include <QWidget>

#include "timedate1interface.h"

using Timedate1Inter = org::deepin::dde::Timedate1;

class QLabel;
class QBoxLayout;
class RegionFormat;
class DatetimeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DatetimeWidget(RegionFormat *regionFormat, QWidget *parent = nullptr) ;

    inline bool is24HourFormat() const { return m_24HourFormat; }
    inline QString getDateTime() { return m_dateTime; }
    void setDockPanelSize(const QSize &dockSize);
    // 任务栏是否处于时尚模式，一行显示只在时尚模式下生效
    void setFashionMode(bool fashionMode);
    void dockPositionChanged();

    void setRegionFormat(RegionFormat *newRegionFormat);

    QSize sizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void requestUpdateGeometry() const;

public slots:
    void set24HourFormat(const bool value);
    void updateDateTimeString();

private Q_SLOTS:
    void setWeekdayFormat(int type);

private:
    void initUI();
    void adjustUI();
    void adjustFontSize();
    void adjustLayout();
    // 一行显示时时间和日期字号不同，按基线对齐两个 label
    void alignSingleLineBaseline();
    void updateDateTime();
    void updateWeekdayFormat();
    // 任务栏高度扣掉任务栏给插件的上下边距后，插件真正可用的高度
    int availableHeight() const;
    // 可用高度放不下两行时，时间和日期并排显示成一行
    bool shouldUseSingleLine() const;

private:
    QLabel *m_timeLabel;
    QLabel *m_dateLabel;
    QLabel *m_apLabel;
    QWidget *m_spacerItem;
    QBoxLayout *m_layout;

private:
    bool m_24HourFormat;
    bool m_singleLineLayout;
    bool m_fashionMode;
    int m_weekdayFormatType;
    QString m_shortDateFormat;
    QString m_dateTime;
    QString m_weekFormat;
    QSize m_dockSize;

    Timedate1Inter *m_timedateInter;
    RegionFormat *m_regionFormat;
};

#endif // DATETIMEWIDGET_H
