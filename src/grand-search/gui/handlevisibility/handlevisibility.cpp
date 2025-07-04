// SPDX-FileCopyrightText: 2021 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "handlevisibility.h"
#include "gui/mainwindow.h"

#include <DApplication>

#include <QtDebug>
#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logGrandSearch)

#define SessionManagerService "org.deepin.dde.SessionManager1"
#define SessionManagerPath "/org/deepin/dde/SessionManager1"

using namespace GrandSearch;
DWIDGET_USE_NAMESPACE

HandleVisibility::HandleVisibility(MainWindow *mainWindow, QObject *parent)
    : QObject(parent),
      m_mainWindow(mainWindow)
// , m_sessionManagerInter(new com::deepin::SessionManager(SessionManagerService, SessionManagerPath, QDBusConnection::sessionBus(), this))
{
    qCDebug(logGrandSearch) << "Creating HandleVisibility";
    init();
    qCDebug(logGrandSearch) << "HandleVisibility created successfully";
}

void HandleVisibility::init()
{
    qCDebug(logGrandSearch) << "Initializing visibility handler";

    m_regionMonitor = new DRegionMonitor(this);
    m_regionMonitor->setCoordinateType(DRegionMonitor::ScaleRatio);
    // 鼠标点击主界面之外的区域，退出进程
    connect(m_regionMonitor, &DRegionMonitor::buttonPress, this, &HandleVisibility::regionMousePress);
    registerRegion(true);

    // 进程变为非激活状态时，退出
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &HandleVisibility::onApplicationStateChanged);

    // 锁屏状态改变
    // FIXME: how to replace this feature on V25
    // connect(m_sessionManagerInter, &com::deepin::SessionManager::LockedChanged, this, &HandleVisibility::onLockedChanged);
}

void HandleVisibility::onApplicationStateChanged(const Qt::ApplicationState state)
{
    Q_ASSERT(m_mainWindow);

    if (Qt::ApplicationInactive == state) {
        // todo 二阶段：截图导致本进程窗口变为非激活，不退出。截图退出后，主动激活本进程主窗口。
        qCInfo(logGrandSearch) << "Application state changed to inactive - Closing main window";
        m_mainWindow->close();
    } else if (Qt::ApplicationActive == state) {
        qCDebug(logGrandSearch) << "Application state changed to active - Showing main window";
        m_mainWindow->show();
    }
}

void HandleVisibility::onLockedChanged(const bool locked)
{
    Q_ASSERT(m_mainWindow);

    qCDebug(logGrandSearch) << "Screen lock state changed - Locked:" << locked
                            << "Setting window visible:" << !locked;
    m_mainWindow->setVisible(!locked);
}

void HandleVisibility::onCloseWindow()
{
    Q_ASSERT(m_mainWindow);

    qCDebug(logGrandSearch) << "Closing main window";
    m_mainWindow->close();
}

void HandleVisibility::registerRegion(const bool isRegister)
{
    Q_ASSERT(m_regionMonitor);

    if (isRegister == m_regionMonitor->registered())
        return;

    qCDebug(logGrandSearch) << "Region registration:" << isRegister;
    // FIXME: how to fix blocked 25s issue on treeland
    /*
    if (isRegister)
        m_regionMonitor->registerRegion();
    else
        m_regionMonitor->unregisterRegion();
    */
}

void HandleVisibility::regionMousePress(const QPoint &p, const int flag)
{
    Q_UNUSED(flag)

    for (auto w : qApp->topLevelWidgets()) {
        if (w->isVisible() && w->geometry().contains(p)) {
            return;
        }
    }
    // 点击位置在程序窗口之外则退出
    qCInfo(logGrandSearch) << "Mouse click outside application window - Position:" << p
                           << "Flag:" << flag
                           << "Scheduling window close";
    // todo ：后期根据情况，可能需要设置为隐藏，而不能直接退出
    // 延迟退出，便于dock插件响应时能正确获取当前状态
    QTimer::singleShot(500, this, &HandleVisibility::onCloseWindow);
}
