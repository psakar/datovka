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

#pragma once

#include <QCollator>
#include <QLocale>
#include <QString>

/*!
 * @brief Encapsulates localisation specific settings.
 */
class Localisation {

private:
	/*!
	 * @Brief Private constructor.
	 */
	Localisation(void);

public:
	/*!
	 * @brief Sets program locale according to supplied language code.
	 *
	 * @param[in] langCode Language code.
	 */
	static
	void setProgramLocale(const QString &langCode);

	/*!
	 * @brief Returns short language code (eg. "cs", "en").
	 *
	 * @param[in] langCode Language code.
	 * @return Short language ode or language code used by system if
	 *     \a langCode is unknown.
	 */
	static
	QString shortLangName(const QString &langCode);

	static
	QLocale programLocale; /*!< Global locale instance. */
	static
	QCollator stringCollator; /*!< Used for localised string collation. */

	/*
	 * Using QString instead of char* causes problems with initialisation
	 * order, where the instances may be used before actually being created.
	 */
	static
	const char *langCs; /*!< Czech language code. */
	static
	const char *langEn; /*!< English language code. */
	static
	const char *langSystem; /*!< System-set language. */
};
