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

#include <QDir>
#include <QString>
#include <QtTest/QtTest>

#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/isds/message_interface.h"
#include "src/datovka_shared/isds/types.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/isds/message_functions.h"
#include "src/isds/services_login.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_authenticate_message.h"
#include "src/worker/task_verify_message.h"
#include "tests/helper_qt.h"
#include "tests/test_task_verify_message.h"

class TestTaskVerifyMessage : public QObject {
	Q_OBJECT

public:
	TestTaskVerifyMessage(void);

	~TestTaskVerifyMessage(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void verifyMessage(void);

	void authenticateMessage(void);

private:
	void loadCredentials(LoginCredentials &cred, int line);

	const bool m_testing; /*!< Testing account. */

	const QString m_testPath; /*!< Test path. */
	QDir m_testDir;  /*!< Directory containing testing data. */

	const QString m_credFName; /*!< Credentials file name. */

	/* Log-in using user name and password. */
	LoginCredentials m_third; /*!< Neither sender nor recipient. */
	LoginCredentials m_sender; /*!< Sender credentials. */

	const QString msgPath;
	const QString delInfoPath;
};

TestTaskVerifyMessage::TestTaskVerifyMessage(void)
    : m_testing(true),
    m_testPath(QDir::currentPath() + QDir::separator() + QLatin1String("_test_dir")),
    m_testDir(m_testPath),
    m_credFName(QLatin1String(CREDENTIALS_FNAME)),
    m_third(),
    m_sender(),
    msgPath("data/DZ_6452235.zfo"),
    delInfoPath("data/DD_6452235.zfo")
{
}

TestTaskVerifyMessage::~TestTaskVerifyMessage(void)
{
}

void TestTaskVerifyMessage::initTestCase(void)
{
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

	/* Create ISDS session container. */
	QVERIFY(GlobInstcs::isdsSessionsPtr == Q_NULLPTR);
	GlobInstcs::isdsSessionsPtr = new (std::nothrow) IsdsSessions;
	if (GlobInstcs::isdsSessionsPtr == Q_NULLPTR) {
		QSKIP("Cannot create session container.");
	}
	QVERIFY(GlobInstcs::isdsSessionsPtr != Q_NULLPTR);

	loadCredentials(m_third, 1);
	loadCredentials(m_sender, 2);
}

void TestTaskVerifyMessage::cleanupTestCase(void)
{
	/* Destroy ISDS session container. */
	delete GlobInstcs::isdsSessionsPtr; GlobInstcs::isdsSessionsPtr = Q_NULLPTR;

	/* The configuration directory should be non-existent. */
	QVERIFY(!QDir(GlobInstcs::prefsPtr->confDir()).exists());

	/* Delete testing directory. */
	m_testDir.removeRecursively();
	QVERIFY(!m_testDir.exists());

	delete GlobInstcs::prefsPtr; GlobInstcs::prefsPtr = Q_NULLPTR;

	delete GlobInstcs::msgProcEmitterPtr; GlobInstcs::msgProcEmitterPtr = Q_NULLPTR;

	delete GlobInstcs::logPtr; GlobInstcs::logPtr = Q_NULLPTR;
}

void TestTaskVerifyMessage::verifyMessage(void)
{
	Isds::Message message, delInfo;
	QVERIFY(message.isNull());
	QVERIFY(delInfo.isNull());

	message = Isds::messageFromFile(msgPath, Isds::LT_MESSAGE);
	QVERIFY(!message.isNull());
	QVERIFY(!message.envelope().isNull());
	QVERIFY(!message.envelope().dmHash().isNull());

	delInfo = Isds::messageFromFile(delInfoPath, Isds::LT_DELIVERY);
	QVERIFY(!delInfo.isNull());
	QVERIFY(!delInfo.envelope().isNull());
	QVERIFY(!delInfo.envelope().dmHash().isNull());

	QVERIFY(message.envelope().dmId() == 6452235LL);
	QVERIFY(message.envelope().dmId() == delInfo.envelope().dmId());

	qint64 dmId = message.envelope().dmId();

	Isds::Hash messageHash(message.envelope().dmHash());
	Isds::Hash delInfoHash(delInfo.envelope().dmHash());
	QVERIFY(messageHash == message.envelope().dmHash());
	QVERIFY(delInfoHash == delInfo.envelope().dmHash());
	QVERIFY(messageHash == delInfoHash);

	TaskVerifyMessage *task = Q_NULLPTR;

	/* Test with account that is nor sender, neither recipient. */
	task = new (std::nothrow) TaskVerifyMessage(m_third.userName,
	    dmId, messageHash);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskVerifyMessage::VERIFY_ISDS_ERR);

	delete task; task = Q_NULLPTR;

	/* Test with actual sender. */
	task = new (std::nothrow) TaskVerifyMessage(m_sender.userName,
	    dmId, messageHash);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskVerifyMessage::VERIFY_SUCCESS);

	delete task; task = Q_NULLPTR;

	{
		QByteArray value(messageHash.value());
		value[0] = ~value[0];
		messageHash.setValue(value);
	}

	/* Test with actual sender but invalid hash. */
	task = new (std::nothrow) TaskVerifyMessage(m_sender.userName,
	    dmId, messageHash);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskVerifyMessage::VERIFY_NOT_EQUAL);

	delete task; task = Q_NULLPTR;
}

void TestTaskVerifyMessage::authenticateMessage(void)
{
	Isds::Message message, delInfo;
	QVERIFY(message.isNull());
	QVERIFY(delInfo.isNull());

	message = Isds::messageFromFile(msgPath, Isds::LT_MESSAGE);
	QVERIFY(!message.isNull());
	QVERIFY(!message.envelope().isNull());
	QVERIFY(!message.envelope().dmHash().isNull());

	delInfo = Isds::messageFromFile(delInfoPath, Isds::LT_DELIVERY);
	QVERIFY(!delInfo.isNull());
	QVERIFY(!delInfo.envelope().isNull());
	QVERIFY(!delInfo.envelope().dmHash().isNull());

	QVERIFY(!message.raw().isEmpty());
	QVERIFY(!delInfo.raw().isEmpty());

	TaskAuthenticateMessage *task = Q_NULLPTR;

	task = new (std::nothrow) TaskAuthenticateMessage(m_third.userName,
	    message.raw());
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskAuthenticateMessage::AUTH_SUCCESS);

	delete task; task = Q_NULLPTR;

	task = new (std::nothrow) TaskAuthenticateMessage(m_third.userName,
	    delInfo.raw());
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskAuthenticateMessage::AUTH_SUCCESS);

	delete task; task = Q_NULLPTR;

	task = new (std::nothrow) TaskAuthenticateMessage(m_sender.userName,
	    message.raw());
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskAuthenticateMessage::AUTH_SUCCESS);

	delete task; task = Q_NULLPTR;

	task = new (std::nothrow) TaskAuthenticateMessage(m_sender.userName,
	    delInfo.raw());
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskAuthenticateMessage::AUTH_SUCCESS);

	delete task; task = Q_NULLPTR;

	/* Invalid data. */

	QByteArray badRaw(delInfo.raw());
	badRaw[0] = ~badRaw[0];

	task = new (std::nothrow) TaskAuthenticateMessage(m_sender.userName,
	    badRaw);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskAuthenticateMessage::AUTH_ISDS_ERROR);

	delete task; task = Q_NULLPTR;

	task = new (std::nothrow) TaskAuthenticateMessage(m_sender.userName,
	    QByteArray(1, '\0'));
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskAuthenticateMessage::AUTH_ISDS_ERROR);

	delete task; task = Q_NULLPTR;
}

void TestTaskVerifyMessage::loadCredentials(LoginCredentials &cred, int line)
{
	bool ret;

	/* Load credentials. */
	ret = cred.loadLoginCredentials(m_credFName, line);
	if (!ret) {
		QSKIP("Failed to load login credentials. Skipping remaining tests.");
	}
	QVERIFY(ret);
	QVERIFY(!cred.userName.isEmpty());
	QVERIFY(!cred.pwd.isEmpty());

	/* Log into ISDS. */
	Isds::Session *ctx =
	    GlobInstcs::isdsSessionsPtr->session(cred.userName);
	if (!GlobInstcs::isdsSessionsPtr->holdsSession(cred.userName)) {
		QVERIFY(ctx == Q_NULLPTR);
		ctx = GlobInstcs::isdsSessionsPtr->createCleanSession(
		    cred.userName,
		    GlobInstcs::prefsPtr->isdsDownloadTimeoutMs);
	}
	if (ctx == Q_NULLPTR) {
		QSKIP("Cannot obtain communication context.");
	}
	QVERIFY(ctx != Q_NULLPTR);
	Isds::Error err = Isds::Login::loginUserName(ctx, cred.userName,
	    cred.pwd, m_testing);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		QSKIP("Error connecting into ISDS.");
	}
	QVERIFY(err.code() == Isds::Type::ERR_SUCCESS);
}

QObject *newTestTaskVerifyMessage(void)
{
	return new (::std::nothrow) TestTaskVerifyMessage();
}

//QTEST_MAIN(TestTaskVerifyMessage)
#include "test_task_verify_message.moc"
