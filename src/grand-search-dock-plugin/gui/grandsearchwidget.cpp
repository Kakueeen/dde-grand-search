// SPDX-FileCopyrightText: 2021 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "versiondefine.h"

#include "grandsearchwidget.h"
#include "interfaces/grandsearchinterface.h"

#include <DGuiApplicationHelper>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    include <DGuiApplicationHelper>
#else
#    include <DApplicationHelper>
#endif
#include <DStyleHelper>
#include <DFontSizeManager>

#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QVBoxLayout>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDock)

using namespace GrandSearch;
DWIDGET_USE_NAMESPACE

static QPixmap iconPixmap(const QString &fileName, const QSize &size, qreal ratio)
{
    QString iconPath = QString(":/icons/%1.dci").arg(fileName);
    QPixmap pixmap;
    DDciIcon dciIcon = DDciIcon::fromTheme(iconPath);
    if (!dciIcon.isNull()) {
        auto theme = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType
                ? DDciIcon::Light
                : DDciIcon::Dark;
        pixmap = dciIcon.pixmap(ratio, size.width(), theme);
        qCDebug(logDock) << "Icon loaded from DCI theme - File:" << fileName << "Size:" << size << "Theme:" << (theme == DDciIcon::Light ? "Light" : "Dark");
    } else {
        iconPath = QString(":/icons/%1.svg").arg(fileName);
        pixmap = QIcon::fromTheme(iconPath).pixmap(size);
        qCDebug(logDock) << "Icon loaded from SVG fallback - File:" << fileName << "Size:" << size;
    }

    if (pixmap.isNull()) {
        qCWarning(logDock) << "Failed to load icon - File:" << fileName;
    }

    return pixmap;
}

GrandSearchWidget::GrandSearchWidget(QWidget *parent)
    : QWidget(parent)
{
    qCDebug(logDock) << "Initializing GrandSearchWidget";

    setMouseTracking(true);

    m_grandSearchInterface = new GrandSearchInterface(this);
    connect(m_grandSearchInterface, &GrandSearchInterface::VisibleChanged, this, &GrandSearchWidget::grandSearchVisibleChanged);
    qCDebug(logDock) << "GrandSearchInterface created and connected";

    bool isRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered("com.deepin.dde.GrandSearch");
    qCDebug(logDock) << "Checking Grand Search D-Bus service registration - Registered:" << isRegistered;

    if (isRegistered) {
        m_grandSearchVisible = m_grandSearchInterface->IsVisible();
        qCDebug(logDock) << "Grand Search D-Bus service found - Initial visibility:" << m_grandSearchVisible;
    } else {
        m_grandSearchVisible = false;
        qCWarning(logDock) << "Grand Search D-Bus service not registered - Setting visibility to false";
    }

    qCDebug(logDock) << "GrandSearchWidget initialization completed";
}

GrandSearchWidget::~GrandSearchWidget()
{
    qCDebug(logDock) << "GrandSearchWidget destructor called";
}

QString GrandSearchWidget::itemCommand(const QString &itemKey)
{
    qCDebug(logDock) << "GrandSearchWidget itemCommand called - Item key:" << itemKey;
    Q_UNUSED(itemKey)

    m_grandSearchVisible = !m_grandSearchVisible;
    m_grandSearchInterface->SetVisible(m_grandSearchVisible);

    return QString();
}

void GrandSearchWidget::grandSearchVisibleChanged(bool visible)
{
    m_grandSearchVisible = visible;
    Q_EMIT visibleChanged(visible);
}

void GrandSearchWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    /*!
     * 1.插件如果安装在插件区域即plugins目录下，需要插件自己绘制背景色，并根据鼠标移动、悬浮、点击的状态设置不同的透明度。
     *   否则图标将无背景，鼠标移动、悬浮、点击等无背景变色等效果。
     * 2.插件如果安装在托盘区域即plugins/system-tray目录下，插件只需要绘制自己的图标icon，不需要处理鼠标状态，不能自己绘制背景。
     *   否则将出现背景被重叠绘制，出现背景颜色非常深的问题。
     */
    QPixmap pixmap;
    QString iconName = "grand-search-light";
    int iconSize = ITEM_ICON_SIZE;

    QPainter painter(this);

    if (rect().height() > PLUGIN_BACKGROUND_MIN_SIZE) {
        qCDebug(logDock) << "Drawing background for large plugin size - Height:" << rect().height();

        QColor color;
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            color = Qt::black;
            painter.setOpacity(0.5);

            if (m_hover) {
                painter.setOpacity(0.6);
            }

            if (m_pressed) {
                painter.setOpacity(0.3);
            }
        } else {
            color = Qt::white;
            painter.setOpacity(0.1);

            if (m_hover) {
                painter.setOpacity(0.2);
            }

            if (m_pressed) {
                painter.setOpacity(0.05);
            }
        }

        painter.setRenderHint(QPainter::Antialiasing, true);

        DStyleHelper dstyle(style());
        const int radius = dstyle.pixelMetric(DStyle::PM_FrameRadius);

        QPainterPath path;

        int minSize = std::min(width(), height());
        QRect rc(0, 0, minSize, minSize);
        rc.moveTo(rect().center() - rc.center());

        path.addRoundedRect(rc, radius, radius);
        painter.fillPath(path, color);
    } else if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        // 最小尺寸时，不画背景，采用深色图标
        iconName.replace(QString("-light"), PLUGIN_MIN_ICON_NAME);
        qCDebug(logDock) << "Using minimum size dark icon for light theme";
    }

    painter.setOpacity(1);
    iconSize = QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps) ? iconSize : (iconSize * devicePixelRatioF());
    pixmap = iconPixmap(iconName, QSize(iconSize, iconSize), devicePixelRatioF());

    const QRectF &rf = QRectF(rect());
    const QRectF &rfp = QRectF(pixmap.rect());
    painter.drawPixmap(rf.center() - rfp.center() / pixmap.devicePixelRatioF(), pixmap);
}

void GrandSearchWidget::mousePressEvent(QMouseEvent *event)
{
    m_pressed = true;
    update();

    QWidget::mousePressEvent(event);
}

void GrandSearchWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressed = false;
    m_hover = false;
    update();

    QWidget::mouseReleaseEvent(event);
}

void GrandSearchWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_hover = true;
    QWidget::mouseMoveEvent(event);
}

void GrandSearchWidget::leaveEvent(QEvent *event)
{
    m_hover = false;
    m_pressed = false;
    update();

    QWidget::leaveEvent(event);
}

QuickPanel::QuickPanel(const QString &desc, QWidget *parent)
    : QWidget(parent)
{
    qCDebug(logDock) << "Initializing QuickPanel with description:" << desc;

    QVBoxLayout *lay = new QVBoxLayout;
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(0);
    lay->addStretch(1);

    iconLabel = new DLabel;
    iconLabel->setFixedSize(PANEL_ICON_SIZE, PANEL_ICON_SIZE);
    iconLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(iconLabel, 0, Qt::AlignHCenter);

    DLabel *textLabel = new DLabel;
    textLabel->setText(desc);
    textLabel->setElideMode(Qt::ElideRight);
    textLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(textLabel, DFontSizeManager::T10);
    lay->addSpacing(10);
    lay->addWidget(textLabel, 0, Qt::AlignHCenter);
    lay->addStretch(1);

    setLayout(lay);

    updateIcon();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &QuickPanel::updateIcon);
    qCDebug(logDock) << "QuickPanel initialized with theme change connection";
}

void QuickPanel::updateIcon()
{
    const QString name = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType ? QString("grand-search-dark") : QString("grand-search-light");
    qCDebug(logDock) << "Updating QuickPanel icon for theme - Theme:" << (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType ? "Light" : "Dark") << "Icon:" << name;

    const auto &pixmap = iconPixmap(name, { PANEL_ICON_SIZE, PANEL_ICON_SIZE }, devicePixelRatioF());
    iconLabel->setPixmap(pixmap);

    update();
}
