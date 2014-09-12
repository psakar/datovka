#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QMutex>

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"


class Worker : public QObject {
    Q_OBJECT

public:
	explicit Worker(AccountDb &accountDb, AccountModel &accountModel,
	   int count, QList<MessageDb*> messageDbList, QObject *parent = 0);

	/*!
	* @brief Requests the process to start
	*/
	void requestWork(void);

	/*!
	* @brief Requests the process to abort
	*/
	void abort(void);

private:
	bool _abort;
	bool _working;

	QMutex mutex;
	AccountDb &m_accountDb;
	AccountModel &m_accountModel;
	int m_count;
	QList<MessageDb*> m_messageDbList;

	/*!
	* @brief Download sent/received message list from ISDS for current
	* account index
	*/
	qdatovka_error downloadMessageList(const QModelIndex &acntTopIdx,
	    const QString messageType, MessageDb &messageDb, QString label);

	/*!
	* @brief Get list of sent message state changes
	*/
	bool getListSentMessageStateChanges(const QModelIndex &acntTopIdx,
	    MessageDb &messageDb, QString label);

	/*!
	* @brief Get password expiration info for account index
	*/
	bool getPasswordInfo(const QModelIndex &acntTopIdx);

	/*!
	* @brief Download sent message delivery info and get list of events
	* message
	*/
	bool getSentDeliveryInfo(const QModelIndex &acntTopIdx,
	    int msgIdx, bool signedMsg, MessageDb &messageDb);

	/*!
	* @brief Download attachments, envelope and raw for message.
	*/
	qdatovka_error downloadMessage(const QModelIndex &acntTopIdx,
	    const QString dmId, bool signedMsg, MessageDb &messageDb,
	    const QString messageType);

	/*!
	* @brief Download delivery info for message.
	*/
	bool getReceivedsDeliveryInfo(const QModelIndex &acntTopIdx,
	    const QString dmId, bool signedMsg, MessageDb &messageDb);

	/*!
	* @brief Get additional info about author (sender)
	*/
	bool getMessageAuthor(const QModelIndex &acntTopIdx,
	    const QString dmId, MessageDb &messageDb);

	/*!
	* @brief Set message as downloaded from ISDS.
	*/
	bool markMessageAsDownloaded(const QModelIndex &acntTopIdx,
	    const QString dmId);

signals:
	/*!
	 * @brief This signal is emitted when the Worker request to Work
	*/
	void workRequested(void);

	/*!
	* @brief This signal is emitted when counted value is changed
	*/
	void valueChanged(QString label, int value);

	void refreshAccountList(const QModelIndex);
	/*!
	* @brief This signal is emitted when process is finished
	*/
	void finished(void);

public slots:
	/*!
	* @brief Run Message downloading in thread
	*/
	void doWork(void);
};

#endif // WORKER_H
