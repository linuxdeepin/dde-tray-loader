// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "regionFormat.h"
#include "constants.h"

#include <QLocale>
#include <QGuiApplication>

RegionFormat::RegionFormat(QObject *parent)
    : QObject(parent)
    , m_config(DTK_CORE_NAMESPACE::DConfig::createGeneric("org.deepin.region-format", QString(), this))
{
    initData();
    initConnect();
}

void RegionFormat::initConnect()
{
    connect(m_config, &DTK_CORE_NAMESPACE::DConfig::valueChanged, this, [this] (const QString &key) {
        if (key == shortDateFormat_key) {
            setShortDateFormat(m_config->value(key).toString());
        } else if (key == longDateFormat_key) {
            setLongDateFormat(m_config->value(key).toString());
        } else if (key == shortTimeFormat_key) {
            setShortTimeFormat(m_config->value(key).toString());
        } else if (key == longTimeFormat_key) {
            setLongTimeFormat(m_config->value(key).toString());
        }
    });
}

void RegionFormat::initData()
{
    if (!m_config->isValid())
        return;

    if (m_config->isDefaultValue(localeName_key)) {
        setLocaleName(QLocale::system().name());
    } else {
        setLocaleName(m_config->value(localeName_key).toString());
    }
    if (m_config->isDefaultValue(shortDateFormat_key)) {
        QLocale locale(QLocale::system().name());
        setShortDateFormat(locale.dateFormat(QLocale::ShortFormat));
    } else {
        setShortDateFormat(m_config->value(shortDateFormat_key).toString());
    }
    if (m_config->isDefaultValue(longDateFormat_key)) {
        QLocale locale(QLocale::system().name());
        setLongDateFormat(locale.dateFormat(QLocale::LongFormat));
    } else {
        setLongDateFormat(m_config->value(longDateFormat_key).toString());
    }
    if (m_config->isDefaultValue(shortTimeFormat_key)) {
        QLocale locale(QLocale::system().name());
        setShortTimeFormat(locale.timeFormat(QLocale::ShortFormat));
    } else {
        setShortTimeFormat(m_config->value(shortTimeFormat_key).toString());
    }
    if (m_config->isDefaultValue(longTimeFormat_key)) {
        QLocale locale(QLocale::system().name());
        setLongTimeFormat(locale.timeFormat(QLocale::LongFormat));
    } else {
        setLongTimeFormat(m_config->value(longTimeFormat_key).toString());
    }
}

QString RegionFormat::getShortDateFormat() const
{
    return shortDateFormat;
}

void RegionFormat::setShortDateFormat(const QString &newShortDateFormat)
{
    m_originShortDateFormat = newShortDateFormat;
    QString format = newShortDateFormat;
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    // 任务栏在左/右的时候不显示年份
    const bool removeY = (position == Dock::Top || position == Dock::Bottom) ? false : true;
    if (removeY) {
        format.remove('y');
        format.remove('Y');
    }

    if (!format.at(0).isLetter()) {
        format.remove(0, 1);
    }

    if (!format.at(format.length() -1).isLetter()) {
        format.chop(1);
    }

    if (shortDateFormat == format)
        return;
    shortDateFormat = format;
    emit shortDateFormatChanged();
}

QString RegionFormat::getLongDateFormat() const
{
    return longDateFormat;
}

void RegionFormat::setLongDateFormat(const QString &newLongDateFormat)
{
    if (longDateFormat == newLongDateFormat)
        return;
    longDateFormat = newLongDateFormat;
    emit longDateFormatChanged();
}

QString RegionFormat::getShortTimeFormat() const
{
    return shortTimeFormat;
}

void RegionFormat::setShortTimeFormat(const QString &newShortTimeFormat)
{
    if (shortTimeFormat == newShortTimeFormat)
        return;

    if (newShortTimeFormat.contains("ap", Qt::CaseInsensitive)) {
        m_12HourFormat = newShortTimeFormat;
    }

    shortTimeFormat = newShortTimeFormat;
    emit shortTimeFormatChanged();
}

QString RegionFormat::getLongTimeFormat() const
{
    return longTimeFormat;
}

void RegionFormat::setLongTimeFormat(const QString &newLongTimeFormat)
{
    if (longTimeFormat == newLongTimeFormat)
        return;
    longTimeFormat = newLongTimeFormat;
    emit longTimeFormatChanged();
}

QString RegionFormat::getLocaleName() const
{
    return localeName;
}

void RegionFormat::setLocaleName(const QString &newLocaleName)
{
    if (localeName == newLocaleName)
        return;
    localeName = newLocaleName;
    emit localeNameChanged(localeName);
}

void RegionFormat::onDockPositionChanged(int position)
{
    Q_UNUSED(position)
    if (m_config->isDefaultValue(shortDateFormat_key)) {
        QLocale locale(QLocale::system().name());
        setShortDateFormat(locale.dateFormat(QLocale::ShortFormat));
    } else {
        setShortDateFormat(m_config->value(shortDateFormat_key).toString());
    }
}

void RegionFormat::sync24HourFormatConfig(bool is24HourFormat)
{
    QString hour12Format = m_12HourFormat.isEmpty() ? "AP h:mm" : m_12HourFormat;
    // 24小时制/12小时制切换，对应控制中心时间设置格式。
    QString value = is24HourFormat ? "HH:mm" : hour12Format;
    m_config->setValue(shortTimeFormat_key, value);
}

bool RegionFormat::is24HourFormat() const
{
    return !getShortTimeFormat().contains("AP", Qt::CaseInsensitive);
}
