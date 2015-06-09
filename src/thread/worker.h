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


#ifndef _WORKER_H_
#define _WORKER_H_


#include <QMutex>
#include <QObject>
#include <QProgressBar> /* Progress. */

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"


class Worker : public QObject {
	Q_OBJECT

public:

	class Job {
	public:
		Job(void)
		    : acntTopIdx(QModelIndex()),
		    msgDb(0),
		    msgDirect(MSG_RECEIVED),
		    msgId(-1)
		{
		}
		Job(const QModelIndex &idx, MessageDb *mDb,
		    enum MessageDirection direc, qint64 dmId = -1)
		    : acntTopIdx(idx),
		    msgDb(mDb),
		    msgDirect(direc),
		    msgId(dmId)
		{
		}

		bool isValid(void) {
			return acntTopIdx.isValid() && (0 != msgDb);
		}

		QModelIndex acntTopIdx;
		MessageDb *msgDb;
		enum MessageDirection msgDirect;
		qint64 msgId; /*!<
		               * If != -1, then only a single message is going
		               * to be downloaded.
		               */
	};

	class JobList : private QList<Job>, private QMutex {
	public:
		JobList(void);
		~JobList(void);

		/*!
		 * @brief Atomic prepend.
		 *
		 * @param[in] value  Worker job.
		 */
		void append(const Job &value);
		/*!
		 * @brief Atomic get first and pop.
		 *
		 * param[in] pop  Whether to also pop the first value.
		 * @return Worker job. Empty list returns invalid worker job.
		 */
		Job firstPop(bool pop);
		/*!
		 * @brief Atomic prepend.
		 *
		 * @param[in] value  Worker job.
		 */
		void prepend(const Job &value);
	};

	static
	JobList jobList;

	static
	QMutex downloadMessagesMutex;

	/*!
	 * @brief Constructor for job list usage.
	 */
	explicit Worker(QObject *parent = 0);

	/*!
	 * @brief Constructor for single job usage.
	 *
	 * @param[in] job        Worker job.
	 *
	 * @note Consider using the job queue rather than a single job as
	 *     the queue is the preferred version.
	 */
	explicit Worker(const Job &job, QObject *parent = 0);

	/*!
	 * @brief Requests the process to start
	 */
	void requestWork(void);

	/*!
	 * @brief Store message into database.
	 */
	static
	qdatovka_error storeMessage(bool signedMsg,
	    enum MessageDirection msgDirect,
	    MessageDb &messageDb, const struct isds_message *msg,
	    const QString &progressLabel, QProgressBar *pBar, Worker *worker);

	/*!
	 * @brief Store received message delivery information into database.
	 */
	static
	qdatovka_error storeDeliveryInfo(bool signedMsg,
	    MessageDb &messageDb, const struct isds_message *msg);

	/*!
	 * @brief Store sent message delivery information into database.
	 */
	static
	qdatovka_error updateMessageState(enum MessageDirection msgDirect,
	    MessageDb &messageDb, const struct isds_envelope *envel);

	/*!
	 * @brief Download attachments, envelope and raw for message.
	 */
	static
	qdatovka_error downloadMessage(const QString &userName,
	    qint64 dmId, bool signedMsg, enum MessageDirection msgDirect,
	    MessageDb &messageDb, QString &errMsg, const QString &progressLabel,
	    QProgressBar *pBar, Worker *worker);

	/*!
	 * @brief Store envelope into database.
	 */
	static
	qdatovka_error storeEnvelope(enum MessageDirection msgDirect,
	    MessageDb &messageDb, const struct isds_envelope *envel);

	/*!
	 * @brief Download sent/received message list from ISDS for current
	 *     account index.
	 */
	static
	qdatovka_error downloadMessageList(const QString &userName,
	    enum MessageDirection msgDirect, MessageDb &messageDb, QString &errMsg,
	    const QString &progressLabel, QProgressBar *pBar, Worker *worker,
	    int &total, int &news);

private:

	const Job m_job; /*!< If invalid, then a job list is used. */

	/*!
	* @brief Download sent message delivery info and get list of events
	* message
	*/
	static
	bool getMessageState(enum MessageDirection msgDirect,
	    const QString &userName, qint64 dmId, bool signedMsg,
	    MessageDb &messageDb);

	/*!
	 * @brief Download delivery info for message.
	 */
	static
	bool getDeliveryInfo(const QString &userName,
	    qint64 dmId, bool signedMsg, MessageDb &messageDb);

	/*!
	 * @brief Get additional info about author (sender)
	 */
	static
	bool getMessageAuthor(const QString &userName,
	    qint64 dmId, MessageDb &messageDb);

	/*!
	 * @brief Set message as downloaded from ISDS.
	 */
	static
	bool markMessageAsDownloaded(const QString &userName, qint64 dmId);

	/*!
	 * @brief Update message envelope.
	 */
	static
	qdatovka_error updateEnvelope(enum MessageDirection msgDirect,
	    MessageDb &messageDb, const struct isds_envelope *envel);

signals:
	/*!
	 * @brief This signal is emitted when the Worker request to Work
	 */
	void workRequested(void);

	/*!
	 * @brief This signal is emitted when counted value is changed
	 */
	void valueChanged(QString label, int value);

	/*!
	 * @brief This signal is emitted when process is finished
	 */
	void refreshAccountList(const QModelIndex);

	/*!
	 * @brief This signal is emitted when message downloading is finished
	 */
	void refreshAttachmentList(const QModelIndex, qint64);

	/*!
	 * @brief This signal is emitted when account is processed.
	 *     It sends the number of processed messages.
	 */
	void changeStatusBarInfo(bool, int, int, int, int);

	/*!
	 * @brief This signal is emitted when download of message fails =
	 * clear info in status bar.
	 */
	void clearStatusBarAndShowDialog(qint64, QString);

	/*!
	 * @brief This signal is emitted when process is finished
	 */
	void finished(void);

public slots:
	/*!
	 * @brief Run message downloading in thread.
	 */
	void doJob(void);
};

#endif /* _WORKER_H_ */
