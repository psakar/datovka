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

#include "src/datovka_shared/crypto/crypto_pin.h"
#include "src/datovka_shared/crypto/crypto_wrapped.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/settings/pin.h"

#define SETTINGS_SECURITY_GROUP "security"

//#define SETTINGS_PIN_VAL "pin_val" // Don't store readable PIN value into settings.
#define SETTINGS_PIN_ALG "pin_alg"
#define SETTINGS_PIN_SALT "pin_salt"
#define SETTINGS_PIN_CODE "pin_code"

PinSettings::PinSettings(void)
    : _pinVal(),
    pinAlg(),
    pinSalt(),
    pinCode()
{
}

void PinSettings::loadFromSettings(const QSettings &settings)
{
	//_pinVal = QString::fromUtf8(QByteArray::fromBase64(
	//    settings.value(SETTINGS_SECURITY_GROUP "/" SETTINGS_PIN_VAL,
	//        QString()).toString().toUtf8()));

	pinAlg = settings.value(SETTINGS_SECURITY_GROUP "/" SETTINGS_PIN_ALG,
	    QString()).toString();

	pinSalt = QByteArray::fromBase64(settings.value(
	    SETTINGS_SECURITY_GROUP "/" SETTINGS_PIN_SALT,
	    QString()).toString().toUtf8());

	pinCode = QByteArray::fromBase64(settings.value(
	    SETTINGS_SECURITY_GROUP "/" SETTINGS_PIN_CODE,
	    QString()).toString().toUtf8());
}

void PinSettings::saveToSettings(QSettings &settings) const
{
	settings.beginGroup(SETTINGS_SECURITY_GROUP);

	if (!_pinVal.isEmpty()) {
		//settings.setValue(SETTINGS_PIN_VAL,
		//    QString::fromUtf8(_pinVal.toUtf8().toBase64()));

		Q_ASSERT(!pinAlg.isEmpty());
		settings.setValue(SETTINGS_PIN_ALG, pinAlg);

		Q_ASSERT(!pinSalt.isEmpty());
		settings.setValue(SETTINGS_PIN_SALT,
		    QString::fromUtf8(pinSalt.toBase64()));

		Q_ASSERT(!pinCode.isEmpty());
		settings.setValue(SETTINGS_PIN_CODE,
		    QString::fromUtf8(pinCode.toBase64()));
	}

	settings.endGroup();
}

bool PinSettings::pinConfigured(void) const
{
	return !pinAlg.isEmpty() && !pinSalt.isEmpty() && !pinCode.isEmpty();
}

void PinSettings::updatePinSettings(PinSettings &sett, const QString &pinValue)
{
	QByteArray pinSalt;
	QByteArray pinCode;

	if (!pinValue.isEmpty()) {
		/* Generate random salt. */
		pinSalt = randomSalt(pbkdf2_sha256.out_len);

		pinCode = computePinCode(pinValue, pbkdf2_sha256.name, pinSalt,
		    pbkdf2_sha256.out_len);
	}

	if (!pinCode.isEmpty()) {
		sett._pinVal = pinValue;
		sett.pinAlg = pbkdf2_sha256.name;
		sett.pinSalt = pinSalt;
		sett.pinCode = pinCode;
	} else {
		sett._pinVal.clear();
		sett.pinAlg.clear();
		sett.pinSalt.clear();
		sett.pinCode.clear();
	}
}

bool PinSettings::verifyPin(PinSettings &sett, const QString &pinValue)
{
	if (sett.pinAlg.isEmpty() || sett.pinSalt.isEmpty() ||
	    sett.pinCode.isEmpty()) {
		logErrorNL("%s",
		    "PIN algorithm, PIN salt or encoded PIN are missing.");
		return false;
	}

	bool verResult = sett.pinCode == computePinCode(pinValue,
	    sett.pinAlg, sett.pinSalt, pbkdf2_sha256.out_len);
	if (verResult) {
		/* Remember entered correct PIN. */
		sett._pinVal = pinValue;
		logDebugLv0NL("%s", "Remembering entered PIN value.");
	}
	return verResult;
}
