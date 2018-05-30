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

#include <QtTest/QtTest>

#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_search_owner_fulltext.h"
#include "src/worker/task_search_owner.h"
#include "tests/helper_qt.h"
#include "tests/test_task_search_owner.h"

class TestTaskSearchOwner : public QObject {
	Q_OBJECT

public:
	TestTaskSearchOwner(void);

	~TestTaskSearchOwner(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void searchOwner(void);

	void searchOwnerFulltext01(void);

	void searchOwnerFulltext02(void);

	void searchOwnerFulltext03(void);

private:
	void checkIndexList(const QString &res, const QString &query,
	    const QList< QPair<int, int> > &list);

	const bool m_testing; /*!< Testing account. */

	const QString m_testPath; /*!< Test path. */
	QDir m_testDir;  /*!< Directory containing testing data. */

	const QString m_credFName; /*!< Credentials file name. */

	/* Log-in using user name and password. */
	LoginCredentials m_sender; /*!< Sender credentials. */
};

TestTaskSearchOwner::TestTaskSearchOwner(void)
    : m_testing(true),
    m_testPath(QDir::currentPath() + QDir::separator() + QLatin1String("_test_dir")),
    m_testDir(m_testPath),
    m_credFName(QLatin1String(CREDENTIALS_FNAME)),
    m_sender()
{
}

TestTaskSearchOwner::~TestTaskSearchOwner(void)
{
}

void TestTaskSearchOwner::initTestCase(void)
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

	/* Load credentials. */
	ret = m_sender.loadLoginCredentials(m_credFName, 1);
	if (!ret) {
		QSKIP("Failed to load login credentials. Skipping remaining tests.");
	}
	QVERIFY(ret);
	QVERIFY(!m_sender.userName.isEmpty());
	QVERIFY(!m_sender.pwd.isEmpty());

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
		QSKIP("Error connecting into ISDS.");
	}
	QVERIFY(err == IE_SUCCESS);
}

void TestTaskSearchOwner::cleanupTestCase(void)
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

void TestTaskSearchOwner::searchOwner(void)
{
	TaskSearchOwner *task = Q_NULLPTR;

	QVERIFY(!m_sender.userName.isEmpty());

	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_sender.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));

	Isds::DbOwnerInfo soughtOwnerInfo;
	QVERIFY(soughtOwnerInfo.isNull());

	/* Searching for empty owner info must fail. */
	task = new (std::nothrow) TaskSearchOwner(m_sender.userName, soughtOwnerInfo);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskSearchOwner::SO_ERROR);
	QVERIFY(task->m_foundBoxes.size() == 0);

	delete task; task = Q_NULLPTR;

	/* Searching for some data box should succeed. */
	{
		soughtOwnerInfo.setDbType(Isds::Type::BT_PO);

		Isds::PersonName personName;
		const QString queryName("barbucha");

		personName.setFirstName(queryName);
		personName.setLastName(queryName);
		soughtOwnerInfo.setPersonName(personName);
		soughtOwnerInfo.setFirmName(queryName);
	}
	QVERIFY(!soughtOwnerInfo.isNull());

	task = new (std::nothrow) TaskSearchOwner(m_sender.userName, soughtOwnerInfo);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskSearchOwner::SO_SUCCESS);
	QVERIFY(task->m_foundBoxes.size() >= 0);

	if (task->m_foundBoxes.size() != 1) {
		QSKIP("Received more than one search result.");
	}

	const Isds::DbOwnerInfo &foundOwnerInfo(task->m_foundBoxes.constFirst());
	QVERIFY(foundOwnerInfo.dbID() == QString("qrdae26"));

	delete task; task = Q_NULLPTR;
}

void TestTaskSearchOwner::searchOwnerFulltext01(void)
{
	TaskSearchOwnerFulltext *task = Q_NULLPTR;

	QVERIFY(!m_sender.userName.isEmpty());

	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_sender.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));

	/* Should receive a single result. */
	task = new (std::nothrow) TaskSearchOwnerFulltext(m_sender.userName,
	    QString("barbucha"), Isds::Type::FST_GENERAL,
	    TaskSearchOwnerFulltext::BT_ALL, 0, true, false);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskSearchOwnerFulltext::SOF_SUCCESS);
	QVERIFY(task->m_foundBoxes.size() >= 0);
	QVERIFY(((int)task->m_totalMatchingBoxes) == task->m_foundBoxes.size());

	if (task->m_foundBoxes.size() != 1) {
		QSKIP("Received more than one search result.");
	}

	const Isds::FulltextResult &foundResult(task->m_foundBoxes.constFirst());
	QVERIFY(foundResult.dbID() == QString("qrdae26"));
	QVERIFY(foundResult.nameMatches().size() == 0);
	QVERIFY(foundResult.addressMatches().size() == 0);

	delete task; task = Q_NULLPTR;
}

void TestTaskSearchOwner::searchOwnerFulltext02(void)
{
	TaskSearchOwnerFulltext *task = Q_NULLPTR;

	QVERIFY(!m_sender.userName.isEmpty());

	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_sender.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));

	/* Should receive many results. */
	task = new (std::nothrow) TaskSearchOwnerFulltext(m_sender.userName,
	    QString("praha"), Isds::Type::FST_GENERAL,
	    TaskSearchOwnerFulltext::BT_ALL, 0, true, false);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskSearchOwnerFulltext::SOF_SUCCESS);
	QVERIFY(task->m_foundBoxes.size() >= 0);
	QVERIFY(((int)task->m_totalMatchingBoxes) == task->m_foundBoxes.size());

	QVERIFY(task->m_foundBoxes.size() >= 500);
	foreach (const Isds::FulltextResult &foundResult, task->m_foundBoxes) {
		QVERIFY(foundResult.nameMatches().size() == 0);
		QVERIFY(foundResult.addressMatches().size() == 0);
	}

	delete task; task = Q_NULLPTR;
}

void TestTaskSearchOwner::searchOwnerFulltext03(void)
{
	TaskSearchOwnerFulltext *task = Q_NULLPTR;

	QVERIFY(!m_sender.userName.isEmpty());

	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));
	struct isds_ctx *ctx = GlobInstcs::isdsSessionsPtr->session(
	    m_sender.userName);
	QVERIFY(ctx != NULL);
	QVERIFY(GlobInstcs::isdsSessionsPtr->isConnectedToIsds(m_sender.userName));

	const QString query("praha");

	/* Should receive many results. */
	task = new (std::nothrow) TaskSearchOwnerFulltext(m_sender.userName,
	    query, Isds::Type::FST_GENERAL, TaskSearchOwnerFulltext::BT_ALL,
	    0, true, true);
	QVERIFY(task != Q_NULLPTR);
	task->setAutoDelete(false);

	task->run();

	QVERIFY(task->m_result == TaskSearchOwnerFulltext::SOF_SUCCESS);
	QVERIFY(task->m_foundBoxes.size() >= 0);
	QVERIFY(((int)task->m_totalMatchingBoxes) == task->m_foundBoxes.size());

	QVERIFY(task->m_foundBoxes.size() >= 500);
	foreach (const Isds::FulltextResult &foundResult, task->m_foundBoxes) {
		/* At least one list must be set. */
		QVERIFY((foundResult.nameMatches().size() > 0) || (foundResult.addressMatches().size() > 0));
		checkIndexList(foundResult.dbName(), query, foundResult.nameMatches());
		checkIndexList(foundResult.dbAddress(), query, foundResult.addressMatches());
	}

	delete task; task = Q_NULLPTR;
}

void TestTaskSearchOwner::checkIndexList(const QString &res,
    const QString &query, const QList< QPair<int, int> > &list)
{
	typedef QPair<int, int> entryType;

	foreach (const entryType &entry, list) {
		int start = entry.first;
		int stop = entry.second;
		int diff = stop - start;
		QVERIFY(diff >= 0);
		QString mid(res.mid(start, diff));
		/* The full-text search allows small deviations in the text. */
		if (query.toLower() != mid.toLower()) {
			fprintf(stderr, "Queried '%s', found '%s'.\n",
			    query.toUtf8().constData(), mid.toUtf8().constData());
		}
	}
}

QObject *newTestTaskSearchOwner(void)
{
	return new (::std::nothrow) TestTaskSearchOwner();
}

//QTEST_MAIN(TestTaskSearchOwner)
#include "test_task_search_owner.moc"
