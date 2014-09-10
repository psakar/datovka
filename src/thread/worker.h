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
	explicit Worker(QObject *parent = 0);

	/*!
	* @brief Requests the process to start
	*/
	void requestWorkInThread(void);

	/*!
	* @brief Requests the process to abort
	*/
	void abortWork(void);

private:
	bool _abort;
	bool _working;
	QMutex mutex;

signals:
	/*!
	 * @brief This signal is emitted when the Worker request to Work
	*/
	void workRequestedInThread(void);

	/*!
	* @brief This signal is emitted when counted value is changed (every sec)
	*/
	void valueChanged(const QString &value);

	/*!
	* @brief This signal is emitted when process is finished (either by counting 60 sec or being aborted)
	*/
	void finishedWork(void);

public slots:

	void doWork(void);
};

#endif // WORKER_H
