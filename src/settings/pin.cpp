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

#include "src/settings/pin.h"

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
