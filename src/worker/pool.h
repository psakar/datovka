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

#ifndef _POOL_H_
#define _POOL_H_

#include <QMutex>
#include <QQueue>
#include <QObject>
#include <QRunnable>
#include <QThread>
#include <QVector>
#include <QWaitCondition>

/*
 * QThreadPool is not be best choice.
 * Worker objects are unnecessary.
 */

class WorkerPool;

/*!
 * @brief Worker.
 */
class WorkerThread : public QThread {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] pool Pointer to be stored.
	 */
	WorkerThread(WorkerPool *pool);

protected:
	/*!
	 * @brief Runs the worker code.
	 */
	virtual
	void run(void);

	friend class WorkerPool;

private:
	WorkerPool *m_pool; /* Pointer to worker pool, must be non-null. */
};

/*!
 * @brief Pool of workers.
 */
class WorkerPool : public QObject {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] threads Number of threads to create.
	 * @param[in] parent  Object parent.
	 */
	WorkerPool(unsigned threads, QObject *parent = 0);

	/*!
	 * @brief Destructor.
	 */
	~WorkerPool(void);

	/*!
	 * @brief Start all threads in the worker pool.
	 */
	void start(void);

	/*!
	 * @brief Stop processing of new tasks, start stopping worker threads
	 *     when possible.
	 */
	void stop(void);

	/*!
	 * @brief Temporarily suspend the execution of worker pool.
	 */
	void suspend(void);

	/*!
	 * @brief Resume the execution of worker pool.
	 */
	void resume(void);

	/*!
	 * @brief Wait till the number of pending tasks is zero.
	 */
	void wait(void);

	/*!
	 * @brief Assign a task to be performed by a worker in the pool.
	 *
	 * @param[in] task Task to be performed by the worker.
	 */
	void assign(QRunnable *task);

	/*!
	 * @brief Clear all tasks enqueued in pool processing queue.
	 */
	void clear(void);

protected:
	/*!
	 * @brief Worker code.
	 */
	static
	void run(WorkerPool *pool);

	friend class WorkerThread;

private:
	Q_DISABLE_COPY(WorkerPool)

	QVector<WorkerThread *> m_threadPtrs; /*!< Pool of threads. */

	QMutex m_lock;
	QWaitCondition m_wake;

	bool m_terminating; /*!< Is the pool terminating? */
	bool m_suspended; /*!< Is the execution temporarily suspended? */
	int m_running; /*!< Number of running threads. */

	QQueue<QRunnable *> m_tasks; /*!< Queue of tasks. */
};

/*!
 * @brief Global instance of the structure.
 */
extern WorkerPool globWorkPool;

#endif /* _POOL_H_ */
