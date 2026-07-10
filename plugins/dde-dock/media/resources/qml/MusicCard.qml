// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import Qt5Compat.GraphicalEffects
import org.deepin.dtk 1.0 as D

Item {
    id: root

    implicitWidth: 190
    implicitHeight: 40

    readonly property bool dark: D.DTK.themeType === D.ApplicationHelper.DarkType
    readonly property int contentHeight: {
        const cardHeight = Math.max(30, height > 0 ? height : implicitHeight)
        const verticalInset = Math.max(4, Math.round(cardHeight * 0.16))
        return Math.max(24, cardHeight - verticalInset * 2)
    }
    readonly property int artworkSize: Math.max(22, Math.min(34, Math.round(contentHeight * 0.86)))
    readonly property int buttonSize: Math.max(26, Math.min(30, contentHeight))
    readonly property int buttonSpacing: Math.max(3, Math.round(contentHeight * 0.07))
    readonly property int animationDuration: 100
    property real hoverProgress: hoverHandler.hovered ? 1 : 0

    Behavior on hoverProgress {
        NumberAnimation {
            duration: root.animationDuration + 70
            easing.type: Easing.OutCubic
        }
    }

    HoverHandler {
        id: hoverHandler
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: mediaController.raise()
    }

    component MusicActionButton: Item {
        id: button

        property string iconName
        property int layoutIndex
        property int hoverDelay
        property real revealProgress: hoverHandler.hovered ? 1 : 0
        readonly property bool interactive: enabled && revealProgress > 0.55

        signal clicked()

        implicitWidth: root.buttonSize
        implicitHeight: root.buttonSize
        x: {
            const collapsedX = (parent.width - width) / 2
            const expandedX = layoutIndex * (root.buttonSize + root.buttonSpacing)
            return collapsedX + (expandedX - collapsedX) * revealProgress
        }
        scale: 0.7 + revealProgress * 0.3
        opacity: revealProgress

        Behavior on revealProgress {
            SequentialAnimation {
                PauseAnimation {
                    duration: hoverHandler.hovered ? button.hoverDelay : 0
                }
                NumberAnimation {
                    duration: hoverHandler.hovered ? root.animationDuration + 110 : root.animationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        HoverHandler {
            id: buttonHoverHandler
            enabled: button.interactive
        }

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: root.dark ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.08)
            opacity: buttonHoverHandler.hovered ? button.revealProgress : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: root.animationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        Image {
            id: iconImage

            anchors.centerIn: parent
            width: 16
            height: 16
            source: "qrc:/icons/deepin/builtin/texts/" + button.iconName + "_24px.svg"
            sourceSize: Qt.size(width, height)
            fillMode: Image.PreserveAspectFit
            smooth: true
            visible: false
        }

        ColorOverlay {
            anchors.fill: iconImage
            source: iconImage
            color: root.dark ? "white" : "black"
            opacity: button.enabled ? 1 : 0.38
        }

        MouseArea {
            anchors.fill: parent
            enabled: button.interactive
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: button.clicked()
        }
    }

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: root.contentHeight

        Item {
            id: artworkSlot

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(root.artworkSize, root.contentHeight + 2)
            height: width

            Rectangle {
                id: artworkFrame

                anchors.centerIn: parent
                width: root.artworkSize
                height: root.artworkSize
                radius: 4
                color: root.dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.06)
                visible: mediaController.hasArt && artworkImage.status === Image.Ready
            }

            Image {
                anchors.centerIn: parent
                width: root.artworkSize
                height: root.artworkSize
                source: "qrc:/deepin-music.svg"
                sourceSize: Qt.size(width, height)
                fillMode: Image.PreserveAspectFit
                smooth: true
                visible: !artworkFrame.visible
            }

            Image {
                id: artworkImage

                anchors.fill: artworkFrame
                visible: false
                source: mediaController.artSource
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false
                sourceSize: {
                    const pixelSize = Math.round(40 * Math.max(1, Screen.devicePixelRatio))
                    return Qt.size(pixelSize, pixelSize)
                }
                mipmap: true
                smooth: true
            }

            Rectangle {
                id: artworkMask

                anchors.fill: artworkFrame
                radius: 4
                visible: false
            }

            OpacityMask {
                anchors.fill: artworkFrame
                source: artworkImage
                maskSource: artworkMask
                visible: artworkFrame.visible
                cached: false
            }

            Rectangle {
                anchors.fill: artworkFrame
                radius: 4
                color: "transparent"
                border.width: 1
                border.color: Qt.rgba(0, 0, 0, 0.2)
                visible: artworkFrame.visible
            }
        }

        Item {
            anchors.left: artworkSlot.right
            anchors.leftMargin: 4
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: Math.max(infoColumn.implicitHeight, controls.height)
            clip: true

            Column {
                id: infoColumn

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                spacing: 1
                opacity: 1 - root.hoverProgress
                scale: 1 - root.hoverProgress * 0.08
                x: -10 * root.hoverProgress
                visible: opacity > 0.01

                Text {
                    width: parent.width
                    text: mediaController.titleText
                    color: root.dark ? Qt.rgba(1, 1, 1, 0.96) : Qt.rgba(0, 0, 0, 0.92)
                    font.pixelSize: Math.max(11, Math.round(root.contentHeight * 0.4))
                    renderType: Text.NativeRendering
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: mediaController.subtitleText
                    color: root.dark ? Qt.rgba(1, 1, 1, 0.68) : Qt.rgba(0, 0, 0, 0.58)
                    font.pixelSize: Math.max(9, Math.round(root.contentHeight * 0.24))
                    renderType: Text.NativeRendering
                    elide: Text.ElideRight
                    visible: text.length > 0
                }
            }

            Item {
                id: controls

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: root.buttonSize * 3 + root.buttonSpacing * 2
                height: root.buttonSize

                MusicActionButton {
                    layoutIndex: 0
                    hoverDelay: 24
                    iconName: "play-previous"
                    enabled: mediaController.canGoPrevious
                    onClicked: mediaController.previous()
                }

                MusicActionButton {
                    layoutIndex: 1
                    iconName: mediaController.playing ? "play-pause" : "play-start"
                    enabled: mediaController.canTogglePlayback
                    onClicked: mediaController.togglePlayback()
                }

                MusicActionButton {
                    layoutIndex: 2
                    hoverDelay: 48
                    iconName: "play-next"
                    enabled: mediaController.canGoNext
                    onClicked: mediaController.next()
                }
            }
        }
    }
}
