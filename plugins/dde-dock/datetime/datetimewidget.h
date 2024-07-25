// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DATETIMEWIDGET_H
#define DATETIMEWIDGET_H

#include <com_deepin_daemon_timedate.h>

#include <QWidget>

#include "regionFormat.h"

using Timedate = com::deepin::daemon::Timedate;

class DatetimeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DatetimeWidget(RegionFormat *regionFormat, QWidget *parent = nullptr) ;

    QSize sizeHint() const;
    inline bool is24HourFormat() const { return m_24HourFormat; }
    inline QString getDateTime() { return m_dateTime; }
    void setDockPanelSize(const QSize &dockSize);
    void dockPositionChanged();

    void setRegionFormat(RegionFormat *newRegionFormat);

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *e);

signals:
    void requestUpdateGeometry() const;

public slots:
    void set24HourFormat(const bool value);
    void updateDateTimeString();
    void updateDateTime();

private Q_SLOTS:
    void setShortDateFormat(int type);

    void setWeekdayFormat(int type);

private:
    QSize curTimeSize() const;
    void updateWeekdayFormat();

private:
    bool m_24HourFormat;
    int m_weekdayFormatType;
    mutable QFont m_timeFont;
    mutable QFont m_dateFont;
    Timedate *m_timedateInter;
    QString m_shortDateFormat;
    QString m_dateTime;
    QString m_weekFormat;
    QSize m_dockSize;

    RegionFormat *m_regionFormat;
};

#endif // DATETIMEWIDGET_H
