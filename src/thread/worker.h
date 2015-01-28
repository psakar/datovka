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

	static
	QMutex downloadMessagesMutex;

	/*!
	 * @brief Constructor.
	 */
	explicit Worker(QModelIndex acntTopIdx, QString dmId,
	    AccountDb &accountDb, AccountModel &accountModel,
	    int count, QList<MessageDb*> messageDbList,
	    QList<bool> downloadThisAccounts, QObject *parent);

	/*!
	 * @brief Requests the process to start
	 */
	void requestWork(void);

	/*!
	 * @brief Store message into database.
	 */
	static
	qdatovka_error storeMessage(bool signedMsg, bool incoming,
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
	qdatovka_error updateMessageState(bool signedMsg,
	    MessageDb &messageDb, const struct isds_message *msg);

	/*!
	 * @brief Download attachments, envelope and raw for message.
	 */
	static
	qdatovka_error downloadMessage(const QModelIndex &acntTopIdx,
	    const QString &dmId, bool signedMsg, bool incoming,
	    MessageDb &messageDb,
	    const QString &progressLabel, QProgressBar *pBar, Worker *worker);

	/*!
	 * @brief Store envelope into database.
	 */
	static
	qdatovka_error storeEnvelope(const QString &messageType,
	    MessageDb &messageDb, const struct isds_envelope *envel);

	/*!
	 * @brief Download sent/received message list from ISDS for current
	 *     account index.
	 */
	static
	qdatovka_error downloadMessageList(const QModelIndex &acntTopIdx,
	    const QString &messageType, MessageDb &messageDb,
	    const QString &progressLabel, QProgressBar *pBar, Worker *worker,
	    int &total, int &news);

private:

	QModelIndex m_acntTopIdx;
	QString m_dmId;
	AccountDb &m_accountDb;
	AccountModel &m_accountModel;
	int m_count;
	QList<MessageDb*> m_messageDbList;
	QList<bool> m_downloadThisAccounts;

	/*!
	* @brief Get password expiration info for account index
	*/
	bool getPasswordInfo(const QModelIndex &acntTopIdx);

	/*!
	* @brief Download sent message delivery info and get list of events
	* message
	*/
	static
	bool getMessageState(const QModelIndex &acntTopIdx,
	    int msgIdx, bool signedMsg, MessageDb &messageDb);

	/*!
	 * @brief Download delivery info for message.
	 */
	static
	bool getDeliveryInfo(const QModelIndex &acntTopIdx,
	    const QString &dmId, bool signedMsg, MessageDb &messageDb);

	/*!
	 * @brief Get additional info about author (sender)
	 */
	static
	bool getMessageAuthor(const QModelIndex &acntTopIdx,
	    const QString &dmId, MessageDb &messageDb);

	/*!
	 * @brief Set message as downloaded from ISDS.
	 */
	static
	bool markMessageAsDownloaded(const QModelIndex &acntTopIdx,
	    const QString &dmId);

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
	void refreshAttachmentList(const QModelIndex, QString);

	/*!
	 * @brief This signal is emitted when accout is proccessed
	 */
	void changeStatusBarInfo(bool, QString, int, int, int, int);

	/*!
	 * @brief This signal is emitted when download of message fails =
	 * clear info in status bar.
	 */
	void clearStatusBarAndShowDialog(QString);

	/*!
	 * @brief This signal is emitted when process is finished
	 */
	void finished(void);

public slots:
	/*!
	 * @brief Run Message downloading in thread
	 */
	void syncAllAccounts(void);
	void syncOneAccount(void);
	void downloadCompleteMessage(void);
};

#endif /* _WORKER_H_ */
