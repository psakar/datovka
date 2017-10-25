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

class TestCryptoPin : public QObject {
	Q_OBJECT

public:
	TestCryptoPin(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

private:
//	static
//	void loadSettings(const QString &confPath, PinSettings &pinSet,
//	    AccountsMap &accounts);

	const QString configPinDisabledPath;
	const QString configPinEnabledPath;
};

TestCryptoPin::TestCryptoPin(void)
    : QObject(Q_NULLPTR),
    configPinDisabledPath("data/config_pin_disabled.conf"),
    configPinEnabledPath("data/config_pin_enabled.conf")
{
}

void TestCryptoPin::initTestCase(void)
{
	/* No initialisation is needed. */
}

void TestCryptoPin::cleanupTestCase(void)
{
}

QObject *newTestCryptoPin(void)
{
	return new (std::nothrow) TestCryptoPin();
}

//QTEST_MAIN(TestCryptoPin)
#include "test_crypto_pin.moc"
