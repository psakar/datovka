/*
 * Copyright (C) 2014-2016 CZ.NIC
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
#include <QSet>
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
	 * @brief Whether to enqueue at the front or the rear of the queue.
	 */
	enum EnqueueOrder {
		APPEND = 0,
		PREPEND
	};

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
	 * @brief Assign a low priority task to be performed by a worker in
	 *     the pool.
	 *
	 * @param[in] task  Task to be performed by the worker.
	 * @param[in] order Whether to prepend a task, default is append.
	 */
	void assignLo(QRunnable *task, enum EnqueueOrder order = APPEND);

	/*!
	 * @brief Assign a high priority task to be performed by a worker in
	 *     the pool.
	 *
	 * @param[in] task  Task to be performed by the worker.
	 * @param[in] order Whether to prepend a task, default is append.
	 */
	void assignHi(QRunnable *task, enum EnqueueOrder order = APPEND);

	/*!
	 * @brief Run a single task to be performed by a worker in the pool.
	 *
	 * @note Blocks until it can be enqueued and waits for being finished.
	 *
	 * @param[in] task Task to be performed by the worker.
	 */
	void runSingle(QRunnable *task);

	/*!
	 * @brief Clear all tasks enqueued in pool processing queue.
	 */
	void clear(void);

	/*!
	 * @brief Return true if some workers have jobs to do.
	 */
	bool working(void);

signals:
	/*!
	 * @brief Emitted when all jobs finished and queues are empty.
	 */
	void finished(void);

	/*!
	 * @brief Emitted when all enqueued jobs finished and queues are empty.
	 */
	void assignedFinished(void);

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

	/*
	 * Single task has the highest priority. It is used when the code
	 * has to directly wait for the results -- it may block the event loop.
	 * Single tasks are advised to have the auto delete property disabled.
	 *
	 * Tasks in high priority queue take precedence over all low priority
	 * tasks. High priority tasks are mainly used for sending messages,
	 * whereas low priority tasks are prevalently used for downloading
	 * data. Tasks in these queues should have their auto delete properties
	 * enabled as there is currently no other mechanism how to delete them.
	 */
	QRunnable *m_singleTask; /*!< Single task. */
	QQueue<QRunnable *> m_tasksHi; /*!< Queue of high priority tasks. */
	QQueue<QRunnable *> m_tasksLo; /*!< Queue of low priority tasks. */

	QSet<QRunnable *> m_dequeuedRunning; /*!<
	                                      * Set of running tasks except
	                                      * the single task.
	                                      */

	/*
	 * Single task has the highest priority. The runSingle() method
	 * blocks when a single task has already been assigned but didn't
	 * finish.
	 *
	 * The single task is meant a as a synchronous blocking supplement of
	 * direct worker calls such as download single message.
	 */

	enum ExecutionState {
		PENDING, /*!< Task waiting to be executed. */
		EXECUTING, /*!< Task currently being executed. */
		FINISHED /*!< Task execution finished. */
	};

	enum ExecutionState m_singleState; /*!< Single execution state. */
};

/*!
 * @brief Global instance of the structure.
 */
extern WorkerPool globWorkPool;

#endif /* _POOL_H_ */
