

#ifndef _WORKER_H_
#define _WORKER_H_


#include <QMutex>
#include <QObject>
#include <QProgressBar> /* Progress. */

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"


class Worker : public QObject {
    Q_OBJECT

public:
	/*!
	 * @brief Construtor.
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
	 * @brief Download attachments, envelope and raw for message.
	 */
	static
	qdatovka_error downloadMessage(const QModelIndex &acntTopIdx,
	    const QString dmId, bool signedMsg, bool incoming,
	    MessageDb &messageDb);

	/*!
	 * @brief Download sent/received message list from ISDS for current
	 *     account index.
	 */
	static
	qdatovka_error downloadMessageList(const QModelIndex &acntTopIdx,
	    const QString messageType, MessageDb &messageDb, QString label,
	    QProgressBar *pBar, Worker *worker);

private:

	QModelIndex m_acntTopIdx;
	QString m_dmId;
	AccountDb &m_accountDb;
	AccountModel &m_accountModel;
	int m_count;
	QList<MessageDb*> m_messageDbList;
	QList<bool> m_downloadThisAccounts;

	static
	QMutex downloadMessagesMutex;

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
	static
	bool getSentDeliveryInfo(const QModelIndex &acntTopIdx,
	    int msgIdx, bool signedMsg, MessageDb &messageDb);

	/*!
	 * @brief Download delivery info for message.
	 */
	static
	bool getReceivedsDeliveryInfo(const QModelIndex &acntTopIdx,
	    const QString dmId, bool signedMsg, MessageDb &messageDb);

	/*!
	 * @brief Get additional info about author (sender)
	 */
	static
	bool getMessageAuthor(const QModelIndex &acntTopIdx,
	    const QString dmId, MessageDb &messageDb);

	/*!
	 * @brief Set message as downloaded from ISDS.
	 */
	static
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
	void syncAllAccounts(void);
	void syncOneAccount(void);
	void downloadCompleteMessage(void);
};

#endif /* _WORKER_H_ */
