// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "lunarmanager.h"
#include "commondef.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>

namespace {

// 解析 getHuangLiMonth 返回的 JSON：
// {"Days":N, "FirstDayWeek":X, "Datas":[{Suit/Avoid/Worktime/LunarFestival/SolarFestival/Term/
//   Zodiac/LunarLeapMonth/LunarDayName/LunarMonthName/GanZhiDay/GanZhiMonth/GanZhiYear}, ...]}
// Datas 的下标 i 对应当月第 i+1 天（fill=false 时仅包含当月日期）。
void parseHuangLiMonth(const QDate &month, const QString &json, QMap<QDate, CaHuangLiDayInfo> &out)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "parse getHuangLiMonth json error:" << err.errorString();
        return;
    }
    const QJsonArray datas = doc.object().value("Datas").toArray();
    for (int i = 0; i < datas.size(); ++i) {
        const QDate date(month.year(), month.month(), i + 1);
        if (!date.isValid())
            break;
        CaHuangLiDayInfo info;
        info.jsonObjectToInfo(datas.at(i).toObject());
        out.insert(date, info);
    }
}

// 解析 getFestivalMonth 返回的 JSON：
// [{"id":"...","description":"","FestivalName":"中秋节","list":[{"date":"2026-09-25","status":1}, ...]}]
// 注意：节日名（FestivalName）在外层对象上，整个假期区间共用一个名字；
//       list 里每一天只有 date + status（1-休，2-班），**没有 name 字段**。
// 因此这里只提取 date -> status 的映射（用于休/班角标）；节日名不从此处取，
// 节日名由 getHuangLiMonth 的 LunarFestival/SolarFestival 字段给出（仅节日当天有值），
// 避免整个假期每天都顶着同一个节日名。
void parseFestivalMonth(const QString &json, QMap<QDate, int> &out)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "parse getFestivalMonth json error:" << err.errorString();
        return;
    }
    const QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        const QJsonArray list = val.toObject().value("list").toArray();
        for (const QJsonValue &item : list) {
            const QJsonObject obj = item.toObject();
            const QDate date = QDate::fromString(obj.value("date").toString(), "yyyy-MM-dd");
            if (!date.isValid())
                continue;
            out.insert(date, obj.value("status").toInt());
        }
    }
}

} // namespace

LunarManager::LunarManager(QObject* parent)
    : QObject(parent)
{
}

LunarManager* LunarManager::instace()
{
    static LunarManager lunarManager;
    return &lunarManager;
}

void LunarManager::asyncRequestLunar(const QDate& date)
{
    // 创建 D-Bus 方法调用消息
    QDBusMessage message = QDBusMessage::createMethodCall(
        "com.deepin.dataserver.Calendar",
        "/com/deepin/dataserver/Calendar/HuangLi",
        "com.deepin.dataserver.Calendar.HuangLi",
        "getHuangLiDay"
    );

    // 设置参数
    message << quint32(date.year()) << quint32(date.month()) << quint32(date.day());

    // 异步调用并创建一个 Watcher
    QDBusPendingCall asyncCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncCall, this);

    // 连接 Watcher 的信号以处理结果
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        QDBusPendingReply<QString> reply = *watcher;
        if (reply.isError()) {
            qWarning() << "Failed to call getHuangLiDay:" << reply.error().message();
        } else {
            QString replyStr = reply.value();
            bool isVaild;
            CaHuangLiDayInfo info;
            info.strJsonToInfo(replyStr, isVaild);
            if (isVaild) {
                emit lunarInfoReady(info);
            } else {
                qWarning()<< "parse huangli json error:" << replyStr;
            }
        }

        watcher->deleteLater();
    });
}

void LunarManager::requestMonth(const QDate &month)
{
    const QDate firstDay(month.year(), month.month(), 1);
    if (!firstDay.isValid())
        return;

    // 缓存命中：直接返回
    if (m_cache.contains(firstDay)) {
        Q_EMIT monthInfoReady(firstDay, m_cache.value(firstDay));
        return;
    }
    if (m_pending.contains(firstDay))
        return;

    m_pending.insert(firstDay, PendingMonthData());

    // 1) 农历/节气/节日：getHuangLiMonth(year, month, fill=false)
    QDBusMessage msg = QDBusMessage::createMethodCall(
        CalendarServiceName,
        CalendarPath + "/HuangLi",
        CalendarServiceName + ".HuangLi",
        "getHuangLiMonth");
    msg << quint32(firstDay.year()) << quint32(firstDay.month()) << false;
    QDBusPendingCall asyncCall = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, firstDay]() {
        watcher->deleteLater();
        auto it = m_pending.find(firstDay);
        if (it == m_pending.end())
            return;
        QDBusPendingReply<QString> reply = *watcher;
        if (reply.isError()) {
            qWarning() << "Failed to call getHuangLiMonth:" << reply.error().message();
        } else {
            parseHuangLiMonth(firstDay, reply.value(), it->days);
        }
        it->daysReady = true;
        mergeAndEmitMonth(firstDay);
    });

    // 2) 节假日/调休：getFestivalMonth(year, month)
    QDBusMessage festivalMsg = QDBusMessage::createMethodCall(
        CalendarServiceName,
        CalendarPath + "/HuangLi",
        CalendarServiceName + ".HuangLi",
        "getFestivalMonth");
    festivalMsg << quint32(firstDay.year()) << quint32(firstDay.month());
    QDBusPendingCall festivalCall = QDBusConnection::sessionBus().asyncCall(festivalMsg);
    QDBusPendingCallWatcher *festivalWatcher = new QDBusPendingCallWatcher(festivalCall, this);
    connect(festivalWatcher, &QDBusPendingCallWatcher::finished, this, [this, festivalWatcher, firstDay]() {
        festivalWatcher->deleteLater();
        auto it = m_pending.find(firstDay);
        if (it == m_pending.end())
            return;
        QDBusPendingReply<QString> reply = *festivalWatcher;
        if (reply.isError()) {
            qWarning() << "Failed to call getFestivalMonth:" << reply.error().message();
        } else {
            parseFestivalMonth(reply.value(), it->festivals);
        }
        it->festivalsReady = true;
        mergeAndEmitMonth(firstDay);
    });
}

void LunarManager::mergeAndEmitMonth(const QDate &month)
{
    auto it = m_pending.find(month);
    if (it == m_pending.end())
        return;
    if (!it->daysReady || !it->festivalsReady)
        return;

    const PendingMonthData data = it.value();
    m_pending.erase(it);

    QMap<QDate, CalendarDayInfo> infos;
    for (auto dayIt = data.days.constBegin(); dayIt != data.days.constEnd(); ++dayIt) {
        CalendarDayInfo info;
        info.lunar = dayIt.value();
        info.holidayStatus = data.festivals.value(dayIt.key(), 0);
        infos.insert(dayIt.key(), info);
    }
    m_cache.insert(month, infos);
    Q_EMIT monthInfoReady(month, infos);
}
