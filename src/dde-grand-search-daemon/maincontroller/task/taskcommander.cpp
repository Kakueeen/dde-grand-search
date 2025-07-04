// SPDX-FileCopyrightText: 2021 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "taskcommander.h"
#include "taskcommander_p.h"

#include <QDateTime>
#include <QtConcurrent>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDaemon)

using namespace GrandSearch;

TaskCommanderPrivate::TaskCommanderPrivate(TaskCommander *parent)
    : QObject(parent),
      q(parent)
{
    qCDebug(logDaemon) << "TaskCommanderPrivate constructor";
}

TaskCommanderPrivate::~TaskCommanderPrivate()
{
    qCDebug(logDaemon) << "TaskCommanderPrivate destructor - Task ID:" << m_id;
}

void TaskCommanderPrivate::merge(MatchedItemMap &addTo, const MatchedItemMap &addFrom)
{
    qCDebug(logDaemon) << "Merging results - Target groups:" << addTo.size()
                       << "Source groups:" << addFrom.size();

    for (auto iter = addFrom.begin(); iter != addFrom.end(); ++iter) {
        MatchedItemMap::iterator org = addTo.find(iter.key());
        if (org == addTo.end()) {
            addTo.insert(iter.key(), iter.value());
        } else {
            org.value().append(iter.value());
        }
    }
}

void TaskCommanderPrivate::working(ProxyWorker *worker)
{
    Q_ASSERT(worker);
    qCDebug(logDaemon) << "Starting sync worker execution";
    worker->working(nullptr);
    qCDebug(logDaemon) << "Sync worker execution completed";
}

void TaskCommanderPrivate::onUnearthed(ProxyWorker *worker)
{
    Q_ASSERT(worker);

    qCDebug(logDaemon) << "Worker results unearthed - Worker:" << worker
                       << "Has items:" << worker->hasItem();

    auto isEmpty = [](const MatchedItemMap &map) {
        for (const MatchedItems &it : map.values()) {
            if (!it.isEmpty())
                return false;
        }
        return true;
    };

    if (m_allWorkers.contains(worker) && worker->hasItem()) {
        auto results = worker->takeAll();
        qCDebug(logDaemon) << "Taking results from worker - Result groups:" << results.size();

        QWriteLocker lk(&m_lock);
        // check buffer has real item.
        bool emptyBuffer = isEmpty(m_buffer);

        merge(m_results, results);
        merge(m_buffer, results);

        bool emptyBuffer2 = isEmpty(m_buffer);
        // 回到主线程发送信号
        if (emptyBuffer && !emptyBuffer2) {
            qCDebug(logDaemon) << "Buffer transitioned from empty to non-empty, emitting matched signal";
            QMetaObject::invokeMethod(q, "matched", Qt::QueuedConnection);
        }
    }
}

void TaskCommanderPrivate::onFinished()
{
    qCDebug(logDaemon) << "Task finished - Working workers:" << m_workingWorkers.size()
                       << "Finished:" << m_finished << "Sender:" << sender();
    // 工作线程退出，若之前调用了deleteSelf那么在这里执行释放，否则发送结束信号
    if (m_asyncLine.isFinished() && m_syncLine.isFinished()) {
        if (m_deleted) {
            qCDebug(logDaemon) << "Task marked for deletion, deleting now - ID:" << m_id;
            q->deleteLater();
            disconnect(q, nullptr, nullptr, nullptr);
        } else if (m_workingWorkers.isEmpty() && !m_finished) {
            m_finished = true;
            qCDebug(logDaemon) << "All workers completed, task finished - ID:" << m_id;
            emit q->finished();
        }
    }
}

void TaskCommanderPrivate::onWorkFinished(ProxyWorker *worker)
{
    // 检查
    ProxyWorker *send = dynamic_cast<ProxyWorker *>(sender());
    if (worker == nullptr || send != worker) {
        qCWarning(logDaemon) << "Invalid worker finish notification - Expected:" << worker
                             << "Actual sender:" << send;
        return;
    }

    qCDebug(logDaemon) << "Worker finished - Removing from working list";
    m_workingWorkers.removeOne(worker);
    qCDebug(logDaemon) << "Remaining working workers:" << m_workingWorkers.size();

    onFinished();
}

TaskCommander::TaskCommander(const QString &content, QObject *parent)
    : QObject(parent),
      d(new TaskCommanderPrivate(this))
{
    d->m_id = QString::number(QDateTime::currentMSecsSinceEpoch());
    d->m_content = content;

    qCDebug(logDaemon) << "TaskCommander created - ID:" << d->m_id
                       << "Content length:" << content.size();
}

TaskCommander::~TaskCommander()
{
    qCDebug(logDaemon) << "TaskCommander destructor - ID:" << d->m_id;
}

QString TaskCommander::taskID() const
{
    return d->m_id;
}

QString TaskCommander::content() const
{
    return d->m_content;
}

void TaskCommander::setContent(const QString &content)
{
    qCDebug(logDaemon) << "Task content updated - ID:" << d->m_id
                       << "Old length:" << d->m_content.size()
                       << "New length:" << content.size();
    d->m_content = content;
}

bool TaskCommander::start()
{
    qCInfo(logDaemon) << "Starting task - ID:" << this->taskID()
                      << "Already working:" << d->m_working
                      << "Async workers:" << d->m_asyncWorkers.size()
                      << "Sync workers:" << d->m_syncWorkers.size();

    if (d->m_working) {
        qCWarning(logDaemon) << "Task already running - ID:" << this->taskID();
        return false;
    }

    d->m_working = true;
    bool isOn = false;
    // 所有异步搜索项在一个线程中依次执行
    if (!d->m_asyncWorkers.isEmpty()) {
        qCDebug(logDaemon) << "Starting async workers execution - Count:" << d->m_asyncWorkers.size();

        d->m_asyncLine.setFuture(QtConcurrent::run([this]() {
            QString taskID = d->m_id;
            for (auto worker : d->m_asyncWorkers) {
                if (!d->m_working) {
                    qCDebug(logDaemon) << "Task stopped during async execution - Task ID:" << taskID;
                    return;
                }

                if (worker->working(&taskID)) {
                    // 在主线程处理搜索结束
                    d->connect(worker, &ProxyWorker::asyncFinished, d, &TaskCommanderPrivate::onWorkFinished, Qt::QueuedConnection);
                    d->m_workingWorkers.append(worker);
                }
            }
        }));
        connect(&d->m_asyncLine, &QFutureWatcherBase::finished, d, &TaskCommanderPrivate::onFinished);
        isOn = true;
    }

    // 所有同步搜索项在线程池中执行
    if (!d->m_syncWorkers.isEmpty()) {
        qCDebug(logDaemon) << "Starting sync workers execution - Count:" << d->m_syncWorkers.size();

        d->m_syncLine.setFuture(QtConcurrent::map(d->m_syncWorkers, TaskCommanderPrivate::working));
        connect(&d->m_syncLine, &QFutureWatcherBase::finished, d, &TaskCommanderPrivate::onFinished);
        isOn = true;
    }

    // 无工作对象，直接结束。
    if (!isOn) {
        d->m_working = false;
        qCWarning(logDaemon) << "No workers available for task - ID:" << this->taskID();
        // 加入队列，在start函数返回后发送结束信号
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }

    return true;
}

void TaskCommander::stop()
{
    qCDebug(logDaemon) << "Stopping task - ID:" << this->taskID();
    d->m_asyncLine.cancel();
    d->m_syncLine.cancel();

    for (auto worker : d->m_allWorkers) {
        Q_ASSERT(worker);
        worker->terminate();
    }
    qCDebug(logDaemon) << "All workers stopped - Task ID:" << this->taskID();

    d->m_working = false;
    d->m_finished = true;
}

MatchedItemMap TaskCommander::getResults() const
{
    QReadLocker lk(&d->m_lock);
    return d->m_results;
}

MatchedItemMap TaskCommander::readBuffer() const
{
    QWriteLocker lk(&d->m_lock);
    decltype(d->m_buffer) ret;
    ret = std::move(d->m_buffer);
    return ret;
}

bool TaskCommander::isEmptyBuffer() const
{
    QReadLocker lk(&d->m_lock);
    return d->m_buffer.isEmpty();
}

bool TaskCommander::join(ProxyWorker *worker)
{
    // 已经开启任务不允许添加搜索项
    if (d->m_working) {
        qCWarning(logDaemon) << "Cannot join worker - Task already running - ID:" << this->taskID();
        return false;
    }

    Q_ASSERT(worker);

    qCDebug(logDaemon) << "Joining worker to task - ID:" << this->taskID()
                       << "Worker async:" << worker->isAsync();

    worker->setParent(d);
    worker->setContext(d->m_content);
    d->m_allWorkers.append(worker);

    if (worker->isAsync()) {
        d->m_asyncWorkers.append(worker);
        qCDebug(logDaemon) << "Worker added to async list - Total async workers:" << d->m_asyncWorkers.size();
    } else {
        d->m_syncWorkers.append(worker);
        qCDebug(logDaemon) << "Worker added to sync list - Total sync workers:" << d->m_syncWorkers.size();
    }

    // 直连，在线程处理
    connect(worker, &ProxyWorker::unearthed, d, &TaskCommanderPrivate::onUnearthed, Qt::DirectConnection);

    qCDebug(logDaemon) << "Worker joined successfully - Task ID:" << this->taskID()
                       << "Total workers:" << d->m_allWorkers.size();

    return true;
}

void TaskCommander::deleteSelf()
{
    qCDebug(logDaemon) << "Deletion requested - ID:" << this->taskID()
                       << "Async finished:" << d->m_asyncLine.isFinished()
                       << "Sync finished:" << d->m_syncLine.isFinished();

    // 工作线程未完全退出时不执行释放，待退出后再释放
    if (d->m_asyncLine.isFinished() && d->m_syncLine.isFinished()) {
        qCDebug(logDaemon) << "Deleting task immediately - ID:" << this->taskID();
        delete this;
    } else {
        qCDebug(logDaemon) << "Marking task for deletion - ID:" << this->taskID();
        d->m_deleted = true;
    }
}

bool TaskCommander::isFinished() const
{
    return d->m_finished;
}
