// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef MEDIACONTROLLER_H
#define MEDIACONTROLLER_H

#include "dbusmediaplayer2.h"
#include "mediamonitor.h"

#include <QObject>
#include <QDBusInterface>

class MediaController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY canGoPreviousChanged)
    Q_PROPERTY(bool canTogglePlayback READ canTogglePlayback NOTIFY canTogglePlaybackChanged)
    Q_PROPERTY(QString titleText READ titleText NOTIFY mediaInfoChanged)
    Q_PROPERTY(QString subtitleText READ subtitleText NOTIFY mediaInfoChanged)
    Q_PROPERTY(QString artSource READ artSource NOTIFY mediaInfoChanged)
    Q_PROPERTY(bool hasArt READ hasArt NOTIFY mediaInfoChanged)
public:
    MediaController();
    ~MediaController() {}

    bool isWorking() const;
    bool playing() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canTogglePlayback() const;
    QString titleText() const;
    QString subtitleText() const;
    QString artSource() const;
    bool hasArt() const;

    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void play();
    Q_INVOKABLE void raise();
    const QString &mediaPath() const { return m_path; }

Q_SIGNALS:
    void mediaAcquired() const;
    void mediaChanged() const;
    void mediaLosted() const;
    void playingChanged(bool playing) const;
    void canGoNextChanged(bool canGoNext) const;
    void canGoPreviousChanged(bool canGoPrevious) const;
    void canTogglePlaybackChanged(bool canTogglePlayback) const;
    void mediaInfoChanged() const;

public Q_SLOTS:
    void loadMediaPath(const QString &path);
    void removeMediaPath(const QString &path);
    void onMetaDataChanged();
    void onPlaybackStatusChanged();

private:
    void finishLoadMediaPath(const QString &path);
    void syncPlayerProperties();
    void clearCardState();

private:
    DBusMediaPlayer2 *m_mediaPlayer2;
    MediaMonitor *m_mediaMonitor;

    QString     m_path;
    QStringList m_mediaPaths;
    QString     m_appName;
};

#endif
