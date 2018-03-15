/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include <QDateTime>
#include <QDir>
#include <QtTest/QtTest>

#include "src/global.h"
#include "src/io/account_db.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_message.h"
#include "src/worker/task_download_message_list.h"
#include "tests/helper_qt.h"
#include "tests/test_task_downloads.h"

class TestTaskDownloads : public QObject {
	Q_OBJECT

public:
	explicit TestTaskDownloads(const qint64 &receivedMsgId);

	~TestTaskDownloads(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void downloadMessageList(void);

	void getDeliveryTime(void);

	void downloadMessage(void);

private:
	const bool m_testing; /*!< Testing account. */
	const enum MessageDbSet::Organisation m_organisation; /*!< Database organisation. */

	const QString m_connectionPrefix; /*!< SQL connection prefix. */

	const QString m_testPath; /*!< Test path. */
	QDir m_testDir;  /*!< Directory containing testing data. */

	const QString m_credFName; /*!< Credentials file name. */

	/* Log-in using user name and password. */
	LoginCredentials m_sender; /*!< Sender credentials. */
	LoginCredentials m_recipient; /*!< Recipient credentials. */

	MessageDbSet *m_recipientDbSet; /*!< Databases. */

	const qint64 &m_receivedMsgId; /*!< Identifier ow newly received message. */
	QDateTime m_deliveryTime; /*!< Message delivery time. */
};

TestTaskDownloads::TestTaskDownloads(const qint64 &receivedMsgId)
    : m_testing(true),
    m_organisation(MessageDbSet::DO_YEARLY),
    m_connectionPrefix(QLatin1String("GLOBALDBS")),
    m_testPath(QDir::currentPath() + QDir::separator() + QLatin1String("_test_dir")),
    m_testDir(m_testPath),
    m_credFName(QLatin1String(CREDENTIALS_FNAME)),
    m_sender(),
    m_recipient(),
    m_recipientDbSet(Q_NULLPTR),
    m_receivedMsgId(receivedMsgId),
    m_deliveryTime()
{
}

TestTaskDownloads::~TestTaskDownloads(void)
{
	/* Just in case. */
	delete m_recipientDbSet; m_recipientDbSet = Q_NULLPTR;
}

void TestTaskDownloads::initTestCase(void)
{
	bool ret;

	QVERIFY(GlobInstcs::logPtr == Q_NULLPTR);
	GlobInstcs::logPtr = new (std::nothrow) LogDevice;
	QVERIFY(GlobInstcs::logPtr != Q_NULLPTR);

	QVERIFY(GlobInstcs::msgProcEmitterPtr == Q_NULLPTR);
	GlobInstcs::msgProcEmitterPtr =
	    new (std::nothrow) MessageProcessingEmitter;
	QVERIFY(GlobInstcs::msgProcEmitterPtr != Q_NULLPTR);

	QVERIFY(GlobInstcs::prefsPtr == Q_NULLPTR);
	GlobInstcs::prefsPtr = new (std::nothrow) Preferences;
	QVERIFY(GlobInstcs::prefsPtr != Q_NULLPTR);

	/* Set configuration subdirectory to some value. */
	GlobInstcs::prefsPtr->confSubdir = QLatin1String(".datovka_test");

	/* Create empty working directory. */
	m_testDir.removeRecursively();
	QVERIFY(!m_testDir.exists());
	m_testDir.mkpath(".");
	QVERIFY(m_testDir.exists());

	/* Load credentials. */
	ret = m_sender.loadLoginCredentials(m_credFName, 1);
	if (!ret) {
		QSKIP("Failed to load login credentials. Skipping remaining tests.");
	}
	QVERIFY(ret);
	QVERIFY(!m_sender.userName.isEmpty());
	QVERIFY(!m_sender.pwd.isEmpty());

	ret = m_recipient.loadLoginCredentials(m_credFName, 2);
	if (!ret) {
		QSKIP("Failed to load login credentials. Skipping remaining tests.");
	}
	QVERIFY(ret);
	QVERIFY(!m_recipient.userName.isEmpty());
	QVERIFY(!m_recipient.pwd.isEmpty());

	/* Access message database. */
	m_recipientDbSet = MessageDbSet::createNew(m_testPath, m_recipient.userName,
	    m_testing, m_organisation, m_connectionPrefix,
	    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
	if (m_recipientDbSet == Q_NULLPTR) {
		QSKIP("Failed to open message database.");
	}
	QVERIFY(m_recipientDbSet != Q_NULLPTR);

	/*
	 * Create accounts database and open it. It is required by the task.
	 */
	QVERIFY(GlobInstcs::accntDbPtr == Q_NULLPTR);
	GlobInstcs::accntDbPtr = new (::std::nothrow) AccountDb("accountDb", false);
	if (GlobInstcs::accntDbPtr == Q_NULLPTR) {
		QSKIP("Cannot create accounts database.");
	}
	QVERIFY(GlobInstcs::accntDbPtr != Q_NULLPTR);
	ret = GlobInstcs::accntDbPtr->openDb(
	    m_testPath + QDir::separator() + "messages.shelf.db",
	    SQLiteDb::CREATE_MISSING);
	if (!ret) {
		QSKIP("Cannot open account database.");
	}
	QVERIFY(ret);

	/* Create ISDS session container. */
	QVERIFY(GlobInstcs::isdsSessionsPtr == Q_NULLPTR);
	GlobInstcs::isdsSessionsPtr = new (std::nothrow) IsdsSessions;
	if (GlobInstcs::isdsSessionsPtr == Q_NULLPTR) {
		QSKIP("Cannot create session container.");
	}
	QVERIFY(GlobInstcs::isdsSessionsPtr != Q_NULLPTR);

	/* Log into ISDS. */
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_recipient.userName);
	if (!GlobInstcs::isdsSessionsPtr->holdsSession(m_recipient.userName)) {
		QVERIFY(ctx == NULL);
		ctx = GlobInstcs::isdsSessionsPtr->createCleanSession(
		    m_recipient.userName,
		    GlobInstcs::prefsPtr->isdsDownloadTimeoutMs);
	}
	if (ctx == NULL) {
		QSKIP("Cannot obtain communication context.");
	}
	QVERIFY(ctx != NULL);
	isds_error err = isdsLoginUserName(ctx, m_recipient.userName,
	    m_recipient.pwd, m_testing);
	if (err != IE_SUCCESS) {
		QSKIP("Error connection into ISDS.");
	}
	QVERIFY(err == IE_SUCCESS);

	QVERIFY(GlobInstcs::acntMapPtr == Q_NULLPTR);
	GlobInstcs::acntMapPtr = new (std::nothrow) AccountsMap;
	QVERIFY(GlobInstcs::acntMapPtr != Q_NULLPTR);
}

void TestTaskDownloads::cleanupTestCase(void)
{
	delete m_recipientDbSet; m_recipientDbSet = Q_NULLPTR;

	delete GlobInstcs::acntMapPtr; GlobInstcs::acntMapPtr = Q_NULLPTR;

	/* Destroy ISDS session container. */
	delete GlobInstcs::isdsSessionsPtr; GlobInstcs::isdsSessionsPtr = Q_NULLPTR;

	/* Delete account database. */
	delete GlobInstcs::accntDbPtr; GlobInstcs::accntDbPtr = Q_NULLPTR;

	/* The configuration directory should be non-existent. */
	QVERIFY(!QDir(GlobInstcs::prefsPtr->confDir()).exists());

	/* Delete testing directory. */
	m_testDir.removeRecursively();
	QVERIFY(!m_testDir.exists());

	delete GlobInstcs::prefsPtr; GlobInstcs::prefsPtr = Q_NULLPTR;

	delete GlobInstcs::msgProcEmitterPtr; GlobInstcs::msgProcEmitterPtr = Q_NULLPTR;

	delete GlobInstcs::logPtr; GlobInstcs::logPtr = Q_NULLPTR;
}

void TestTaskDownloads::downloadMessageList(void)
{
	TaskDownloadMessageList *task;

	QVERIFY(!m_recipient.userName.isEmpty());

	QVERIFY(m_recipientDbSet != Q_NULLPTR);

	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_recipient.userName));
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_recipient.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_recipient.userName));

	task = new (::std::nothrow) TaskDownloadMessageList(
	    m_recipient.userName, m_recipientDbSet, MSG_RECEIVED, false,
	    MESSAGE_LIST_LIMIT, MESSAGESTATE_ANY);

	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskDownloadMessageList::DL_SUCCESS);

	delete task; task = Q_NULLPTR;
}

void TestTaskDownloads::getDeliveryTime(void)
{
	if (m_receivedMsgId == 0) {
		QSKIP("No specific message to download.");
	}

	QVERIFY(!m_recipient.userName.isEmpty());

	QVERIFY(m_recipientDbSet != Q_NULLPTR);

	MessageDb::MsgId msgId(m_recipientDbSet->msgsMsgId(m_receivedMsgId));
	QVERIFY(msgId.isValid());

	m_deliveryTime = msgId.deliveryTime;
	QVERIFY(m_deliveryTime.isValid());
}

void TestTaskDownloads::downloadMessage(void)
{
	TaskDownloadMessage *task;

	if (m_receivedMsgId == 0 || !m_deliveryTime.isValid()) {
		QSKIP("No specific message to download or delivery time invalid.");
	}
	QVERIFY(m_receivedMsgId != 0);

	QVERIFY(!m_recipient.userName.isEmpty());

	QVERIFY(m_recipientDbSet != Q_NULLPTR);

	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_recipient.userName));
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_recipient.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_recipient.userName));

	/* Should fail, is a received message. */
	task = new (::std::nothrow) TaskDownloadMessage(m_recipient.userName,
	    m_recipientDbSet, MSG_SENT,
	    MessageDb::MsgId(m_receivedMsgId, m_deliveryTime), false);

	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskDownloadMessage::DM_ISDS_ERROR);

	delete task; task = Q_NULLPTR;

	/* Must succeed. */
	task = new (::std::nothrow) TaskDownloadMessage(m_recipient.userName,
	    m_recipientDbSet, MSG_RECEIVED,
	    MessageDb::MsgId(m_receivedMsgId, m_deliveryTime), false);

	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskDownloadMessage::DM_SUCCESS);

	delete task; task = Q_NULLPTR;
}

QObject *newTestTaskDownloads(const qint64 &receivedMsgId)
{
	return new (::std::nothrow) TestTaskDownloads(receivedMsgId);
}

//QTEST_MAIN(TestTaskDownloads)
#include "test_task_downloads.moc"
