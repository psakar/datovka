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
#include <QtTest/QtTest>

#include "src/global.h"
#include "src/io/message_db_set_container.h"
#include "src/settings/preferences.h"
#include "tests/test_db_container.h"

class TestDbContainer : public QObject {
	Q_OBJECT

public:
	TestDbContainer(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void accessDbSet00(void);

	void accessDbSet01(void);

	void deleteAndRecreate01(void);

	void accessDbSet02(void);

	void accessDbSet03(void);

	void accessDbSet04(void);

	void deleteAndRecreate02(void);

	void accessDbSet05(void);

	void deleteDbSet(void);

private:
	inline
	QStringList dbFileNameList(void) const
	{
		return m_dbDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
	}

	void deleteAndRecreate(void);

	const QString m_connectionPrefix; /*!< SQL connection prefix. */
	const QString m_dbLocationPath; /*!< Directory path. */
	QDir m_dbDir;  /*!< Directory containing testing databases. */
};

#define printDirContent() \
	fprintf(stderr, "Content: %s\n", dbFileNameList().join(" ").toUtf8().constData())

TestDbContainer::TestDbContainer(void)
    : m_connectionPrefix(QLatin1String("GLOBALDBS")),
    m_dbLocationPath(QDir::currentPath() + QDir::separator() + QLatin1String("_db_sets")),
    m_dbDir(m_dbLocationPath)
{
}

void TestDbContainer::initTestCase(void)
{
	QVERIFY(GlobInstcs::prefsPtr == Q_NULLPTR);
	GlobInstcs::prefsPtr = new (std::nothrow) GlobPreferences;
	QVERIFY(GlobInstcs::prefsPtr != Q_NULLPTR);

	/* Pointer must be null before initialisation. */
	QVERIFY(GlobInstcs::msgDbsPtr == Q_NULLPTR);
	GlobInstcs::msgDbsPtr = new (std::nothrow) DbContainer(m_connectionPrefix);
	QVERIFY(GlobInstcs::msgDbsPtr != Q_NULLPTR);

	/* Remove and check that is not present. */
	QVERIFY(m_dbDir.removeRecursively());
	QVERIFY(!m_dbDir.exists());
}

void TestDbContainer::cleanupTestCase(void)
{
	delete GlobInstcs::msgDbsPtr; GlobInstcs::msgDbsPtr = Q_NULLPTR;

	delete GlobInstcs::prefsPtr; GlobInstcs::prefsPtr = Q_NULLPTR;

	QVERIFY(m_dbDir.removeRecursively());
	QVERIFY(!m_dbDir.exists());
}

void TestDbContainer::accessDbSet00(void)
{
	const QString uName(QLatin1String("user000"));
	bool testing = false;

	MessageDbSet *dbSet;

	/* Nothing does exist, there is nothing to open. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	/* CM_CREATE_EMPTY_CURRENT cannot be used with DO_UNKNOWN. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_CREATE_ON_DEMAND);
	QVERIFY(dbSet == NULL);

	/* CM_CREATE_ON_DEMAND cannot be used with DO_UNKNOWN. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_CREATE_ON_DEMAND);
	QVERIFY(dbSet == NULL);
}

void TestDbContainer::accessDbSet01(void)
{
	const QString uName1(QLatin1String("user001"));
	const QString uName2(QLatin1String("user002"));
	const QString uName3(QLatin1String("user003"));
	bool testing = false;

	MessageDbSet *dbSet, *dbSet2;

	/* Nothing does exist, there is nothing to open. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName1,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	/* Open database, but actually don't create a file. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName1,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_CREATE_ON_DEMAND);
	QVERIFY(dbSet != NULL);

	/* There must be no files in the directory. */
	QVERIFY(dbFileNameList().isEmpty());

	/* Open database, create a file. There must be one file. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 1);

	/* Open database again. There must be one file. */
	dbSet2 = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
	QVERIFY(dbSet2 == dbSet);
	QVERIFY(dbFileNameList().size() == 1);

	/* Open database, create a file. There must be two files. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 2);

	/* Open database again. There must be one file.  */
	dbSet2 = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
	QVERIFY(dbSet2 == dbSet);
	QVERIFY(dbFileNameList().size() == 2);
}

void TestDbContainer::deleteAndRecreate01(void)
{
	deleteAndRecreate();
}

void TestDbContainer::accessDbSet02(void)
{
	const QString uName1(QLatin1String("user001"));
	const QString uName2(QLatin1String("user002"));
	const QString uName3(QLatin1String("user003"));
	bool testing = false;

	MessageDbSet *dbSet;

	/* Nothing does exist, there is nothing to open. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName1,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName1,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	/* Database exists, but is a single file. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	/* Database exists, but is yearly organised. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);
}

void TestDbContainer::accessDbSet03(void)
{
	const QString uName1(QLatin1String("user001"));
	const QString uName2(QLatin1String("user002"));
	const QString uName3(QLatin1String("user003"));
	bool testing = true;

	MessageDbSet *dbSet;

	/* Nothing does exist, there is nothing to open. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName1,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName1,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	/* Database exists, but is a single file. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	/* Database exists, but is yearly organised. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);
}

void TestDbContainer::accessDbSet04(void)
{
	const QString uName2(QLatin1String("user002"));
	const QString uName3(QLatin1String("user003"));
	bool testing = false;

	MessageDbSet *dbSet;

	/* Databases exist. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_SINGLE_FILE,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 2);

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_YEARLY,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 2);
}

void TestDbContainer::deleteAndRecreate02(void)
{
	deleteAndRecreate();
}

void TestDbContainer::accessDbSet05(void)
{
	const QString uName2(QLatin1String("user002"));
	const QString uName3(QLatin1String("user003"));
	bool testing = false;

	MessageDbSet *dbSet;

	/* Databases exist. */
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 2);

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 2);
}

void TestDbContainer::deleteDbSet(void)
{
	const QString uName1(QLatin1String("user001"));
	const QString uName2(QLatin1String("user002"));
	const QString uName3(QLatin1String("user003"));
	bool testing = false;

	MessageDbSet *dbSet;

	/*
	 * The method will fail on NULL pointer ow on values that aren't
	 * handled by the container.
	 */
	//QVERIFY(!GlobInstcs::msgDbsPtr->deleteDbSet(NULL));

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 2);
	QVERIFY(GlobInstcs::msgDbsPtr->deleteDbSet(dbSet));
	QVERIFY(dbFileNameList().size() == 1);
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName2,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);

	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet != NULL);
	QVERIFY(dbFileNameList().size() == 1);
	QVERIFY(GlobInstcs::msgDbsPtr->deleteDbSet(dbSet));
	QVERIFY(dbFileNameList().isEmpty());
	dbSet = GlobInstcs::msgDbsPtr->accessDbSet(m_dbLocationPath, uName3,
	    testing, MessageDbSet::DO_UNKNOWN,
	    MessageDbSet::CM_MUST_EXIST);
	QVERIFY(dbSet == NULL);
}

void TestDbContainer::deleteAndRecreate(void)
{
	delete GlobInstcs::msgDbsPtr; GlobInstcs::msgDbsPtr = Q_NULLPTR;

	GlobInstcs::msgDbsPtr = new (std::nothrow) DbContainer(m_connectionPrefix);
	QVERIFY(GlobInstcs::msgDbsPtr != Q_NULLPTR);
}

QObject *newTestDbContainer(void)
{
	return new (std::nothrow) TestDbContainer();
}

//QTEST_MAIN(TestDbContainer)
#include "test_db_container.moc"
