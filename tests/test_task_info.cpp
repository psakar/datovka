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
#include "src/datovka_shared/isds/types.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/io/account_db.h"
#include "src/io/isds_sessions.h"
#include "src/isds/services_login.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_credit_info.h"
#include "src/worker/task_download_owner_info.h"
#include "src/worker/task_download_password_info.h"
#include "src/worker/task_download_user_info.h"
#include "tests/helper_qt.h"
#include "tests/test_task_info.h"

class TestTaskInfo : public QObject {
	Q_OBJECT

public:
	TestTaskInfo(void);

	~TestTaskInfo(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void downloadCreditInfo(void);

	void downloadPasswordInfo(void);

	void downloadOwnerInfo(void);

	void downloadUserInfo(void);

private:
	void loadCredentials(LoginCredentials &cred, int line);

	const bool m_testing; /*!< Testing account. */

	const QString m_testPath; /*!< Test path. */
	QDir m_testDir;  /*!< Directory containing testing data. */

	const QString m_credFName; /*!< Credentials file name. */

	/* Log-in using user name and password. */
	LoginCredentials m_first;
	LoginCredentials m_second;
};

TestTaskInfo::TestTaskInfo(void)
    : m_testing(true),
    m_testPath(QDir::currentPath() + QDir::separator() + QLatin1String("_test_dir")),
    m_testDir(m_testPath),
    m_credFName(QLatin1String(CREDENTIALS_FNAME)),
    m_first(),
    m_second()
{
}

TestTaskInfo::~TestTaskInfo(void)
{
}

void TestTaskInfo::initTestCase(void)
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

	loadCredentials(m_first, 1);
	loadCredentials(m_second, 2);
}

void TestTaskInfo::cleanupTestCase(void)
{
	/* Destroy ISDS session container. */
	delete GlobInstcs::isdsSessionsPtr; GlobInstcs::isdsSessionsPtr = Q_NULLPTR;

	/* Delete account database. */
	delete GlobInstcs::accntDbPtr; GlobInstcs::accntDbPtr = Q_NULLPTR;

	/* Delete testing directory. */
	m_testDir.removeRecursively();
	QVERIFY(!m_testDir.exists());

	delete GlobInstcs::prefsPtr; GlobInstcs::prefsPtr = Q_NULLPTR;

	delete GlobInstcs::msgProcEmitterPtr; GlobInstcs::msgProcEmitterPtr = Q_NULLPTR;

	delete GlobInstcs::logPtr; GlobInstcs::logPtr = Q_NULLPTR;
}

void TestTaskInfo::downloadCreditInfo(void)
{
	TaskDownloadCreditInfo *task = Q_NULLPTR;

	/* Non-existent box. */
	task = new (std::nothrow) TaskDownloadCreditInfo(m_first.userName, QString("box"));
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_heller == -1);

	delete task; task = Q_NULLPTR;

	/* Box with wrong identifier (insufficient privileges). */
	task = new (std::nothrow) TaskDownloadCreditInfo(m_first.userName, m_second.boxName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_heller == -1);

	delete task; task = Q_NULLPTR;

	/* Box with correct identifier. */
	task = new (std::nothrow) TaskDownloadCreditInfo(m_first.userName, m_first.boxName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_heller > 0);

	delete task; task = Q_NULLPTR;
}

void TestTaskInfo::downloadPasswordInfo(void)
{
	QDateTime dateTime;
	TaskDownloadPasswordInfo *task = Q_NULLPTR;

	/* Password info not present. */
	dateTime = GlobInstcs::accntDbPtr->getPwdExpirFromDb(
	    AccountDb::keyFromLogin(m_first.userName));
	QVERIFY(dateTime.isNull());
	dateTime = GlobInstcs::accntDbPtr->getPwdExpirFromDb(
	    AccountDb::keyFromLogin(m_second.userName));
	QVERIFY(dateTime.isNull());

	/* Download password info for both accounts. */
	task = new (std::nothrow) TaskDownloadPasswordInfo(m_first.userName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_success);

	delete task; task = Q_NULLPTR;

	task = new (std::nothrow) TaskDownloadPasswordInfo(m_second.userName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_success);

	delete task; task = Q_NULLPTR;

	/* Password info present, but no expiration is set. */
	dateTime = GlobInstcs::accntDbPtr->getPwdExpirFromDb(
	    AccountDb::keyFromLogin(m_first.userName));
	QVERIFY(dateTime.isNull());
	dateTime = GlobInstcs::accntDbPtr->getPwdExpirFromDb(
	    AccountDb::keyFromLogin(m_second.userName));
	QVERIFY(dateTime.isNull());
}

void TestTaskInfo::downloadOwnerInfo(void)
{
	Isds::DbOwnerInfo ownerInfo;
	TaskDownloadOwnerInfo *task = Q_NULLPTR;

	/* Owner info not present. */
	ownerInfo = GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(m_first.userName));
	QVERIFY(ownerInfo.dbID().isNull());
	ownerInfo = GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(m_second.userName));
	QVERIFY(ownerInfo.dbID().isNull());

	/* Download owner info for both accounts. */
	task = new (std::nothrow) TaskDownloadOwnerInfo(m_first.userName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_success);

	delete task; task = Q_NULLPTR;

	task = new (std::nothrow) TaskDownloadOwnerInfo(m_second.userName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_success);

	delete task; task = Q_NULLPTR;

	/* Owner info downloaded. */
	ownerInfo = GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(m_first.userName));
	QVERIFY(!ownerInfo.dbID().isEmpty());
	ownerInfo = GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(m_second.userName));
	QVERIFY(!ownerInfo.dbID().isEmpty());
}

void TestTaskInfo::downloadUserInfo(void)
{
	DbEntry entry;
	TaskDownloadUserInfo *task = Q_NULLPTR;

	/* User info not present. */
	entry = GlobInstcs::accntDbPtr->userEntry(
	    AccountDb::keyFromLogin(m_first.userName));
	QVERIFY(!entry.hasValue("userType"));
	entry = GlobInstcs::accntDbPtr->userEntry(
	    AccountDb::keyFromLogin(m_second.userName));
	QVERIFY(!entry.hasValue("userType"));

	/* Download user info for both accounts. */
	task = new (std::nothrow) TaskDownloadUserInfo(m_first.userName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_success);

	delete task; task = Q_NULLPTR;

	task = new (std::nothrow) TaskDownloadUserInfo(m_second.userName);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_success);

	delete task; task = Q_NULLPTR;

	/* User info downloaded. */
	entry = GlobInstcs::accntDbPtr->userEntry(
	    AccountDb::keyFromLogin(m_first.userName));
	QVERIFY(entry.hasValue("userType"));
	entry = GlobInstcs::accntDbPtr->userEntry(
	    AccountDb::keyFromLogin(m_second.userName));
	QVERIFY(entry.hasValue("userType"));
}

void TestTaskInfo::loadCredentials(LoginCredentials &cred, int line)
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

QObject *newTestTaskInfo(void)
{
	return new (::std::nothrow) TestTaskInfo();
}

//QTEST_MAIN(TestTaskInfo)
#include "test_task_info.moc"
