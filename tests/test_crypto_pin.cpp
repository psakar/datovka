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

#include <QSettings>
#include <QString>
#include <QtTest/QtTest>

#include "src/settings/account.h"
#include "src/settings/accounts.h"
#include "src/settings/pin.h"
#include "tests/test_crypto_pin.h"

class TestCryptoPin : public QObject {
	Q_OBJECT

public:
	TestCryptoPin(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void loadConfigPinDisabled(void);

	void loadConfigPinEnabled(void);

private:
	static
	void loadSettings(const QString &confPath, PinSettings &pinSet,
	    AccountsMap &accounts);

	const QString configPinDisabledPath;
	const QString configPinEnabledPath;
	const QString correctPin;
	const QString incorrectPin;
	const QString expectedAcntName;
	const QString expectedUsername;
	const QString expectedPwd;
};

TestCryptoPin::TestCryptoPin(void)
    : QObject(Q_NULLPTR),
    configPinDisabledPath("data/config_pin_disabled.conf"),
    configPinEnabledPath("data/config_pin_enabled.conf"),
    correctPin("1234"),
    incorrectPin("123"),
    expectedAcntName("account_abcdef"),
    expectedUsername("abcdef"),
    expectedPwd("password01")
{
}

void TestCryptoPin::initTestCase(void)
{
	/* No initialisation is needed. */
}

void TestCryptoPin::cleanupTestCase(void)
{
}

void TestCryptoPin::loadConfigPinDisabled(void)
{
	PinSettings pinSet;
	AccountsMap accounts;

	loadSettings(configPinDisabledPath, pinSet, accounts);

	QVERIFY2(!pinSet.pinConfigured(), "Expected PIN to be not configured.");

	QVERIFY(accounts.find(expectedUsername) != accounts.end());

	AcntSettings acntSet(accounts[expectedUsername]);
	QVERIFY(acntSet.accountName() == expectedAcntName);
	QVERIFY(acntSet.userName() == expectedUsername);

	QVERIFY(acntSet.password() == expectedPwd);
	QVERIFY(acntSet.pwdAlg().isEmpty());
	QVERIFY(acntSet.pwdSalt().isEmpty());
	QVERIFY(acntSet.pwdIv().isEmpty());
	QVERIFY(acntSet.pwdCode().isEmpty());
}

void TestCryptoPin::loadConfigPinEnabled(void)
{
	PinSettings pinSet;
	AccountsMap accounts;

	loadSettings(configPinEnabledPath, pinSet, accounts);

	QVERIFY2(pinSet.pinConfigured(), "Expected PIN to be configured.");

	bool ret = PinSettings::verifyPin(pinSet, QString());
	QVERIFY2(!ret, "Expected PIN check to fail.");
	QVERIFY(pinSet._pinVal.isEmpty());

	ret = PinSettings::verifyPin(pinSet, incorrectPin);
	QVERIFY2(!ret, "Expected PIN check to fail.");
	QVERIFY(pinSet._pinVal.isEmpty());

	ret = PinSettings::verifyPin(pinSet, correctPin);
	QVERIFY2(ret, "Expected PIN check to succeed.");
	QVERIFY(!pinSet._pinVal.isEmpty());
	QVERIFY(pinSet._pinVal == correctPin);

	QVERIFY(accounts.find(expectedUsername) != accounts.end());

	AcntSettings acntSet(accounts[expectedUsername]);
	QVERIFY(acntSet.accountName() == expectedAcntName);
	QVERIFY(acntSet.userName() == expectedUsername);

	QVERIFY(acntSet.password().isEmpty());
	QVERIFY(!acntSet.pwdAlg().isEmpty());
	QVERIFY(!acntSet.pwdSalt().isEmpty());
	QVERIFY(!acntSet.pwdIv().isEmpty());
	QVERIFY(!acntSet.pwdCode().isEmpty());

	acntSet.decryptPassword(QString());
	QVERIFY(acntSet.password().isEmpty());

	acntSet.decryptPassword(incorrectPin);
	QVERIFY(!acntSet.password().isEmpty());
	QVERIFY(acntSet.password() != expectedPwd);

	acntSet.decryptPassword(correctPin);
	QVERIFY(!acntSet.password().isEmpty());
	/* Password already stored in decrypted form. */
	QVERIFY(acntSet.password() != expectedPwd);

	acntSet.setPassword(QString());
	acntSet.decryptPassword(correctPin);
	QVERIFY(!acntSet.password().isEmpty());
	QVERIFY(acntSet.password() == expectedPwd);
}

void TestCryptoPin::loadSettings(const QString &confPath, PinSettings &pinSet,
    AccountsMap &accounts)
{
	QSettings settings(confPath, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	pinSet.loadFromSettings(settings);
	accounts.loadFromSettings(QString(), settings);
}

QObject *newTestCryptoPin(void)
{
	return new (std::nothrow) TestCryptoPin();
}

//QTEST_MAIN(TestCryptoPin)
#include "test_crypto_pin.moc"
