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

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

/*!
 * @brief JSON conversion helper functions.
 */
class JsonHelper {
private:
	/*!
	 * @brief Private constructor.
	 */
	JsonHelper(void);

public:
	/*!
	 * @brief Reads a JSON object which comprises the document.
	 *
	 * @param[in]  json JSON data.
	 * @param[out] jsonObj JSON object from the document.
	 * @return True on success, false else.
	 */
	static
	bool readRootObject(const QByteArray &json, QJsonObject &jsonObj);

	/*!
	 * @brief Searches for a value on JSON object.
	 *
	 * @param[in]  jsonObject Object to search in.
	 * @param[in]  key Key to search for.
	 * @param[out] jsonVal Found value.
	 * @return True if key found, false else.
	 */
	static
	bool readValue(const QJsonObject &jsonObj, const QString &key,
	    QJsonValue &jsonVal);

	/*!
	 * @brief Reads an integer value from supplied JSON object.
	 *
	 * @param[in]  jsonObj JSON object.
	 * @param[in]  key Key identifying the string.
	 * @param[out] val Value to be stored.
	 * @param[in]  acceptNull True if null value should also be accepted.
	 * @return True on success, false else.
	 */
	static
	bool readInt(const QJsonObject &jsonObj, const QString &key,
	    int &val, bool acceptNull);

	/*!
	 * @brief Reads a string value from supplied JSON object.
	 *
	 * @param[in]  jsonObj JSON object.
	 * @param[in]  key Key identifying the string.
	 * @param[out] val Value to be stored.
	 * @param[in]  acceptNull True if null value should also be accepted.
	 * @return True on success, false else.
	 */
	static
	bool readString(const QJsonObject &jsonObj, const QString &key,
	    QString &val, bool acceptNull);

	/*!
	 * @brief Reads an arry value from supplied JSON object.
	 *
	 * @param[in]  jsonObj JSON object.
	 * @param[in]  key Key identifying the string.
	 * @param[out] arr Array to be stored.
	 * @param[in]  acceptNull True if null value should also be accepted.
	 * @return True on success, false else.
	 */
	static
	bool readArray(const QJsonObject &jsonObj, const QString &key,
	    QJsonArray &arr, bool acceptNull);

	/*!
	 * @brief Reads a string list from supplied JSON object.
	 *
	 * @param[in]  jsonObj JSON object.
	 * @param[in]  key Key identifying the string list.
	 * @param[out] val Values to be stored.
	 * @param[in]  acceptNull True if null value should also be accepted.
	 * @return True on success, false else.
	 */
	static
	bool readStringList(const QJsonObject &jsonObj, const QString &key,
	    QStringList &val, bool acceptNull);

	/*!
	 * @brief Converts JSON document to indented string.
	 *
	 * @param[in] json JSON data.
	 * @retunr Non-empty indented string if JSON data could be read.
	 */
	static
	QString toIndentedString(const QByteArray &json);
};
