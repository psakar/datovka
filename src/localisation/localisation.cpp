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

#include "src/localisation/localisation.h"

QLocale Localisation::programLocale;
QCollator Localisation::stringCollator;

const QString Localisation::langCs(QStringLiteral("cs"));
const QString Localisation::langEn(QStringLiteral("en"));
const QString Localisation::langSystem(QStringLiteral("system"));

void Localisation::setProgramLocale(const QString &langCode)
{
	if (langCode == langCs) {
		programLocale = QLocale(QLocale::Czech, QLocale::CzechRepublic);
	} else if (langCode == langEn) {
		programLocale = QLocale(QLocale::English, QLocale::UnitedKingdom);
	} else {
		/* Use system locale. */
		programLocale = QLocale::system();
	}

	stringCollator.setLocale(programLocale);
}

QString Localisation::shortLangName(const QString &langCode)
{
	if (langCode == langCs) {
		return langCs;
	} else if (langCode == langEn) {
		return langEn;
	} else {
		/* Use system locale. */
		return QLocale::system().name();
	}
}
