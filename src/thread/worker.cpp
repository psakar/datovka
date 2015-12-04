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


#include <QDebug>
#include <QSet>
#include <QThread>

#include "worker.h"
#include "src/common.h"
#include "src/crypto/crypto_funcs.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/gui/datovka.h"
#include "src/io/isds_sessions.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task.h"


/* ========================================================================= */
/*
 * Job list constructor.
 */
Worker::JobList::JobList(void)
/* ========================================================================= */
    : QList<Job>(),
    QMutex(QMutex::NonRecursive)
{
}


/* ========================================================================= */
/*
 * Job list destructor.
 */
Worker::JobList::~JobList(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Atomic append.
 */
void Worker::JobList::append(const Worker::Job &value)
/* ========================================================================= */
{
	QMutex::lock();
	QList<Job>::append(value);
	QMutex::unlock();
}


/* ========================================================================= */
/*
 * Atomic get first and pop.
 */
Worker::Job Worker::JobList::firstPop(bool pop)
/* ========================================================================= */
{
	Job value;

	QMutex::lock();
	if (!isEmpty()) {
		value = QList<Job>::first();
		if (pop) {
			QList<Job>::removeFirst();
		}
	}
	QMutex::unlock();

	return value;
}


/* ========================================================================= */
/*
 * Atomic prepend.
 */
void Worker::JobList::prepend(const Worker::Job &value)
/* ========================================================================= */
{
	QMutex::lock();
	QList<Job>::prepend(value);
	QMutex::unlock();
}


Worker::JobList Worker::jobList;
QMutex Worker::downloadMessagesMutex(QMutex::NonRecursive);


/* ========================================================================= */
/*
 * Constructor for job list usage.
 */
Worker::Worker(QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_job()
{
}


/* ========================================================================= */
/*
 * Constructor for single job usage.
 */
Worker::Worker(const Job &job, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_job(job)
{
}


/* ========================================================================= */
/*
* Tread executing prepare
*/
void Worker::requestWork(void)
/* ========================================================================= */
{
	downloadMessagesMutex.lock();

	qDebug() << "Request worker start from Thread " <<
	    thread()->currentThreadId();

	emit workRequested();
}


/* ========================================================================= */
/*
* Download MessageList for account
*/
void Worker::doJob(void)
/* ========================================================================= */
{
	qDebug() << "Starting worker process in Thread "
	    << thread()->currentThreadId();

	Job job = m_job;

	if (!job.isValid()) {
		/* Use job queue. */
		job = jobList.firstPop(true);
	}

	/* Test whether job is valid. */
	if (!job.isValid()) {
		qDebug() << "Invalid worker job! Downloading is cancelled.";
		downloadMessagesMutex.unlock();
		emit finished();
		return;
	}

	/* Messages counters
	 * rt = receivedTotal, rn = receivedNews,
	 * st = sentTotal, sn = sentNews.
	 * Message counters are send to mainwindow and show in info-statusbar.
	*/
	int rt = 0;
	int rn = 0;
	int st = 0;
	int sn = 0;
	QString errMsg;
	qdatovka_error res = Q_SUCCESS;
	unsigned long dmLimit = MESSAGE_LIST_LIMIT;

	/* != -1 -- specific message required. */
	if (0 <= job.mId.dmId) {

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading message" << job.mId.dmId << "for account"
		    << AccountModel::globAccounts[job.userName].accountName();
		qDebug() << "-----------------------------------------------";

		if (Q_SUCCESS == Task::downloadMessage(job.userName, job.mId,
		        true, job.msgDirect, *job.dbSet, errMsg,
		        "DownloadMessage")) {
			/* Only on successful download. */
			emit globMsgProcEmitter.downloadSuccess(job.userName, job.mId.dmId);
		} else {
			emit globMsgProcEmitter.downloadFail(job.userName,
			    job.mId.dmId, errMsg);
		}

	} else if (MSG_RECEIVED == job.msgDirect) {

		Q_ASSERT(0);

	} else if (MSG_SENT == job.msgDirect) {

		Q_ASSERT(0);

	}

	emit globMsgProcEmitter.progressChange("Idle", 0);

	qDebug() << "-----------------------------------------------";

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	downloadMessagesMutex.unlock();

	emit finished();
}
