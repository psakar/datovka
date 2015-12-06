/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#include "src/worker/pool.h"

WorkerPool globWorkPool(1); /*!< Only one worker thread currently. */

WorkerThread::WorkerThread(WorkerPool *pool)
    : m_pool(pool)
{
}

void WorkerThread::run(void)
{
	WorkerPool::run(m_pool);
}

WorkerPool::WorkerPool(unsigned threads, QObject *parent)
    : QObject(parent),
    m_threadPtrs(threads),
    m_lock(QMutex::NonRecursive),
    m_wake(),
    m_terminating(false),
    m_suspended(false),
    m_running(0),
    m_tasks()
{
	for (unsigned i = 0; i < threads; ++i) {
		m_threadPtrs[i] = new (std::nothrow) WorkerThread(this);
	}
}

WorkerPool::~WorkerPool(void)
{
	for (int i = 0; i < m_threadPtrs.size(); ++i) {
		delete m_threadPtrs[i]; m_threadPtrs[i] = 0;
	}
}

void WorkerPool::start(void)
{
	m_terminating = false;
	m_suspended = false;

	for (int i = 0; i < m_threadPtrs.size(); ++i) {
		m_threadPtrs[i]->start();
	}
}

void WorkerPool::stop(void)
{
	m_lock.lock();
	m_terminating = true;
	m_wake.wakeAll();
	m_lock.unlock();

	for (int i = 0; i < m_threadPtrs.size(); ++i) {
		m_threadPtrs[i]->wait();
	}
}

void WorkerPool::suspend(void)
{
	m_lock.lock();
	m_suspended = true;
	m_lock.unlock();
}

void WorkerPool::resume(void)
{
	m_lock.lock();
	m_suspended = false;
	m_wake.wakeAll();
	m_lock.unlock();
}

void WorkerPool::wait(void)
{
	m_lock.lock();
	while (!m_tasks.isEmpty() || (m_running > 0)) {
		m_wake.wait(&m_lock);
	}
	m_lock.unlock();
}

void WorkerPool::assign(QRunnable *task)
{
	if (0 == task) {
		return;
	}

	m_lock.lock();
	m_tasks.enqueue(task);
	m_wake.wakeAll();
	m_lock.unlock();
}

void WorkerPool::clear(void)
{
	m_lock.lock();
	while (!m_tasks.isEmpty()) {
		QRunnable *task = m_tasks.dequeue();
		if (task->autoDelete()) {
			delete task;
		}
	}
	m_lock.unlock();
}

bool WorkerPool::working(void)
{
	bool isWorking = false;

	m_lock.lock();
	isWorking = !((0 == m_running) && m_tasks.isEmpty());
	m_lock.unlock();

	return isWorking;
}

void WorkerPool::run(WorkerPool *pool)
{
	Q_ASSERT(0 != pool);

	pool->m_lock.lock();

	forever {
		if (pool->m_terminating) {
			break;
		}

		QRunnable *task = 0;
		if (!pool->m_suspended) {
			if (!pool->m_tasks.isEmpty()) {
				task = pool->m_tasks.dequeue();
			}
		}

		if (0 == task) {
			if (0 == pool->m_running) {
				emit pool->finished();
			}
			pool->m_wake.wait(&pool->m_lock);
			continue;
		}

		++pool->m_running;

		pool->m_lock.unlock();
		task->run();
		if (task->autoDelete()) {
			delete task;
		}
		pool->m_lock.lock();

		--pool->m_running;
		pool->m_wake.wakeAll();
	}

	pool->m_lock.unlock();
}
