// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef TIMEFORMAT_H
#define TIMEFORMAT_H

#include <QLocale>
#include <QString>
#include <QTime>

QString addSecondsToShortTimeFormat(const QString &shortTimeFormat, const QString &longTimeFormat);

// 竖向任务栏显示秒时推荐的时间字号。thickness 是逻辑像素，
// 返回 0 表示任务栏厚度不足，不建议显示秒。
int secondsTimeFontSizeForDockThickness(int thickness);

// Format a time without its AM/PM text. The format must keep the AM/PM field
// while formatting so Qt keeps the hour in 12-hour form; the localized AM/PM
// text is removed from the formatted result afterwards.
QString formatTimeWithoutAmPm(const QLocale &locale, const QTime &time, const QString &timeFormat);

#endif // TIMEFORMAT_H
