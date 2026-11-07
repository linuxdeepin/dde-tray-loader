// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef LUNARMANAGER_H
#define LUNARMANAGER_H

#include "huangliData/dbusdatastruct.h"

#include <QObject>
#include <QDate>
#include <QMap>
#include <QMetaType>

// 某一天的综合日历信息：农历、节气、节假日、调休
struct CalendarDayInfo
{
    CaHuangLiDayInfo lunar;       // 农历/节气/农历节日/公历节日
    int holidayStatus = 0;        // 调休状态：0-无，1-休，2-班
};
Q_DECLARE_METATYPE(CalendarDayInfo)

class LunarManager : public QObject
{
    Q_OBJECT
public:
    explicit LunarManager(QObject *parent = nullptr);
    static LunarManager* instace();

    void asyncRequestLunar(const QDate &date);

    // 请求指定公历月（当月 1 日~月末）的农历/节气/节假日/调休数据。
    // 数据就绪后发出 monthInfoReady；同一月份会被缓存，再次请求直接返回。
    void requestMonth(const QDate &month);

Q_SIGNALS:
    void lunarInfoReady(const CaHuangLiDayInfo &info);
    void monthInfoReady(const QDate &month, const QMap<QDate, CalendarDayInfo> &infos);

private:
    // 一个月的两个异步请求（农历黄历 + 节假日调休）的暂存结果
    struct PendingMonthData
    {
        QMap<QDate, CaHuangLiDayInfo> days;              // 农历/节气/节日
        QMap<QDate, int> festivals;                      // 调休状态（1-休，2-班）
        bool daysReady = false;
        bool festivalsReady = false;
    };

    void mergeAndEmitMonth(const QDate &month);

private:
    QMap<QDate, PendingMonthData> m_pending;                 // 请求中的月份
    QMap<QDate, QMap<QDate, CalendarDayInfo>> m_cache;       // 已完成月份的缓存

};
#define gLunarManager LunarManager::instace()
#endif // LUNARMANAGER_H
