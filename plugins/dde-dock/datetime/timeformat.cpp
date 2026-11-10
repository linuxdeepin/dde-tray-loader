// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "timeformat.h"

#include <QMap>
#include <QRegularExpression>

namespace {
const QRegularExpression &hourMinuteRegex()
{
    // Quoted text is consumed as a unit so an m in a literal such as 'min'
    // cannot be mistaken for the minute field.
    static const QRegularExpression re("([hH]{1,2})((?:'[^']*'|[^hHms])*)(m{1,2})");
    return re;
}

const QRegularExpression &minuteSecondsRegex()
{
    static const QRegularExpression re("(m{1,2})((?:'[^']*'|[^ms])*)(s{1,2})((?:\\s*'(?:[^']|'')*')*)");
    return re;
}

const QRegularExpression &literalSeparatorRegex()
{
    static const QRegularExpression re("'[^']*'|[\\p{L}\\p{M}]");
    return re;
}

bool containsSecondsField(QString timeFormat)
{
    static const QRegularExpression quotedTextRegex("'[^']*'");
    static const QRegularExpression secondsRegex("s{1,2}");
    timeFormat.remove(quotedTextRegex);
    return timeFormat.contains(secondsRegex);
}
}

QString addSecondsToShortTimeFormat(const QString &shortTimeFormat, const QString &longTimeFormat)
{
    if (containsSecondsField(shortTimeFormat))
        return shortTimeFormat;

    const QRegularExpressionMatch match = hourMinuteRegex().match(shortTimeFormat);
    if (!match.hasMatch())
        return shortTimeFormat;

    const QString minuteField = match.captured(3);
    QString separator = match.captured(2);
    QString secondsSuffix;
    if (separator.isEmpty() || separator.contains(literalSeparatorRegex())) {
        const QRegularExpressionMatch longMatch = minuteSecondsRegex().match(longTimeFormat);
        if (!longMatch.hasMatch())
            return shortTimeFormat;
        separator = longMatch.captured(2);
        secondsSuffix = longMatch.captured(4);
    }

    QString result = shortTimeFormat;
    const QString secondsField(minuteField.size(), QLatin1Char('s'));
    result.insert(match.capturedEnd(3), separator + secondsField + secondsSuffix);
    return result;
}

QString formatTimeWithoutAmPm(const QLocale &locale, const QTime &time, const QString &timeFormat)
{
    QString result = locale.toString(time, timeFormat);
    if (!timeFormat.contains(QLatin1String("AP"), Qt::CaseInsensitive))
        return result;

    const QString amPm = locale.toString(time, QStringLiteral("AP"));
    if (!amPm.isEmpty())
        result.remove(amPm, Qt::CaseInsensitive);
    // Some locales separate the time and the AM/PM field with a narrow
    // no-break space (for example en_US: "h:mm\u202fAp"). simplified()
    // removes the leftover whitespace after the AM/PM text is removed.
    return result.simplified();
}

int secondsTimeFontSizeForDockThickness(int thickness)
{
    // 显示秒后 "HH:mm:ss" 比 "HH:mm" 多 3 个字符，同等字号下宽度约为 1.6 倍。
    // 按 Noto Sans / 思源黑体实测：11px 约 44px 宽、12px 约 48px、13px 约 52px、
    // 14px 约 56px、15px 约 60px……再算上任务栏给时间插件的左右各 1px 边距和
    // 2px 余量，得到下面的厚度区间。厚度小于 48px 时 11px 字号已经放不下，
    // 此时不建议显示秒。
    static const QMap<int, int> map {
        {48, 11},
        {52, 12},
        {56, 13},
        {60, 14},
        {64, 15},
        {68, 16},
        {72, 17},
        {76, 18},
        {80, 19},
        {84, 20},
    };

    if (thickness < map.firstKey())
        return 0;

    int fontSize = map.first();
    for (auto it = map.cbegin(); it != map.cend() && it.key() <= thickness; ++it)
        fontSize = it.value();

    return fontSize;
}
