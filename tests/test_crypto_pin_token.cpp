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
#include "src/datovka_shared/settings/records_management.h"
#include "src/global.h"
#include "src/log/log.h"
#include "tests/test_crypto_pin_token.h"

class TestCryptoPinToken : public QObject {
	Q_OBJECT

public:
	TestCryptoPinToken(void);

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
	    const QString &pinVal);

	static
	void loadSettings(QSettings &settings, PinSettings &pinSet,
	    RecordsManagementSettings &rmSet);

	static
	void saveSettings(QSettings &settings, const PinSettings &pinSet,
	    const RecordsManagementSettings &rmSet);

	const QString configPinDisabledPath;
	const QString configPinEnabledPath;
	const QString correctPin;
	const QString incorrectPin;
	const QString expectedUrl;
	const QString expectedToken;
};

TestCryptoPinToken::TestCryptoPinToken(void)
    : QObject(Q_NULLPTR),
    configPinDisabledPath("data/config_pin_token_disabled.conf"),
    configPinEnabledPath("data/config_pin_token_enabled.conf"),
    correctPin("1234"),
    incorrectPin("123"),
    expectedUrl("https://records-management.service.cz/api/"),
    expectedToken("jfsah40akv,za[2=40ta;fa32-=gfakg/;as-iqtgfag\\ak-4itagmaga4ag4jgaI)_#@FJWLSAB)*#HRFAS>Ckjha3;faj-354ga6333agfa4")
{
}

void TestCryptoPinToken::initTestCase(void)
{
	QVERIFY(GlobInstcs::logPtr == Q_NULLPTR);
	GlobInstcs::logPtr = new (std::nothrow) LogDevice;
	QVERIFY(GlobInstcs::logPtr != Q_NULLPTR);
}

void TestCryptoPinToken::cleanupTestCase(void)
{
	delete GlobInstcs::logPtr; GlobInstcs::logPtr = Q_NULLPTR;
}

void TestCryptoPinToken::loadConfigPinDisabled(void)
{
	QSettings settings(configPinDisabledPath, QSettings::IniFormat);

	checkConfigPinDisabled(settings);
}

void TestCryptoPinToken::loadConfigPinEnabled(void)
{
	QSettings settings(configPinEnabledPath, QSettings::IniFormat);

	checkConfigPinEnabled(settings);
}

void TestCryptoPinToken::enablePin(void)
{
	QSettings settings01(configPinDisabledPath, QSettings::IniFormat);
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings settings02;

	checkConfigPinDisabled(settings01);

	setPin(settings01, settings02, correctPin);

	checkConfigPinEnabled(settings02);
}

void TestCryptoPinToken::disablePin(void)
{
	QSettings settings01(configPinEnabledPath, QSettings::IniFormat);
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings settings02;

	checkConfigPinEnabled(settings01);

	clearPin(settings01, settings02, correctPin);

	checkConfigPinDisabled(settings02);
}

void TestCryptoPinToken::checkConfigPinDisabled(QSettings &settings) const
{
	PinSettings pinSet;
	RecordsManagementSettings rmSet;

	loadSettings(settings, pinSet, rmSet);

	QVERIFY2(!pinSet.pinConfigured(), "Expected PIN not to be configured.");

	QVERIFY(rmSet.url() == expectedUrl);

	QVERIFY(rmSet.token() == expectedToken);
	QVERIFY(rmSet.tokenAlg().isEmpty());
	QVERIFY(rmSet.tokenSalt().isEmpty());
	QVERIFY(rmSet.tokenIv().isEmpty());
	QVERIFY(rmSet.tokenCode().isEmpty());
}

void TestCryptoPinToken::checkConfigPinEnabled(QSettings &settings) const
{
	PinSettings pinSet;
	RecordsManagementSettings rmSet;

	loadSettings(settings, pinSet, rmSet);

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

	QVERIFY(rmSet.url() == expectedUrl);

	QVERIFY(rmSet.token().isEmpty());
	QVERIFY(!rmSet.tokenAlg().isEmpty());
	QVERIFY(!rmSet.tokenSalt().isEmpty());
	QVERIFY(!rmSet.tokenIv().isEmpty());
	QVERIFY(!rmSet.tokenCode().isEmpty());

	rmSet.decryptToken(QString());
	QVERIFY(rmSet.token().isEmpty());

	rmSet.decryptToken(incorrectPin);
	QVERIFY(!rmSet.token().isEmpty());
	QVERIFY(rmSet.token() != expectedToken);

	rmSet.decryptToken(correctPin);
	QVERIFY(!rmSet.token().isEmpty());
	/* Password already stored in decrypted form. */
	QVERIFY(rmSet.token() != expectedToken);

	rmSet.setToken(QString());
	rmSet.decryptToken(correctPin);
	QVERIFY(!rmSet.token().isEmpty());
	QVERIFY(rmSet.token() == expectedToken);
}

void TestCryptoPinToken::setPin(QSettings &setIn, QSettings &setOut,
    const QString &pinVal)
{
	PinSettings pinSet;
	RecordsManagementSettings rmSet;

	loadSettings(setIn, pinSet, rmSet);

	PinSettings::updatePinSettings(pinSet, pinVal);

	setOut.clear();
	saveSettings(setOut, pinSet, rmSet);
}

void TestCryptoPinToken::clearPin(QSettings &setIn, QSettings &setOut,
    const QString &pinVal)
{
	PinSettings pinSet;
	RecordsManagementSettings rmSet;

	loadSettings(setIn, pinSet, rmSet);

	PinSettings::updatePinSettings(pinSet, QString());
	/* Decrypt token. */
	rmSet.decryptToken(pinVal);

	setOut.clear();
	saveSettings(setOut, pinSet, rmSet);
}

void TestCryptoPinToken::loadSettings(QSettings &settings, PinSettings &pinSet,
    RecordsManagementSettings &rmSet)
{
	settings.setIniCodec("UTF-8");

	pinSet.loadFromSettings(settings);
	rmSet.loadFromSettings(settings);
}

void TestCryptoPinToken::saveSettings(QSettings &settings,
    const PinSettings &pinSet, const RecordsManagementSettings &rmSet)
{
	settings.setIniCodec("UTF-8");

	pinSet.saveToSettings(settings);
	rmSet.saveToSettings(pinSet._pinVal, settings);
}

QObject *newTestCryptoPinToken(void)
{
	return new (std::nothrow) TestCryptoPinToken();
}

//QTEST_MAIN(TestCryptoPinToken)
#include "test_crypto_pin_token.moc"
