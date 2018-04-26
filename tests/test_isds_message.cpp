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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <isds.h>
#include <QString>
#include <QtTest/QtTest>

#include "src/global.h"
#include "src/isds/message_conversion.h"
#include "src/isds/message_functions.h"
#include "src/isds/message_interface.h"
#include "src/log/log.h"
#include "tests/test_isds_message.h"

class TestIsdsMessage : public QObject {
	Q_OBJECT

public:
	TestIsdsMessage(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void nullResponse(void);

	void loadZfo(void);

	void compare(void);

	void conversionChain(void);

private:
	const QString msgPath;
	const QString delInfoPath;
};

TestIsdsMessage::TestIsdsMessage(void)
    : QObject(Q_NULLPTR),
    msgPath("data/DZ_6452235.zfo"),
    delInfoPath("data/DD_6452235.zfo")
{
}

void TestIsdsMessage::initTestCase(void)
{
	QVERIFY(GlobInstcs::logPtr == Q_NULLPTR);
	GlobInstcs::logPtr = new (std::nothrow) LogDevice;
	QVERIFY(GlobInstcs::logPtr != Q_NULLPTR);
}

void TestIsdsMessage::nullResponse(void)
{
	Isds::Message message;
	QVERIFY(message.isNull());

	QVERIFY(message.raw().isNull());
	QVERIFY(message.rawType() == Isds::Type::RT_UNKNOWN);
	QVERIFY(message.envelope().isNull());
	QVERIFY(message.documents().isEmpty());

	QVERIFY(Isds::messageFromData(QByteArray(), Isds::LT_ANY).isNull());
	QVERIFY(Isds::messageFromData(QByteArray(), Isds::LT_MESSAGE).isNull());
	QVERIFY(Isds::messageFromData(QByteArray(), Isds::LT_DELIVERY).isNull());

	QByteArray garbageData("abcdef01235");
	QVERIFY(Isds::messageFromData(garbageData, Isds::LT_ANY).isNull());
	QVERIFY(Isds::messageFromData(garbageData, Isds::LT_MESSAGE).isNull());
	QVERIFY(Isds::messageFromData(garbageData, Isds::LT_DELIVERY).isNull());
}

void TestIsdsMessage::loadZfo(void)
{
	QVERIFY(!Isds::messageFromFile(msgPath, Isds::LT_ANY).isNull());
	QVERIFY(!Isds::messageFromFile(msgPath, Isds::LT_MESSAGE).isNull());
	QVERIFY(Isds::messageFromFile(msgPath, Isds::LT_DELIVERY).isNull());

	QVERIFY(!Isds::messageFromFile(delInfoPath, Isds::LT_ANY).isNull());
	QVERIFY(Isds::messageFromFile(delInfoPath, Isds::LT_MESSAGE).isNull());
	QVERIFY(!Isds::messageFromFile(delInfoPath, Isds::LT_DELIVERY).isNull());
}

void TestIsdsMessage::compare(void)
{
	Isds::Message message01, message02;
	QVERIFY(message01.isNull());
	QVERIFY(message02.isNull());
	QVERIFY(message01 == message01);
	QVERIFY(message01 == message02);
	QVERIFY(message02 == message02);

	message01 = Isds::messageFromFile(msgPath, Isds::LT_MESSAGE);
	QVERIFY(!message01.isNull());
	QVERIFY(message01 == message01);
	QVERIFY(message01 != Isds::Message());

	message02 = Isds::messageFromFile(delInfoPath, Isds::LT_DELIVERY);
	QVERIFY(!message02.isNull());
	QVERIFY(message02 == message02);
	QVERIFY(message02 != Isds::Message());
}

void TestIsdsMessage::conversionChain(void)
{
	Isds::Message message01;
	struct isds_message *im01 = NULL;
	QVERIFY(message01.isNull());
	QVERIFY(im01 == NULL);

	message01 = Isds::messageFromFile(msgPath, Isds::LT_MESSAGE);
	QVERIFY(!message01.isNull());
	im01 = Isds::message2libisds(message01);
	QVERIFY(!message01.isNull());
	QVERIFY(im01 != NULL);

	isds_message_free(&im01);
	QVERIFY(im01 == NULL);
}

void TestIsdsMessage::cleanupTestCase(void)
{
	delete GlobInstcs::logPtr; GlobInstcs::logPtr = Q_NULLPTR;
}

QObject *newTestIsdsMessage(void)
{
	return new (std::nothrow) TestIsdsMessage();
}

//QTEST_MAIN(TestIsdsMessage)
#include "test_isds_message.moc"
