// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "mediacontroller.h"
#include "mediamonitor.h"
#include "mediamodel.h"
#include <qdbusinterface.h>

#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>
#include <QDebug>
#include <QImageReader>
#include <QUrl>

namespace {

QString metadataText(const QVariant &value)
{
    if (value.canConvert<QStringList>()) {
        return value.toStringList().join(QStringLiteral(", "));
    }

    if (value.metaType().id() == QMetaType::QVariantList) {
        QStringList parts;
        const auto values = value.toList();
        for (const auto &item : values) {
            const QString text = item.toString();
            if (!text.isEmpty()) {
                parts.append(text);
            }
        }
        return parts.join(QStringLiteral(", "));
    }

    return value.toString();
}

QString playerIdentity(const QString &service)
{
    QString fallback = service;
    fallback.remove(QStringLiteral("org.mpris.MediaPlayer2."));
    return fallback;
}

}

MediaController::MediaController()
    : m_mediaPlayer2(nullptr)
    , m_mediaMonitor(new MediaMonitor(this))
{
    connect(m_mediaMonitor, &MediaMonitor::mediaAcquired, this, &MediaController::loadMediaPath);
    connect(m_mediaMonitor, &MediaMonitor::mediaLost, this, &MediaController::removeMediaPath);
    m_mediaMonitor->init();
}

void MediaController::loadMediaPath(const QString &path)
{
    QDBusInterface propertiesInter(path,
                                   QStringLiteral("/org/mpris/MediaPlayer2"),
                                   QStringLiteral("org.freedesktop.DBus.Properties"),
                                   QDBusConnection::sessionBus());
    const QDBusPendingCall asyncCall = propertiesInter.asyncCall(QStringLiteral("Get"),
                                                                 QStringLiteral("org.mpris.MediaPlayer2"),
                                                                 QStringLiteral("CanShowInUI"));
    auto *watcher = new QDBusPendingCallWatcher(asyncCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, path](QDBusPendingCallWatcher *watcher) {
        const QDBusPendingReply<QDBusVariant> reply = *watcher;
        watcher->deleteLater();

        const bool showInUI = reply.isError() ? true : reply.value().variant().toBool();
        if (showInUI) {
            finishLoadMediaPath(path);
        }
    });
}

void MediaController::finishLoadMediaPath(const QString &path)
{
    DBusMediaPlayer2 *newMediaPlayer = new DBusMediaPlayer2(path, "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus(), this);

    const bool wasEmpty = !m_mediaPlayer2;
    m_path = path;
    MediaModel::ref().setPath(path);
    m_appName = playerIdentity(path);

    // save paths
    if (!m_mediaPaths.contains(path))
        m_mediaPaths.append(path);

    if (m_mediaPlayer2)
        m_mediaPlayer2->deleteLater();

    m_mediaPlayer2 = newMediaPlayer;

    connect(m_mediaPlayer2, &DBusMediaPlayer2::MetadataChanged, this, &MediaController::onMetaDataChanged);
    connect(m_mediaPlayer2, &DBusMediaPlayer2::PlaybackStatusChanged, this, &MediaController::onPlaybackStatusChanged);
    connect(m_mediaPlayer2, &DBusMediaPlayer2::CanControlChanged, &MediaModel::ref(), &MediaModel::onCanControlChanged);
    connect(m_mediaPlayer2, &DBusMediaPlayer2::CanGoNextChanged, this, &MediaController::canGoNextChanged);
    connect(m_mediaPlayer2, &DBusMediaPlayer2::CanGoPreviousChanged, this, &MediaController::canGoPreviousChanged);
    connect(m_mediaPlayer2, &DBusMediaPlayer2::CanControlChanged, this, [this] {
        Q_EMIT canTogglePlaybackChanged(canTogglePlayback());
    });
    connect(m_mediaPlayer2, &DBusMediaPlayer2::CanPlayChanged, this, [this] {
        Q_EMIT canTogglePlaybackChanged(canTogglePlayback());
    });
    connect(m_mediaPlayer2, &DBusMediaPlayer2::CanPauseChanged, this, [this] {
        Q_EMIT canTogglePlaybackChanged(canTogglePlayback());
    });

    syncPlayerProperties();
    onMetaDataChanged();
    onPlaybackStatusChanged();

    if (wasEmpty)
        Q_EMIT mediaAcquired();
}

void MediaController::onMetaDataChanged()
{
    if (!m_mediaPlayer2) return;

    const auto &meta = m_mediaPlayer2->metadata();
    MediaModel::MediaInfo info;
    info.title = metadataText(meta.value("xesam:title"));
    info.artist = metadataText(meta.value("xesam:artist"));
    const QString artSource = meta.value("mpris:artUrl").toString();

    const QString artPath = QUrl(artSource).toLocalFile();
    if (!artPath.isEmpty()) {
        QImageReader reader(artPath);
        reader.setScaledSize(QSize(32, 32));
        info.pixmap = QPixmap::fromImage(reader.read());
    }
    MediaModel::ref().setMediaInfo(info);
    Q_EMIT mediaInfoChanged();
}

void MediaController::onPlaybackStatusChanged()
{
    if (!m_mediaPlayer2)
        return;
    const QString &stat = m_mediaPlayer2->playbackStatus();
    const bool playing = stat == "Playing";
    MediaModel::ref().setPlayState(playing);
    Q_EMIT playingChanged(playing);
}

void MediaController::removeMediaPath(const QString &path)
{
    m_mediaPaths.removeOne(path);

    if (m_path != path || !m_mediaPlayer2)
        return;

    if (!m_mediaPaths.isEmpty()) {
         loadMediaPath(m_mediaPaths.last());
         return;
    }

    m_mediaPlayer2->deleteLater();
    m_mediaPlayer2 = nullptr;
    m_path.clear();
    MediaModel::ref().setPath(QString());
    MediaModel::ref().setMediaInfo(MediaModel::MediaInfo());
    MediaModel::ref().setPlayState(false);
    MediaModel::ref().onCanControlChanged(false);
    clearCardState();

    Q_EMIT mediaLosted();
}

void MediaController::next()
{
    if (m_mediaPlayer2) {
        m_mediaPlayer2->Next();
    }
}

void MediaController::previous()
{
    if (m_mediaPlayer2) {
        m_mediaPlayer2->Previous();
    }
}

void MediaController::togglePlayback()
{
    if (m_mediaPlayer2) {
        m_mediaPlayer2->PlayPause();
    }
}

void MediaController::raise()
{
    if (!m_path.isEmpty()) {
        QDBusInterface inter(m_path, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2", QDBusConnection::sessionBus());
        inter.asyncCall("Raise");
    }
}

void MediaController::pause()
{
    if (m_mediaPlayer2) {
        m_mediaPlayer2->Pause();
    }
}

void MediaController::play()
{
    if (m_mediaPlayer2) {
        m_mediaPlayer2->Play();
    }
}

bool MediaController::isWorking() const
{
    return m_mediaPlayer2 != nullptr;
}

void MediaController::syncPlayerProperties()
{
    if (m_mediaPlayer2) {
        MediaModel::ref().onCanControlChanged(m_mediaPlayer2->canControl());
    }

    Q_EMIT canGoNextChanged(canGoNext());
    Q_EMIT canGoPreviousChanged(canGoPrevious());
    Q_EMIT canTogglePlaybackChanged(canTogglePlayback());
}

bool MediaController::playing() const
{
    return m_mediaPlayer2 && m_mediaPlayer2->playbackStatus() == QStringLiteral("Playing");
}

bool MediaController::canGoNext() const
{
    return m_mediaPlayer2 && m_mediaPlayer2->canGoNext();
}

bool MediaController::canGoPrevious() const
{
    return m_mediaPlayer2 && m_mediaPlayer2->canGoPrevious();
}

bool MediaController::canTogglePlayback() const
{
    return m_mediaPlayer2
        && m_mediaPlayer2->canControl()
        && (m_mediaPlayer2->canPlay() || m_mediaPlayer2->canPause());
}

QString MediaController::titleText() const
{
    if (!m_mediaPlayer2) {
        return tr("Music");
    }

    const auto metadata = m_mediaPlayer2->metadata();
    const QString title = metadataText(metadata.value("xesam:title"));
    if (!title.isEmpty()) {
        return title;
    }

    const QString artist = metadataText(metadata.value("xesam:artist"));
    if (!artist.isEmpty()) {
        return artist;
    }

    return tr("Music");
}

QString MediaController::subtitleText() const
{
    if (m_mediaPlayer2) {
        const auto metadata = m_mediaPlayer2->metadata();
        const QString title = metadataText(metadata.value("xesam:title"));
        const QString artist = metadataText(metadata.value("xesam:artist"));
        if (!artist.isEmpty() && artist != title) {
            return artist;
        }
    }

    return m_appName;
}

QString MediaController::artSource() const
{
    return m_mediaPlayer2
        ? m_mediaPlayer2->metadata().value("mpris:artUrl").toString()
        : QString();
}

bool MediaController::hasArt() const
{
    return !artSource().isEmpty();
}

void MediaController::clearCardState()
{
    m_appName.clear();
    Q_EMIT mediaInfoChanged();
    Q_EMIT playingChanged(false);
    syncPlayerProperties();
}
