// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "quickdragcore.h"
#include "../util/utils.h"
#include <dtkgui_config.h>

#include <QBitmap>
#include <QCoreApplication>
#include <QDBusInterface>
#include <QDebug>
#include <QDragEnterEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QWidget>

#include <DWindowManagerHelper>

#include <com_deepin_dde_daemon_dock.h>
#include <types/dockrect.h>

// 如果不需要鼠标穿透可以不用添加相关头文件和链接库
#ifdef NEED_TRANSPARENT_FOR_MOUSE_EVENT

#include <qpa/qplatformnativeinterface.h>
#include <QX11Info>
#include <X11/extensions/shape.h>
#include <wayland-client.h>

#endif

using DockInter = com::deepin::dde::daemon::Dock;
DWIDGET_USE_NAMESPACE

static QRect dockRect = QRect();

QuickPluginMimeData::QuickPluginMimeData(PluginWrapper* item, QDrag* drag)
    : QMimeData()
    , m_item(item)
    , m_drag(drag)
{
}

QuickPluginMimeData::~QuickPluginMimeData()
{
}

PluginWrapper* QuickPluginMimeData::pluginWrapper() const
{
    return m_item;
}

QDrag* QuickPluginMimeData::drag() const
{
    return m_drag;
}

/**
 * @brief 拖动图标的窗口，可以根据实际情况设置动态图标
 * @param dragSource
 */
QuickIconDrag::QuickIconDrag(QObject* dragSource, const QPixmap& pixmap, int radius, bool enableBlurWindow)
    : QDrag(dragSource)
    , m_imageWidget(new QWidget)
    , m_timer(new QTimer(this))
    , m_sourcePixmap(pixmap)
    , m_currentPixmap(pixmap)
    , m_hotPoint(QPoint(0, 0))
    , m_originDist(0)
    , m_radius(radius)
    , m_percent(1.0)
    , m_showAnimation(false)
    , m_handle(new Dtk::Widget::DPlatformWindowHandle(m_imageWidget))
{
    m_sourcePixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    m_currentPixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    m_timer->setInterval(10);
    connect(m_timer, &QTimer::timeout, this, &QuickIconDrag::onDragMove);
    m_timer->start();

#ifdef DTKGUI_VERSION_STR
    #if (DTK_VERSION_CHECK(DTKGUI_VERSION_MAJOR, DTKGUI_VERSION_MINOR, DTKGUI_VERSION_PATCH, DTKGUI_VERSION_BUILD) > DTK_VERSION_CHECK(5, 6, 9, 4))
        m_handle->setWindowEffect(DPlatformWindowHandle::EffectNoStart | DPlatformWindowHandle::EffectNoBorder | DPlatformWindowHandle::EffectNoShadow);
    #else
        m_handle->setBorderWidth(0); // 去掉边框
        m_handle->setShadowColor(Qt::transparent); // 去掉阴影
        m_handle->setEnableBlurWindow(enableBlurWindow);
    #endif
#else
    m_handle->setBorderWidth(0); // 去掉边框
    m_handle->setShadowColor(Qt::transparent); // 去掉阴影
    m_handle->setEnableBlurWindow(enableBlurWindow);
#endif

    m_handle->setWindowRadius(m_radius);
    m_imageWidget->setVisible(false); // 避免位置还没有设置，在其它位置闪现一下
    m_imageWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
    m_imageWidget->installEventFilter(this);
    m_imageWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_imageWidget->setFixedSize(m_sourcePixmap.size() / qApp->devicePixelRatio());
    m_imageWidget->show();
    m_imageWidget->raise();

#ifdef NEED_TRANSPARENT_FOR_MOUSE_EVENT
    // X11 下面需要将鼠标事件穿透到任务栏上，否则任务栏收不到 drag 相关事件, Wayland 使用该方法x11库会崩溃
    if (!Utils::IS_WAYLAND_DISPLAY) {
        XShapeCombineRectangles(QX11Info::display(), m_imageWidget->winId(), ShapeInput, 0, 0,
            NULL, 0, ShapeSet, YXBanded);
    } else {
        do {
            QWindow *window = m_imageWidget->windowHandle();
            if (!window) {
                qWarning() << "Window handle is null";
                break;
            }
            QPlatformNativeInterface *native = qApp->platformNativeInterface();
            if (!native) {
                qWarning() << "Platform native interface is null";
                break;
            }
            wl_display *display = reinterpret_cast<wl_display *>(native->nativeResourceForWindow(QByteArrayLiteral("display"), window));
            if (!display) {
                qWarning() << "Wayland display is null";
                break;
            }
            wl_surface *surface = reinterpret_cast<wl_surface *>(native->nativeResourceForWindow(QByteArrayLiteral("surface"), window));
            if (!surface) {
                qWarning() << "Wayland surface is null";
                break;
            }
            wl_compositor *compositor = reinterpret_cast<wl_compositor *>(native->nativeResourceForWindow(QByteArrayLiteral("compositor"), window));
            if (!compositor) {
                qWarning() << "Wayland compositor is null";
                break;
            }
            wl_region *region = wl_compositor_create_region(compositor);
            wl_region_add(region, 0, 0, 0, 0);
            wl_surface_set_input_region(surface, region);
            wl_display_flush(display);
        } while (0);
    }
#endif

    // TO OPTIMIZE 想办法去掉这个耦合?
    DockInter* inter = new DockInter("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this);
    dockRect = inter->frontendWindowRect();
    m_originDist = point2DockDist(QCursor::pos() * qApp->devicePixelRatio());
}

QuickIconDrag::~QuickIconDrag()
{
    m_imageWidget->deleteLater();
    m_handle->deleteLater();
}

void QuickIconDrag::setShowAnimation(bool show)
{
    m_showAnimation = show;
}

void QuickIconDrag::setDragHotPot(QPoint point)
{
    m_hotPoint = point;
    m_imageWidget->update();
}

int QuickIconDrag::point2DockDist(QPoint currentP)
{
    int dist = 0;
    auto postion = qApp->property(PROP_POSITION).value<Dock::Position>();
    if (postion == Dock::Bottom) {
        dist = dockRect.top() - currentP.y();
    } else if (postion == Dock::Top) {
        dist = currentP.y() - dockRect.bottom();
    } else if (postion == Dock::Left) {
        dist = currentP.x() - dockRect.right();
    } else {
        dist = dockRect.left() - currentP.x();
    }

    return dist;
}

void QuickIconDrag::setTargetPixmap(QPixmap pixmap)
{
    m_targetPixmap = pixmap;
}

void QuickIconDrag::activateDragWidget(int count)
{
    if (m_imageWidget) {
        if (count < 0)
            return;

        qDebug() << "Try active window" << count;
        if (m_imageWidget->isActiveWindow()) {
            qDebug() << "Finally active window is me";
            return;
        }

        m_imageWidget->activateWindow();
        QTimer::singleShot(50 , this, std::bind(&QuickIconDrag::activateDragWidget, this, count -1));
    }
}

bool QuickIconDrag::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_imageWidget) {
        switch (event->type()) {
        case QEvent::Paint: {
            QPainter painter(m_imageWidget);
            // 没有窗口特效时填充背景色，否则默认是黑色的
            if (!Utils::hasBlurWindow()) {
                painter.fillRect(m_imageWidget->rect(), m_imageWidget->palette().background());
            }
            painter.drawPixmap(QPoint(0, 0), m_currentPixmap);
            painter.end();
            break;
        }
        case QEvent::WindowDeactivate:
            activateDragWidget();
            break;
        default:
            break;
        }
    }
    return QDrag::eventFilter(watched, event);
}

QPoint QuickIconDrag::currentPoint() const
{
    const auto& mousePos = QCursor::pos();
    // 在图片变化的时候鼠标一直处在最开始的，看起来图片以这个点为原点进行缩放的
    if (m_showAnimation) {
        return mousePos - (m_hotPoint * m_percent);
    }
    return mousePos - m_hotPoint;
}

/**
 * @brief 移动过程中通过鼠标初始离任务栏的举例和目前离任务栏的举例计算一个比例
 * 通过比例缩放和透明图片，从而得到一个离任务栏越近图片越小越透明的效果。
 * 当图片小到和目标图片（一般是在任务栏上展示的图片）大小一样（或者小与）时，将展示的图片替换为任务栏上面的图片。
 */
void QuickIconDrag::onDragMove()
{
    auto pos = currentPoint();
    const auto ratio = qApp->devicePixelRatio();
    if (m_showAnimation) {
        const auto dist = point2DockDist(QCursor::pos() * ratio);
        m_percent = dist > 0 ? qMin(1.0, dist * 1.0 / m_originDist) : ((m_targetPixmap.width() * 1.0) / m_sourcePixmap.width());
        // 当鼠标已经在任务栏上或者图片尺寸<=目标图片的尺寸时显示目标图片
        if (dist < 0 || m_sourcePixmap.width() * m_percent <= m_targetPixmap.width()) {
            m_handle->setWindowRadius(0);
            m_currentPixmap = m_targetPixmap;
            const auto& targetSize = m_targetPixmap.size();
            const auto& sourceSize = m_sourcePixmap.size();
            // x 和 y 值需要单独计算，类似音量这种插件，宽高差距非常大，会导致鼠标偏移
            const auto runtimeHotPoint = QPoint(m_hotPoint.x() * ((targetSize.width() * 1.0) / sourceSize.width()),
                m_hotPoint.y() * ((targetSize.height() * 1.0) / sourceSize.height()));
            pos = QCursor::pos() - runtimeHotPoint;
        } else {
            m_handle->setWindowRadius(m_radius);
            m_currentPixmap = roundPixmap(m_sourcePixmap, m_radius, m_radius, m_percent);
        }
        // 缩放到一半的时候取消模糊效果，否则会出现锯齿、模糊等问题
        m_handle->setEnableBlurWindow(m_percent >= 0.5);
        m_imageWidget->setFixedSize(m_currentPixmap.size() / ratio);
        m_imageWidget->update();
    } else {
        m_currentPixmap = m_sourcePixmap;
    }

    // 避免被弹窗遮挡
    if (!m_imageWidget->isActiveWindow())
        m_imageWidget->raise();

    m_imageWidget->move(pos);
}

/**
 * @brief 构建`带圆角&缩放&透明度`的pixmap
 *
 * @param origin 原始 pixmap
 * @param xRadius 圆角值
 * @param yRadius 圆角值
 * @param percent 缩放比例和透明度
 * @param setOpacity 是否使用透明度
 * @return QPixmap
 */
QPixmap QuickIconDrag::roundPixmap(const QPixmap& origin, qreal xRadius, qreal yRadius, double percent, bool setOpacity)
{
    // 没有窗口特效时不使用透明度、圆角等效果
    const bool hasBlurWindow = Utils::hasBlurWindow();
    const auto dpi = origin.devicePixelRatioF();
    const auto realSize = QSize(origin.width(), origin.height()) * percent;

    QPixmap resultPixmap(realSize);
    if (hasBlurWindow) {
        resultPixmap.fill(Qt::transparent);
    }
    QPainter painter(&resultPixmap);
    if (hasBlurWindow) {
        QPainterPath path;
        path.addRoundedRect(0, 0, realSize.width(), realSize.height(), xRadius, yRadius);
        painter.setClipPath(path);
        if (setOpacity) {
            painter.setOpacity(percent);
        }
    }
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform); //抗锯齿 & 平滑像素图变换
    painter.drawPixmap(0, 0, realSize.width(), realSize.height(), origin.scaled(realSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    resultPixmap.setDevicePixelRatio(dpi);
    return resultPixmap;
}
