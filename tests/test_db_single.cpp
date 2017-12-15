/*
 * Copyright (C) 2014-2017 CZ.NIC
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
#include <QFile>
#include <QFileInfo>
#include <QtTest/QtTest>

#include "src/io/tag_db.h"
#include "tests/test_db_single.h"

class TestDbSingle : public QObject {
	Q_OBJECT

public:
	TestDbSingle(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void init(void);

	void cleanup(void);

	void relocateImmovableFile(void);

	void relocateImmovableMemory(void);

	void relocateMovableFileCopy(void);

	void relocateMovableFileReopen(void);

	void relocateMovableFileMove(void);

	void relocateMovableFileOpen(void);

	void relocateMovableMemory(void);

private:
	bool fillDbContent(TagDb *dbPtr) const;

	bool checkContent(TagDb *dbPtr) const;

	static
	bool filePresent(const QString &fileName);

	const QString m_connectionPrefix; /*!< SQL connection prefix. */
	const QString m_dbLocationPath; /*!< Directory path. */
	QDir m_dbDir;  /*!< Directory containing testing databases. */
	const QString m_dbName1; /*!< Database file name. */
	const QString m_dbName2; /*!< Database file name. */
	const QString m_dbPath1;
	const QString m_dbPath2;
	const QString m_tagName;
	const QString m_tagColour;
};

TestDbSingle::TestDbSingle(void)
    : m_connectionPrefix(QLatin1String("TAGDB")),
    m_dbLocationPath(QDir::currentPath() + QDir::separator() + QLatin1String("_db")),
    m_dbDir(m_dbLocationPath),
    m_dbName1(QLatin1String("tag1.db")),
    m_dbName2(QLatin1String("tag2.db")),
    m_dbPath1(m_dbLocationPath + QDir::separator() + m_dbName1),
    m_dbPath2(m_dbLocationPath + QDir::separator() + m_dbName2),
    m_tagName(QLatin1String("tag_name")),
    m_tagColour(QLatin1String("00ff00"))
{
}

void TestDbSingle::initTestCase(void)
{
	/* Pointer must be null before initialisation. */
	QVERIFY(globTagDbPtr == Q_NULLPTR);

	QVERIFY(m_dbDir.removeRecursively());
	QVERIFY(!m_dbDir.exists());

	QVERIFY(m_dbDir.mkpath("."));
}

void TestDbSingle::cleanupTestCase(void)
{
	QVERIFY(globTagDbPtr == Q_NULLPTR);

	QVERIFY(m_dbDir.removeRecursively());
	QVERIFY(!m_dbDir.exists());
}

void TestDbSingle::init(void)
{
	QVERIFY(globTagDbPtr == Q_NULLPTR);

	QFile(m_dbPath1).remove();
	QFile(m_dbPath2).remove();

	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::cleanup(void)
{
	delete globTagDbPtr; globTagDbPtr = Q_NULLPTR;
}

void TestDbSingle::relocateImmovableFile(void)
{
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, false);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateImmovableMemory(void)
{
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, false);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(
	    m_dbPath1, SQLiteDb::CREATE_MISSING | SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileCopy(void)
{
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
//	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(globTagDbPtr));
//	QVERIFY(QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

//	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(globTagDbPtr));
//	QVERIFY(!QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileReopen(void)
{
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
//	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(globTagDbPtr));
//	QVERIFY(QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(!checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(!checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

//	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(globTagDbPtr));
//	QVERIFY(!QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileMove(void)
{
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
//	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(globTagDbPtr));
//	QVERIFY(QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

//	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(globTagDbPtr));
//	QVERIFY(!QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileOpen(void)
{
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(!checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(!checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(!checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(globTagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(!checkContent(globTagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableMemory(void)
{
	/*
	 * TODO -- Relocating databases located in memory is not currently
	 * supported and causes assertion checks to fail.
	 */
	return;

	globTagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(globTagDbPtr != Q_NULLPTR);
	QVERIFY(globTagDbPtr->openDb(
	    m_dbPath1, SQLiteDb::CREATE_MISSING | SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(fillDbContent(globTagDbPtr));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!globTagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(globTagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

bool TestDbSingle::fillDbContent(TagDb *dbPtr) const
{
	if (Q_UNLIKELY(dbPtr == Q_NULLPTR)) {
		return false;
	}

	return dbPtr->insertTag(m_tagName, m_tagColour);
}

bool TestDbSingle::checkContent(TagDb *dbPtr) const
{
	if (Q_UNLIKELY(dbPtr == Q_NULLPTR)) {
		return false;
	}

	QList<TagDb::TagEntry> tagEntries(dbPtr->getAllTags());
	if (tagEntries.size() != 1) {
		return false;
	}

	const TagDb::TagEntry &entry = tagEntries.at(0);
	return entry.isValid() &&
	    (entry.name == m_tagName) && (entry.colour == m_tagColour);
}

QObject *newTestDbSingle(void)
{
	return new (std::nothrow) TestDbSingle();
}

//QTEST_MAIN(TestDbSingle)
#include "test_db_single.moc"
