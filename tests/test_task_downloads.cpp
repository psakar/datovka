/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QtTest/QtTest>

#include "src/io/account_db.h"
#include "src/io/isds_sessions.h"
#include "src/settings/preferences.h"
#include "src/worker/task_download_message_list.h"
#include "tests/helper_qt.h"
#include "tests/test_task_downloads.h"

class TestTaskDownloads : public QObject {
	Q_OBJECT

public:
	TestTaskDownloads(void);

	~TestTaskDownloads(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void dowloadMessageList(void);

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

	QString m_confSubDirBackup; /*!< Backup for the configuration directory. */
};

TestTaskDownloads::TestTaskDownloads(void)
    : m_testing(true),
    m_organisation(MessageDbSet::DO_YEARLY),
    m_connectionPrefix(QLatin1String("GLOBALDBS")),
    m_testPath(QDir::currentPath() + QDir::separator() + QLatin1String("_test_dir")),
    m_testDir(m_testPath),
    m_credFName(QLatin1String(CREDENTIALS_FNAME)),
    m_sender(),
    m_recipient(),
    m_recipientDbSet(NULL),
    m_confSubDirBackup(globPref.confSubdir)
{
	/* Set configuration subdirectory to some value. */
	globPref.confSubdir = QLatin1String(".datovka_test");
}

TestTaskDownloads::~TestTaskDownloads(void)
{
	/* Just in case. */
	delete m_recipientDbSet; m_recipientDbSet = NULL;

	/* Restore original value. */
	globPref.confSubdir = m_confSubDirBackup;
}

void TestTaskDownloads::initTestCase(void)
{
	bool ret;

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
	if (m_recipientDbSet == NULL) {
		QSKIP("Failed to open message database.");
	}
	QVERIFY(m_recipientDbSet != NULL);

	/*
	 * Create accounts database and open it. It is required by the task.
	 */
	QVERIFY(globAccountDbPtr == NULL);
	globAccountDbPtr = new (::std::nothrow) AccountDb("accountDb");
	if (globAccountDbPtr == NULL) {
		QSKIP("Cannot create accounts database.");
	}
	QVERIFY(globAccountDbPtr != NULL);
	ret = globAccountDbPtr->openDb(m_testPath + QDir::separator() + "messages.shelf.db");
	if (!ret) {
		QSKIP("Cannot open account database.");
	}
	QVERIFY(ret);

	/* Log into ISDS. */
	struct isds_ctx *ctx = isdsSessions.session(m_recipient.userName);
	if (!isdsSessions.holdsSession(m_recipient.userName)) {
		QVERIFY(ctx == NULL);
		ctx = isdsSessions.createCleanSession(m_recipient.userName,
		    globPref.isds_download_timeout_ms);
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
}

void TestTaskDownloads::cleanupTestCase(void)
{
	delete m_recipientDbSet; m_recipientDbSet = NULL;

	/* Delete account database. */
	delete globAccountDbPtr; globAccountDbPtr = NULL;

	/* The configuration directory should be non-existent. */
	QVERIFY(!QDir(globPref.confDir()).exists());

	/* Delete tesing directory. */
//	m_testDir.removeRecursively();
//	QVERIFY(!m_testDir.exists());
}

void TestTaskDownloads::dowloadMessageList(void)
{
	TaskDownloadMessageList *task;

	QVERIFY(!m_recipient.userName.isEmpty());

	QVERIFY(m_recipientDbSet != NULL);

	QVERIFY(isdsSessions.isConnectedToIsds(m_recipient.userName));
	struct isds_ctx *ctx = isdsSessions.session(m_recipient.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(isdsSessions.isConnectedToIsds(m_recipient.userName));

	task = new (::std::nothrow) TaskDownloadMessageList(
	    m_recipient.userName, m_recipientDbSet, MSG_RECEIVED, false,
	    MESSAGE_LIST_LIMIT, MESSAGESTATE_ANY);

	QVERIFY(task != NULL);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskDownloadMessageList::DL_SUCCESS);

	delete task; task = NULL;
}

QObject *newTestTaskDownloads(void)
{
	return new (::std::nothrow) TestTaskDownloads();
}

//QTEST_MAIN(TestTaskDownloads)
#include "test_task_downloads.moc"
