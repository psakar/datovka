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

#include "src/datovka_shared/isds/message_interface.h"
#include "src/datovka_shared/isds/types.h"
#include "src/global.h"
#include "src/io/account_db.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_send_message.h"
#include "tests/helper_qt.h"
#include "tests/test_task_send_message.h"

class TestTaskSendMessage : public QObject {
	Q_OBJECT

public:
	explicit TestTaskSendMessage(qint64 &sentMsgId);

	~TestTaskSendMessage(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void sendMessage(void);

private:
	static
	Isds::Message buildMessage(const QString &recipBox);

	const bool m_testing; /*!< Testing account. */
	const enum MessageDbSet::Organisation m_organisation; /*!< Database organisation. */

	const QString m_connectionPrefix; /*!< SQL connection prefix. */

	const QString m_testPath; /*!< Test path. */
	QDir m_testDir;  /*!< Directory containing testing data. */

	const QString m_credFName; /*!< Credentials file name. */

	/* Log-in using user name and password. */
	LoginCredentials m_sender; /*!< Sender credentials. */
	LoginCredentials m_recipient; /*!< Recipient credentials. */

	MessageDbSet *m_senderDbSet; /*!< Databases. */

	qint64 &m_sentMsgId; /*!< Identifier ow newly sent message. */
};

#define printCredentials(cred) \
	fprintf(stderr, "Credentials '" #cred "': '%s', '%s', '%s'\n", \
	    (cred).boxName.toUtf8().constData(), \
	    (cred).userName.toUtf8().constData(), \
	    (cred).pwd.toUtf8().constData())

TestTaskSendMessage::TestTaskSendMessage(qint64 &sentMsgId)
    : m_testing(true),
    m_organisation(MessageDbSet::DO_YEARLY),
    m_connectionPrefix(QLatin1String("GLOBALDBS")),
    m_testPath(QDir::currentPath() + QDir::separator() + QLatin1String("_test_dir")),
    m_testDir(m_testPath),
    m_credFName(QLatin1String(CREDENTIALS_FNAME)),
    m_sender(),
    m_recipient(),
    m_senderDbSet(Q_NULLPTR),
    m_sentMsgId(sentMsgId)
{
}

TestTaskSendMessage::~TestTaskSendMessage(void)
{
	/* Just in case. */
	delete m_senderDbSet; m_senderDbSet = Q_NULLPTR;
}

void TestTaskSendMessage::initTestCase(void)
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
	m_senderDbSet = MessageDbSet::createNew(m_testPath, m_sender.userName,
	    m_testing, m_organisation, m_connectionPrefix,
	    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
	if (m_senderDbSet == Q_NULLPTR) {
		QSKIP("Failed to open message database.");
	}
	QVERIFY(m_senderDbSet != Q_NULLPTR);

	/*
	 * Create accounts database and open it. It is required by the task.
	 */
	QVERIFY(GlobInstcs::accntDbPtr == Q_NULLPTR);
	GlobInstcs::accntDbPtr = new (::std::nothrow) AccountDb("accountDb",
	    false);
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
	struct isds_ctx *ctx =
	    GlobInstcs::isdsSessionsPtr->session(m_sender.userName);
	if (!GlobInstcs::isdsSessionsPtr->holdsSession(m_sender.userName)) {
		QVERIFY(ctx == NULL);
		ctx = GlobInstcs::isdsSessionsPtr->createCleanSession(
		    m_sender.userName,
		    GlobInstcs::prefsPtr->isdsDownloadTimeoutMs);
	}
	if (ctx == NULL) {
		QSKIP("Cannot obtain communication context.");
	}
	QVERIFY(ctx != NULL);
	isds_error err = isdsLoginUserName(ctx, m_sender.userName,
	    m_sender.pwd, m_testing);
	if (err != IE_SUCCESS) {
		QSKIP("Error connection into ISDS.");
	}
	QVERIFY(err == IE_SUCCESS);

	QVERIFY(GlobInstcs::acntMapPtr == Q_NULLPTR);
	GlobInstcs::acntMapPtr = new (std::nothrow) AccountsMap;
	QVERIFY(GlobInstcs::acntMapPtr != Q_NULLPTR);
}

void TestTaskSendMessage::cleanupTestCase(void)
{
	delete m_senderDbSet; m_senderDbSet = Q_NULLPTR;

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

void TestTaskSendMessage::sendMessage(void)
{
	TaskSendMessage *task;

	QVERIFY(!m_sender.userName.isEmpty());

	QVERIFY(m_senderDbSet != Q_NULLPTR);

	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_sender.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));

	QString transactionId(QLatin1String("some_id"));
	QString recipientName(QLatin1String("recipient name"));
	QString recipientAddress(QLatin1String("recipient address"));
	bool isPDZ = false;

	/* Sending empty message must fail. */
	task = new (::std::nothrow) TaskSendMessage(m_sender.userName,
	    m_senderDbSet, transactionId, Isds::Message(), recipientName,
	    recipientAddress, isPDZ);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_resultData.result == TaskSendMessage::SM_ERR);

	delete task; task = Q_NULLPTR;

	/* Sending message should succeed. */
	task = new (::std::nothrow) TaskSendMessage(m_sender.userName,
	    m_senderDbSet, transactionId, buildMessage(m_recipient.boxName),
	    recipientName, recipientAddress, isPDZ);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_resultData.result == TaskSendMessage::SM_SUCCESS);

	QVERIFY(task->m_resultData.dmId != 0);
	m_sentMsgId = task->m_resultData.dmId;

	delete task; task = Q_NULLPTR;
}

Isds::Message TestTaskSendMessage::buildMessage(const QString &recipBox)
{
	Isds::Envelope env;
	QList<Isds::Document> docs;
	Isds::Message msg;

	QString dateTime(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

	env.setDbIDRecipient(recipBox);
	env.setDmAnnotation(QLatin1String("sending-test-") + dateTime);

	{
		Isds::Document doc;
		doc.setFileDescr(QLatin1String("priloha.txt"));
		doc.setFileMetaType(Isds::Type::FMT_MAIN);
		doc.setMimeType(QStringLiteral(""));
		doc.setBinaryContent(QString("Priloha vygenerovana %1.").arg(dateTime).toUtf8());
		docs.append(doc);
	}

	msg.setEnvelope(env);
	msg.setDocuments(docs);

	return msg;
}

QObject *newTestTaskSendMessage(qint64 &sentMsgId)
{
	return new (::std::nothrow) TestTaskSendMessage(sentMsgId);
}

//QTEST_MAIN(TestTaskSendMessage)
#include "test_task_send_message.moc"
