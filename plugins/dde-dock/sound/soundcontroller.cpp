// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "soundcontroller.h"
#include "soundmodel.h"

SoundController::SoundController(QObject *parent)
    : QObject(parent)
    , m_audioInter(new DBusAudio("org.deepin.dde.Audio1", "/org/deepin/dde/Audio1", QDBusConnection::sessionBus(), this))
    , m_defaultSinkInter(nullptr)
    , m_dconfig(DConfig::create("org.deepin.dde.tray-loader", "org.deepin.dde.dock.plugin.sound", QString(), this))
{
    connect(m_audioInter, &DBusAudio::DefaultSinkChanged, this, &SoundController::onDefaultSinkChanged);
    connect(m_audioInter, &DBusAudio::CardsWithoutUnavailableChanged, &SoundModel::ref(), &SoundModel::setCardsInfo);
    connect(m_audioInter, &DBusAudio::MaxUIVolumeChanged, &SoundModel::ref(), &SoundModel::setMaxVolumeUI);
    connect(m_audioInter, &DBusAudio::IncreaseVolumeChanged, this, [this] {
        SoundModel::ref().setMaxVolumeUI(m_audioInter->maxUIVolume());
        if (m_defaultSinkInter) {
            SoundModel::ref().setVolume(m_defaultSinkInter->volume());
        }
    });

    SoundModel::ref().setCardsInfo(m_audioInter->cardsWithoutUnavailable());
    SoundModel::ref().setMaxVolumeUI(m_audioInter->maxUIVolume());
    onDefaultSinkChanged(m_audioInter->defaultSink());
}

SoundController::~SoundController()
{

}

bool SoundController::mute()
{
    if (!m_defaultSinkInter)
        return false;

    return m_defaultSinkInter->mute();
}

double SoundController::volume()
{
    if (!m_defaultSinkInter)
        return 0.0;

    return m_defaultSinkInter->volume();
}

void SoundController::SetVolume(double in0, bool in1)
{
     if (!m_defaultSinkInter)
        return;

    m_defaultSinkInter->SetVolume(in0, in1);
}

void SoundController::SetMuteQueued(bool in0)
{
    if (!m_defaultSinkInter)
        return;

    m_defaultSinkInter->SetMuteQueued(in0);
}

void SoundController::SetMute(bool in0)
{
    if (!m_defaultSinkInter)
        return;

    m_defaultSinkInter->SetMute(in0);
}

void SoundController::onDefaultSinkChanged(const QDBusObjectPath &path)
{
    // 防止手动切换设备，与后端交互时，获取到多个信号，设备切换多次，造成混乱
    QThread::msleep(200);

    if (m_defaultSinkInter)
        m_defaultSinkInter->deleteLater();

    m_defaultSinkInter = new DBusSink("org.deepin.dde.Audio1", path.path(), QDBusConnection::sessionBus(), this);

    SoundModel::ref().setActivePort(m_defaultSinkInter->card(), m_defaultSinkInter->activePort().name);
    SoundModel::ref().setMute(m_defaultSinkInter->mute());
    SoundModel::ref().setVolume(existActiveOutputDevice() ? m_defaultSinkInter->volume() : 0);

    // 音量和静音状态变化时手动获取下另外一个的状态，有时候收不到 changed 信号
    connect(m_defaultSinkInter, &DBusSink::MuteChanged, &SoundModel::ref(), [this] (bool value) {
        SoundModel::ref().setMute(m_defaultSinkInter->mute());
        SoundModel::ref().setVolume(m_defaultSinkInter->volume());
    });
    connect(m_defaultSinkInter, &DBusSink::VolumeChanged, &SoundModel::ref(), [this] (double value) {
        SoundModel::ref().setVolume(value);
        // 收到音量变化 dbus 后再取消静音，避免时序问题
        if (m_defaultSinkInter->mute()) {
            m_defaultSinkInter->SetMuteQueued(false);
        }
    });
    connect(m_defaultSinkInter, &DBusSink::ActivePortChanged, this, [this](AudioPort port) {
        SoundModel::ref().setActivePort(m_defaultSinkInter->card(), port.name);
    });
    Q_EMIT defaultSinkChanged(m_defaultSinkInter);
}

/** 判断是否存在未禁用的声音输出设备
 * @brief existActiveOutputDevice
 * @return 存在返回true,否则返回false
 */
bool SoundController::existActiveOutputDevice() const
{
    if (SoundModel::ref().existActiveOutputDevice()) {
        return true;
    }

    if (m_dconfig.data()->isValid()) {
       bool enableAdjustVolumeNoCard = m_dconfig.data()->value("enableAdjustVolumeNoCard", false).toBool();
       if (enableAdjustVolumeNoCard) {
           return m_defaultSinkInter;
       }
    }
    // 兼容云平台无端口的情况
    return SoundModel::ref().ports().isEmpty() && m_defaultSinkInter && !m_defaultSinkInter->name().startsWith("auto_null") && !m_defaultSinkInter->name().contains("bluez");
}
