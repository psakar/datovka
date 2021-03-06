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

#include <QSettings>
#include <QString>
#include <QtTest/QtTest>

#include "src/datovka_shared/settings/pin.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/settings/account.h"
#include "src/settings/accounts.h"
#include "tests/test_crypto_pin_pwd.h"

class TestCryptoPinPwd : public QObject {
	Q_OBJECT

public:
	TestCryptoPinPwd(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void loadConfigPinDisabled(void);

	void loadConfigPinEnabled(void);

	void enablePin(void);

	void disablePin(void);

private:
	void checkConfigPinDisabled(QSettings &settings) const;

	void checkConfigPinEnabled(QSettings &settings) const;

	static
	void setPin(QSettings &setIn, QSettings &setOut, const QString &pinVal);

	static
	void clearPin(QSettings &setIn, QSettings &setOut,
	    const QString &username, const QString &pinVal);

	static
	void loadSettings(QSettings &settings, PinSettings &pinSet,
	    AccountsMap &accounts);

	static
	void saveSettings(QSettings &settings, const PinSettings &pinSet,
	    const AccountsMap &accounts);

	const QString configPinDisabledPath;
	const QString configPinEnabledPath;
	const QString correctPin;
	const QString incorrectPin;
	const QString expectedAcntName;
	const QString expectedUsername;
	const QString expectedPwd;
};

TestCryptoPinPwd::TestCryptoPinPwd(void)
    : QObject(Q_NULLPTR),
    configPinDisabledPath("data/config_pin_pwd_disabled.conf"),
    configPinEnabledPath("data/config_pin_pwd_enabled.conf"),
    correctPin("1234"),
    incorrectPin("123"),
    expectedAcntName("account_abcdef"),
    expectedUsername("abcdef"),
    expectedPwd("password01")
{
}

void TestCryptoPinPwd::initTestCase(void)
{
	QVERIFY(GlobInstcs::logPtr == Q_NULLPTR);
	GlobInstcs::logPtr = new (std::nothrow) LogDevice;
	QVERIFY(GlobInstcs::logPtr != Q_NULLPTR);
}

void TestCryptoPinPwd::cleanupTestCase(void)
{
	delete GlobInstcs::logPtr; GlobInstcs::logPtr = Q_NULLPTR;
}

void TestCryptoPinPwd::loadConfigPinDisabled(void)
{
	QSettings settings(configPinDisabledPath, QSettings::IniFormat);

	checkConfigPinDisabled(settings);
}

void TestCryptoPinPwd::loadConfigPinEnabled(void)
{
	QSettings settings(configPinEnabledPath, QSettings::IniFormat);

	checkConfigPinEnabled(settings);
}

void TestCryptoPinPwd::enablePin(void)
{
	QSettings settings01(configPinDisabledPath, QSettings::IniFormat);
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings settings02;

	checkConfigPinDisabled(settings01);

	setPin(settings01, settings02, correctPin);

	checkConfigPinEnabled(settings02);
}

void TestCryptoPinPwd::disablePin(void)
{
	QSettings settings01(configPinEnabledPath, QSettings::IniFormat);
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings settings02;

	checkConfigPinEnabled(settings01);

	clearPin(settings01, settings02, expectedUsername, correctPin);

	checkConfigPinDisabled(settings02);
}

void TestCryptoPinPwd::checkConfigPinDisabled(QSettings &settings) const
{
	PinSettings pinSet;
	AccountsMap accounts;

	loadSettings(settings, pinSet, accounts);

	QVERIFY2(!pinSet.pinConfigured(), "Expected PIN not to be configured.");

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

void TestCryptoPinPwd::checkConfigPinEnabled(QSettings &settings) const
{
	PinSettings pinSet;
	AccountsMap accounts;

	loadSettings(settings, pinSet, accounts);

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

void TestCryptoPinPwd::setPin(QSettings &setIn, QSettings &setOut,
    const QString &pinVal)
{
	PinSettings pinSet;
	AccountsMap accounts;

	loadSettings(setIn, pinSet, accounts);

	PinSettings::updatePinSettings(pinSet, pinVal);

	setOut.clear();
	saveSettings(setOut, pinSet, accounts);
}

void TestCryptoPinPwd::clearPin(QSettings &setIn, QSettings &setOut,
    const QString &username, const QString &pinVal)
{
	PinSettings pinSet;
	AccountsMap accounts;

	loadSettings(setIn, pinSet, accounts);

	PinSettings::updatePinSettings(pinSet, QString());
	/* Decrypt password. */
	AcntSettings acntSet(accounts[username]);
	acntSet.decryptPassword(pinVal);
	accounts[username] = acntSet;

	setOut.clear();
	saveSettings(setOut, pinSet, accounts);
}

void TestCryptoPinPwd::loadSettings(QSettings &settings, PinSettings &pinSet,
    AccountsMap &accounts)
{
	settings.setIniCodec("UTF-8");

	pinSet.loadFromSettings(settings);
	accounts.loadFromSettings(QString(), settings);
}

void TestCryptoPinPwd::saveSettings(QSettings &settings, const PinSettings &pinSet,
    const AccountsMap &accounts)
{
	settings.setIniCodec("UTF-8");

	pinSet.saveToSettings(settings);

	QString groupName;
	int row = 0;

	foreach (const QString &key, accounts.keys()) {
		groupName = CredNames::creds;
		if (row > 0) {
			groupName.append(QString::number(row + 1));
		}

		accounts[key].saveToSettings(pinSet._pinVal, QString(),
		    settings, groupName);

		++row;
	}
}

QObject *newTestCryptoPinPwd(void)
{
	return new (std::nothrow) TestCryptoPinPwd();
}

//QTEST_MAIN(TestCryptoPinPwd)
#include "test_crypto_pin_pwd.moc"
