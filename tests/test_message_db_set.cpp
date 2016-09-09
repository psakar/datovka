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

#include <ctime>
#include <QDir>
#include <QtTest/QtTest>

#include "src/common.h"
#include "src/io/dbs.h"
#include "src/io/message_db_set.h"
#include "tests/test_message_db_set.h"

class DatabaseIdentifier {
public:
	DatabaseIdentifier(const QString &k, bool t,
	    enum MessageDbSet::Organisation o);

	QString primaryKey; /*!< Part of the database file name. */
	bool testing; /*!< Part of the database name. */
	enum MessageDbSet::Organisation organisation; /*!< Part of the database file name. */
};

DatabaseIdentifier::DatabaseIdentifier(const QString &k, bool t,
    enum MessageDbSet::Organisation o)
    : primaryKey(k),
    testing(t),
    organisation(o)
{
}

class TestMessageDbSet : public QObject {
	Q_OBJECT

public:
	TestMessageDbSet(void);
	~TestMessageDbSet(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void accessEmpty(void);

	void writeEnvelope(void);

private:
	static inline
	QStringList dbFileNameList(const QDir &dbDir)
	{
		return dbDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
	}

	static
	void dateTimeToTimeval(struct timeval &tv, const QDateTime &dateTime);

	void cleanup(bool deleteFiles);

	const QString m_connectionPrefix; /*!< SQL connection prefix. */

	const QString m_dbLocationPathSrc; /*!< Directory path. */
	QDir m_dbDirSrc;  /*!< Directory containing testing databases. */

	const QString m_dbLocationPathDst; /*!< Directory path. */
	QDir m_dbDirDst;  /*!< Directory containing testing databases. */

	const DatabaseIdentifier m_dbId01; /*!< Database identifier. */
	const DatabaseIdentifier m_dbId02; /*!< Database identifier. */

	MessageDbSet *m_dbSet01;
	MessageDbSet *m_dbSet02;
};

TestMessageDbSet::TestMessageDbSet(void)
    : m_connectionPrefix(QLatin1String("GLOBALDBS")),
    m_dbLocationPathSrc(QDir::currentPath() + QDir::separator() + QLatin1String("_db_sets_src")),
    m_dbDirSrc(m_dbLocationPathSrc),
    m_dbLocationPathDst(QDir::currentPath() + QDir::separator() + QLatin1String("_db_sets_dst")),
    m_dbDirDst(m_dbLocationPathDst),
    m_dbId01(QLatin1String("user001"), true, MessageDbSet::DO_SINGLE_FILE),
    m_dbId02(QLatin1String("user002"), true, MessageDbSet::DO_YEARLY),
    m_dbSet01(NULL),
    m_dbSet02(NULL)
{
	/* Remove and check that is not present. */
	QVERIFY(m_dbDirSrc.removeRecursively());
	QVERIFY(!m_dbDirSrc.exists());

	QVERIFY(m_dbDirDst.removeRecursively());
	QVERIFY(!m_dbDirDst.exists());
}

TestMessageDbSet::~TestMessageDbSet(void)
{
	/* Just in case. */
	cleanup(false);
}

void TestMessageDbSet::initTestCase(void)
{
	/* Try accessing a non-existent one. */
	m_dbSet01 = MessageDbSet::createNew(m_dbLocationPathSrc,
	    m_dbId01.primaryKey, m_dbId01.testing, m_dbId01.organisation,
	    m_connectionPrefix, MessageDbSet::CM_MUST_EXIST);
	QVERIFY(m_dbSet01 == NULL);
	QVERIFY(!m_dbDirSrc.exists());

	m_dbSet02 = MessageDbSet::createNew(m_dbLocationPathSrc,
	    m_dbId02.primaryKey, m_dbId02.testing, m_dbId02.organisation,
	    m_connectionPrefix, MessageDbSet::CM_MUST_EXIST);
	QVERIFY(m_dbSet02 == NULL);
	QVERIFY(!m_dbDirSrc.exists());
}

void TestMessageDbSet::accessEmpty(void)
{
	MessageDb *dbA, *dbB, *dbC;

	QDateTime dateTimeA(QDate(2014, 1, 31), QTime(0, 0, 0, 0), Qt::UTC);
	QDateTime dateTimeB(QDate(2015, 3, 31), QTime(0, 0, 0, 0), Qt::UTC);
	QDateTime dateTimeC(QDate(2016, 8, 31), QTime(0, 0, 0, 0), Qt::UTC);

	/* Accessing a single file database. */
	m_dbSet01 = MessageDbSet::createNew(m_dbLocationPathSrc,
	    m_dbId01.primaryKey, m_dbId01.testing, m_dbId01.organisation,
	    m_connectionPrefix, MessageDbSet::CM_CREATE_EMPTY_CURRENT);
	QVERIFY(m_dbSet01 != NULL);
	QVERIFY(m_dbDirSrc.exists());
	QVERIFY(dbFileNameList(m_dbDirSrc).size() == 1);

	QVERIFY(m_dbSet01->fileNames().size() == 1);
	dbA = m_dbSet01->constAccessMessageDb(dateTimeA);
	QVERIFY(dbA != NULL);
	QVERIFY(m_dbSet01->fileNames().size() == 1);
	dbB = m_dbSet01->accessMessageDb(dateTimeB, false);
	QVERIFY(dbB != NULL);
	QVERIFY(dbA == dbB);
	QVERIFY(m_dbSet01->fileNames().size() == 1);
	dbC = m_dbSet01->accessMessageDb(dateTimeC, true);
	QVERIFY(dbC != NULL);
	QVERIFY(dbA == dbC);
	QVERIFY(m_dbSet01->fileNames().size() == 1);

	QVERIFY(dbFileNameList(m_dbDirSrc).size() == 1);

	/* Accessing a multi-file database. */
	m_dbSet02 = MessageDbSet::createNew(m_dbLocationPathSrc,
	    m_dbId02.primaryKey, m_dbId02.testing, m_dbId02.organisation,
	    m_connectionPrefix, MessageDbSet::CM_CREATE_ON_DEMAND);
	QVERIFY(m_dbSet02 != NULL);
	QVERIFY(m_dbDirSrc.exists());
	QVERIFY(dbFileNameList(m_dbDirSrc).size() == 1);

	QVERIFY(m_dbSet02->fileNames().size() == 0);
	dbA = m_dbSet02->constAccessMessageDb(dateTimeA);
	QVERIFY(dbA == NULL);
	QVERIFY(m_dbSet02->fileNames().size() == 0);
	dbB = m_dbSet02->accessMessageDb(dateTimeB, false);
	QVERIFY(dbB == NULL);
	QVERIFY(m_dbSet02->fileNames().size() == 0);
	dbB = m_dbSet02->accessMessageDb(dateTimeB, true);
	QVERIFY(dbB != NULL);
	QVERIFY(m_dbSet02->fileNames().size() == 1);
	dbC = m_dbSet02->accessMessageDb(dateTimeC, true);
	QVERIFY(dbC != NULL);
	QVERIFY(dbB != dbC);
	QVERIFY(m_dbSet02->fileNames().size() == 2);

	QVERIFY(dbFileNameList(m_dbDirSrc).size() == 3);
}

void TestMessageDbSet::writeEnvelope(void)
{
	/* Write some data into the databases. */

	QDateTime dateTime(QDate(2014, 1, 31), QTime(0, 0, 0, 0), Qt::UTC);

	MessageDb *db;
	struct timeval tv;
	bool ret;

	dateTimeToTimeval(tv, dateTime);

	db = m_dbSet01->accessMessageDb(dateTime, true);
	QVERIFY(db != NULL);

	ret = db->msgsInsertMessageEnvelope(1,
	    QLatin1String("_origin"), QLatin1String("dbIDSender"),
	    QLatin1String("dmSender"), QLatin1String("dmSenderAddress"),
	    0, QLatin1String("dmRecipient"),
	    QLatin1String("dmRecipientAddress"),
	    QLatin1String("dmAmbiguousRecipient"),
	    QLatin1String("dmSenderOrgUnit"), QLatin1String("dmSenderOrgUnitNum"),
	    QLatin1String("dbIDRecipient"), QLatin1String("dmRecipientOrgUnit"),
	    QLatin1String("dmRecipientOrgUnitNum"), QLatin1String("dmToHands"),
	    QLatin1String("dmAnnotation"), QLatin1String("dmRecipientRefNumber"),
	    QLatin1String("dmSenderRefNumber"), QLatin1String("dmRecipientIdent"),
	    QLatin1String("dmSenderIdent"), QLatin1String("dmLegalTitleLaw"),
	    QLatin1String("dmLegalTitleYear"), QLatin1String("dmLegalTitleSect"),
	    QLatin1String("dmLegalTitlePar"), QLatin1String("dmLegalTitlePoint"),
	    false, false,
	    QByteArray(),
	    timevalToDbFormat(&tv), timevalToDbFormat(&tv),
	    0, 0, QLatin1String("_dmType"),
	    MSG_RECEIVED);
	QVERIFY(ret);
	QVERIFY(m_dbSet01->fileNames().size() == 1);
	QVERIFY(dbFileNameList(m_dbDirSrc).size() == 3);

	db = m_dbSet02->accessMessageDb(dateTime, true);
	QVERIFY(db != NULL);

	ret = db->msgsInsertMessageEnvelope(1,
	    QLatin1String("_origin"), QLatin1String("dbIDSender"),
	    QLatin1String("dmSender"), QLatin1String("dmSenderAddress"),
	    0, QLatin1String("dmRecipient"),
	    QLatin1String("dmRecipientAddress"),
	    QLatin1String("dmAmbiguousRecipient"),
	    QLatin1String("dmSenderOrgUnit"), QLatin1String("dmSenderOrgUnitNum"),
	    QLatin1String("dbIDRecipient"), QLatin1String("dmRecipientOrgUnit"),
	    QLatin1String("dmRecipientOrgUnitNum"), QLatin1String("dmToHands"),
	    QLatin1String("dmAnnotation"), QLatin1String("dmRecipientRefNumber"),
	    QLatin1String("dmSenderRefNumber"), QLatin1String("dmRecipientIdent"),
	    QLatin1String("dmSenderIdent"), QLatin1String("dmLegalTitleLaw"),
	    QLatin1String("dmLegalTitleYear"), QLatin1String("dmLegalTitleSect"),
	    QLatin1String("dmLegalTitlePar"), QLatin1String("dmLegalTitlePoint"),
	    false, false,
	    QByteArray(),
	    timevalToDbFormat(&tv), timevalToDbFormat(&tv),
	    0, 0, QLatin1String("_dmType"),
	    MSG_RECEIVED);
	QVERIFY(ret);
	QVERIFY(m_dbSet02->fileNames().size() == 3);
	QVERIFY(dbFileNameList(m_dbDirSrc).size() == 4);
}

void TestMessageDbSet::cleanupTestCase(void)
{
	cleanup(true);
}

void TestMessageDbSet::dateTimeToTimeval(struct timeval &tv,
    const QDateTime &dateTime)
{
	tv.tv_sec = dateTime.toTime_t();
	tv.tv_usec = dateTime.time().msec() * 1000;
}

void TestMessageDbSet::cleanup(bool deleteFiles)
{
	delete m_dbSet01; m_dbSet01 = NULL;
	delete m_dbSet02; m_dbSet02 = NULL;

	if (deleteFiles) {
		QVERIFY(m_dbDirSrc.removeRecursively());
		QVERIFY(!m_dbDirSrc.exists());

		QVERIFY(m_dbDirDst.removeRecursively());
		QVERIFY(!m_dbDirDst.exists());
	}
}

QObject *newTestMessageDbSet(void)
{
	return new (std::nothrow) TestMessageDbSet();
}

//QTEST_MAIN(TestMessageDbSet)
#include "test_message_db_set.moc"
