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

#include "src/common.h"
#include "src/global.h"
#include "src/io/isds_login.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"
#include "tests/helper_qt.h"
#include "tests/test_isds_login.h"

class TestIsdsLogin : public QObject {
	Q_OBJECT

public:
	TestIsdsLogin(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void logIn01(void);

	void logIn02(void);

private:
	const QString m_credFName; /*!< Credentials file name. */

	LoginCredentials m_user01; /*!< Login credentials. */
};

TestIsdsLogin::TestIsdsLogin(void)
    : m_credFName(QLatin1String(CREDENTIALS_FNAME)),
    m_user01()
{
}

void TestIsdsLogin::initTestCase(void)
{
	QVERIFY(GlobInstcs::prefsPtr == Q_NULLPTR);
	GlobInstcs::prefsPtr = new (std::nothrow) GlobPreferences;
	QVERIFY(GlobInstcs::prefsPtr != Q_NULLPTR);

	/* Load credentials. */
	bool ret = m_user01.loadLoginCredentials(m_credFName, 1);
	if (!ret) {
		QSKIP("Failed to load login credentials. Skipping remaining tests.");
	}
	QVERIFY(ret);
	QVERIFY(m_user01.loginType != AcntSettings::LIM_UNKNOWN);
	QVERIFY(!m_user01.userName.isEmpty());
	QVERIFY(!m_user01.pwd.isEmpty());
}

void TestIsdsLogin::cleanupTestCase(void)
{
	m_user01.clearAll();

	delete GlobInstcs::prefsPtr; GlobInstcs::prefsPtr = Q_NULLPTR;
}

void TestIsdsLogin::logIn01(void)
{
	IsdsSessions sessions;
	AcntSettings settings;
	enum IsdsLogin::ErrorCode errCode;
	IsdsLogin isdsLogin(sessions, settings);

	settings.setTestAccount(true);

	/* No data at all. */
	errCode = isdsLogin.logIn();
	QVERIFY2(errCode == IsdsLogin::EC_ERR,
	    QString("Got error code %1.").arg(errCode).toUtf8().constData());

	settings.setAccountName(QLatin1String("Some account name"));
	settings.setUserName(m_user01.userName);
	settings.setPassword(m_user01.pwd);

	/* Have login data but no session. */
	errCode = isdsLogin.logIn();
	QVERIFY2(errCode == IsdsLogin::EC_ERR,
	    QString("Got error code %1.").arg(errCode).toUtf8().constData());

	QVERIFY(!sessions.holdsSession(settings.userName()));
	QVERIFY(0 != sessions.createCleanSession(settings.userName(),
	                 ISDS_CONNECT_TIMEOUT_MS));
	QVERIFY(sessions.holdsSession(settings.userName()));

	/* Have all data but no login method specified. */
	errCode = isdsLogin.logIn();
	QVERIFY2(errCode == IsdsLogin::EC_NOT_IMPL,
	    QString("Got error code %1.").arg(errCode).toUtf8().constData());

	settings.setLoginMethod(m_user01.loginType);

	/* Have all data. */
	errCode = isdsLogin.logIn();
	QVERIFY2(errCode == IsdsLogin::EC_OK,
	    QString("Got error code %1 '%2'.").arg(errCode).arg(isdsLogin.isdsErrMsg()).toUtf8().constData());

	/*
	 * Deleting password should not have an impact, because already
	 * logged in.
	 */
	settings.setPassword("");
	errCode = isdsLogin.logIn();
	QVERIFY2(errCode == IsdsLogin::EC_OK,
	    QString("Got error code %1 '%2'.").arg(errCode).arg(isdsLogin.isdsErrMsg()).toUtf8().constData());
}

void TestIsdsLogin::logIn02(void)
{
	IsdsSessions sessions;
	AcntSettings settings;

	IsdsLogin isdsLogin(sessions, settings);

	/* Create session. */
	QVERIFY(!sessions.holdsSession(settings.userName()));
	QVERIFY(0 != sessions.createCleanSession(settings.userName(),
	                 ISDS_CONNECT_TIMEOUT_MS));
	QVERIFY(sessions.holdsSession(settings.userName()));

	/* No data at all. */
	QVERIFY(isdsLogin.logIn() == IsdsLogin::EC_ERR);
}

QObject *newTestIsdsLogin(void)
{
	return new (::std::nothrow) TestIsdsLogin();
}

//QTEST_MAIN(TestIsdsLogin)
#include "test_isds_login.moc"
