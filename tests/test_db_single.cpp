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
#include <QFile>
#include <QFileInfo>
#include <QtTest/QtTest>

#include "src/global.h"
#include "src/io/tag_db.h"
#include "src/log/log.h"
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
	QVERIFY(GlobInstcs::logPtr == Q_NULLPTR);
	GlobInstcs::logPtr = new (std::nothrow) LogDevice;
	QVERIFY(GlobInstcs::logPtr != Q_NULLPTR);

	/* Pointer must be null before initialisation. */
	QVERIFY(GlobInstcs::tagDbPtr == Q_NULLPTR);

	QVERIFY(m_dbDir.removeRecursively());
	QVERIFY(!m_dbDir.exists());

	QVERIFY(m_dbDir.mkpath("."));
}

void TestDbSingle::cleanupTestCase(void)
{
	QVERIFY(GlobInstcs::tagDbPtr == Q_NULLPTR);

	delete GlobInstcs::logPtr; GlobInstcs::logPtr = Q_NULLPTR;

	QVERIFY(m_dbDir.removeRecursively());
	QVERIFY(!m_dbDir.exists());
}

void TestDbSingle::init(void)
{
	QVERIFY(GlobInstcs::tagDbPtr == Q_NULLPTR);

	QFile(m_dbPath1).remove();
	QFile(m_dbPath2).remove();

	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::cleanup(void)
{
	delete GlobInstcs::tagDbPtr; GlobInstcs::tagDbPtr = Q_NULLPTR;
}

void TestDbSingle::relocateImmovableFile(void)
{
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, false);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateImmovableMemory(void)
{
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, false);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(
	    m_dbPath1, SQLiteDb::CREATE_MISSING | SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileCopy(void)
{
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
//	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
//	QVERIFY(QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

//	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
//	QVERIFY(!QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileReopen(void)
{
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
//	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
//	QVERIFY(QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(!checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(!checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

//	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
//	QVERIFY(!QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileMove(void)
{
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
//	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
//	QVERIFY(QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

//	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
//	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
//	QVERIFY(!QFileInfo::exists(m_dbPath1));
//	QVERIFY(!QFileInfo::exists(m_dbPath2));
}

void TestDbSingle::relocateMovableFileOpen(void)
{
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(!checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(!checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(!checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(QFileInfo::exists(m_dbPath2));

	cleanup();
	init();
	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(!checkContent(GlobInstcs::tagDbPtr));
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

	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb(m_connectionPrefix, true);
	QVERIFY(GlobInstcs::tagDbPtr != Q_NULLPTR);
	QVERIFY(GlobInstcs::tagDbPtr->openDb(
	    m_dbPath1, SQLiteDb::CREATE_MISSING | SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(fillDbContent(GlobInstcs::tagDbPtr));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->copyDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->reopenDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->moveDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath1, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));

	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::NO_OPTIONS));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::CREATE_MISSING));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
	QVERIFY(!QFileInfo::exists(m_dbPath1));
	QVERIFY(!QFileInfo::exists(m_dbPath2));
	QVERIFY(!GlobInstcs::tagDbPtr->openDb(m_dbPath2, SQLiteDb::FORCE_IN_MEMORY));
	QVERIFY(checkContent(GlobInstcs::tagDbPtr));
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
