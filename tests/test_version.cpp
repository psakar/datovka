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

#include <QString>
#include <QtTest/QtTest>

#include "src/about.h"

class TestVersion : public QObject {
	Q_OBJECT

public:
	TestVersion(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void compareCleanLegalVersionStrings(void);

	void compareDirtyLegalVersionStrings(void);

	void compareIllegalVersionStrings(void);
};

TestVersion::TestVersion(void)
    : QObject(Q_NULLPTR)
{
}

QObject *newTestVersion(void)
{
	return new (std::nothrow) TestVersion();
}

void TestVersion::initTestCase(void)
{
	/* No initialisation is needed. */
}

void TestVersion::cleanupTestCase(void)
{
}

void TestVersion::compareCleanLegalVersionStrings(void)
{
	QVERIFY( 0 == compareVersionStrings("0.0.0", "0.0.0"));

	QVERIFY(-1 == compareVersionStrings("0.0.0", "0.0.1"));
	QVERIFY( 1 == compareVersionStrings("0.0.1", "0.0.0"));

	QVERIFY(-1 == compareVersionStrings("0.0.0", "0.1.0"));
	QVERIFY( 1 == compareVersionStrings("0.1.0", "0.0.0"));

	QVERIFY(-1 == compareVersionStrings("0.0.0", "0.1.1"));
	QVERIFY( 1 == compareVersionStrings("0.1.1", "0.0.0"));

	QVERIFY(-1 == compareVersionStrings("0.0.0", "1.0.0"));
	QVERIFY( 1 == compareVersionStrings("1.0.0", "0.0.0"));

	QVERIFY(-1 == compareVersionStrings("0.0.0", "1.0.1"));
	QVERIFY( 1 == compareVersionStrings("1.0.1", "0.0.0"));

	QVERIFY(-1 == compareVersionStrings("0.0.0", "1.1.0"));
	QVERIFY( 1 == compareVersionStrings("1.1.0", "0.0.0"));

	QVERIFY(-1 == compareVersionStrings("0.0.0", "1.1.1"));
	QVERIFY( 1 == compareVersionStrings("1.1.1", "0.0.0"));
//
	QVERIFY( 0 == compareVersionStrings("0.0.1", "0.0.1"));

	QVERIFY(-1 == compareVersionStrings("0.0.1", "0.1.0"));
	QVERIFY( 1 == compareVersionStrings("0.1.0", "0.0.1"));

	QVERIFY(-1 == compareVersionStrings("0.0.1", "0.1.1"));
	QVERIFY( 1 == compareVersionStrings("0.1.1", "0.0.1"));

	QVERIFY(-1 == compareVersionStrings("0.0.1", "1.0.0"));
	QVERIFY( 1 == compareVersionStrings("1.0.0", "0.0.1"));

	QVERIFY(-1 == compareVersionStrings("0.0.1", "1.0.1"));
	QVERIFY( 1 == compareVersionStrings("1.0.1", "0.0.1"));

	QVERIFY(-1 == compareVersionStrings("0.0.1", "1.1.0"));
	QVERIFY( 1 == compareVersionStrings("1.1.0", "0.0.1"));

	QVERIFY(-1 == compareVersionStrings("0.0.1", "1.1.1"));
	QVERIFY( 1 == compareVersionStrings("1.1.1", "0.0.1"));
//
	QVERIFY( 0 == compareVersionStrings("0.1.0", "0.1.0"));

	QVERIFY(-1 == compareVersionStrings("0.1.0", "0.1.1"));
	QVERIFY( 1 == compareVersionStrings("0.1.1", "0.1.0"));

	QVERIFY(-1 == compareVersionStrings("0.1.0", "1.0.0"));
	QVERIFY( 1 == compareVersionStrings("1.0.0", "0.1.0"));

	QVERIFY(-1 == compareVersionStrings("0.1.0", "1.0.1"));
	QVERIFY( 1 == compareVersionStrings("1.0.1", "0.1.0"));

	QVERIFY(-1 == compareVersionStrings("0.1.0", "1.1.0"));
	QVERIFY( 1 == compareVersionStrings("1.1.0", "0.1.0"));

	QVERIFY(-1 == compareVersionStrings("0.1.0", "1.1.1"));
	QVERIFY( 1 == compareVersionStrings("1.1.1", "0.1.0"));
//
	QVERIFY( 0 == compareVersionStrings("0.1.1", "0.1.1"));

	QVERIFY(-1 == compareVersionStrings("0.1.1", "1.0.0"));
	QVERIFY( 1 == compareVersionStrings("1.0.0", "0.1.1"));

	QVERIFY(-1 == compareVersionStrings("0.1.1", "1.0.1"));
	QVERIFY( 1 == compareVersionStrings("1.0.1", "0.1.1"));

	QVERIFY(-1 == compareVersionStrings("0.1.1", "1.1.0"));
	QVERIFY( 1 == compareVersionStrings("1.1.0", "0.1.1"));

	QVERIFY(-1 == compareVersionStrings("0.1.1", "1.1.1"));
	QVERIFY( 1 == compareVersionStrings("1.1.1", "0.1.1"));
//
	QVERIFY( 0 == compareVersionStrings("1.0.0", "1.0.0"));

	QVERIFY(-1 == compareVersionStrings("1.0.0", "1.0.1"));
	QVERIFY( 1 == compareVersionStrings("1.0.1", "1.0.0"));

	QVERIFY(-1 == compareVersionStrings("1.0.0", "1.1.0"));
	QVERIFY( 1 == compareVersionStrings("1.1.0", "1.0.0"));

	QVERIFY(-1 == compareVersionStrings("1.0.0", "1.1.1"));
	QVERIFY( 1 == compareVersionStrings("1.1.1", "1.0.0"));
//
	QVERIFY( 0 == compareVersionStrings("1.0.1", "1.0.1"));

	QVERIFY(-1 == compareVersionStrings("1.0.1", "1.1.0"));
	QVERIFY( 1 == compareVersionStrings("1.1.0", "1.0.1"));

	QVERIFY(-1 == compareVersionStrings("1.0.1", "1.1.1"));
	QVERIFY( 1 == compareVersionStrings("1.1.1", "1.0.1"));
//
	QVERIFY( 0 == compareVersionStrings("1.1.0", "1.1.0"));

	QVERIFY(-1 == compareVersionStrings("1.1.0", "1.1.1"));
	QVERIFY( 1 == compareVersionStrings("1.1.1", "1.1.0"));
//
	QVERIFY( 0 == compareVersionStrings("4.9.3", "4.9.3"));
	QVERIFY( 0 == compareVersionStrings("4.10.0", "4.10.0"));

	QVERIFY(-1 == compareVersionStrings("4.9.3", "4.10.0"));
	QVERIFY( 1 == compareVersionStrings("4.10.0", "4.9.3"));
}

void TestVersion::compareDirtyLegalVersionStrings(void)
{
	QVERIFY( 0 == compareVersionStrings("_0.0.0", "0.0.0+"));
	QVERIFY( 0 == compareVersionStrings(" 0.0.0", "0.0.0 "));
	QVERIFY( 0 == compareVersionStrings(" 0.0.0 ", " 0.0.0 "));
//
	QVERIFY( 0 == compareVersionStrings("ballast_+" "4.9.3" " garbage", "added " "4.9.3" "-string"));
	QVERIFY( 0 == compareVersionStrings("foo" "4.10.0" "bar", "goo " "4.10.0" "boo"));

	QVERIFY(-1 == compareVersionStrings("_" "4.9.3", "+" "4.10.0"));
	QVERIFY( 1 == compareVersionStrings("+" "4.10.0", "4.9.3" "_"));
}

void TestVersion::compareIllegalVersionStrings(void)
{
	QVERIFY(-2 == compareVersionStrings("4.-9.3", "4.10.0"));
	QVERIFY(-2 == compareVersionStrings("4.10.-0", "4.9.3"));

	QVERIFY( 2 == compareVersionStrings("4.9.3", "4.+10.0"));
	QVERIFY( 2 == compareVersionStrings("4.10.0", "4.9.+3"));

	QVERIFY(-2 == compareVersionStrings("4.9", "4.10.0"));
	QVERIFY( 2 == compareVersionStrings("4.10.0", "4.9"));

	QVERIFY( 2 == compareVersionStrings("4.9.3", "4.10"));
	QVERIFY(-2 == compareVersionStrings("4.10", "4.9.3"));
}

//QTEST_MAIN(TestVersion)
#include "test_version.moc"
