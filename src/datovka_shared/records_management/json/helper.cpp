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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>

#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/records_management/json/helper.h"

bool JsonHelper::readRootObject(const QByteArray &json, QJsonObject &jsonObj)
{
	QJsonDocument jsonDoc;
	{
		QJsonParseError parseErr;
		jsonDoc = QJsonDocument::fromJson(json, &parseErr);
		if (jsonDoc.isNull()) {
			logErrorNL("Error parsing JSON: %s",
			    parseErr.errorString().toUtf8().constData());
			return false;
		}
	}
	if (!jsonDoc.isObject()) {
		logErrorNL("%s", "JSON document contains no object.");
		return false;
	}

	QJsonObject jsonTmpObj(jsonDoc.object());
	if (jsonTmpObj.isEmpty()) {
		logErrorNL("%s", "JSON object is empty.");
		return false;
	}

	jsonObj = jsonTmpObj;
	return true;
}

bool JsonHelper::readValue(const QJsonObject &jsonObj, const QString &key,
    QJsonValue &jsonVal)
{
	if (jsonObj.isEmpty() || key.isEmpty()) {
		logErrorNL("%s", "JSON object or sought key is empty.");
		return false;
	}

	jsonVal = jsonObj.value(key);
	if (jsonVal.isUndefined()) {
		logErrorNL("Missing key '%s' in JSON object.",
		    key.toUtf8().constData());
		return false;
	}

	return true;
}

bool JsonHelper::readInt(const QJsonObject &jsonObj, const QString &key,
    int &val, bool acceptNull)
{
	QJsonValue jsonVal;
	if (!readValue(jsonObj, key, jsonVal)) {
		return false;
	}
	if (jsonVal.isNull()) {
		val = 0; /* Null value. */
		return acceptNull;
	}
	int readVal = jsonVal.toInt(-1);
	if (readVal == -1) {
		logErrorNL("Value related to key '%s' is not an integer.",
		    key.toUtf8().constData());
		return false;
	}

	val = readVal;
	return true;
}

bool JsonHelper::readString(const QJsonObject &jsonObj, const QString &key,
    QString &val, bool acceptNull)
{
	QJsonValue jsonVal;
	if (!readValue(jsonObj, key, jsonVal)) {
		return false;
	}
	if (jsonVal.isNull()) {
		val = QString(); /* Null string. */
		return acceptNull;
	}
	if (!jsonVal.isString()) {
		logErrorNL("Value related to key '%s' is not a string.",
		    key.toUtf8().constData());
		return false;
	}

	val = jsonVal.toString();
	return true;
}

bool JsonHelper::readArray(const QJsonObject &jsonObj, const QString &key,
    QJsonArray &arr, bool acceptNull)
{
	QJsonValue jsonVal;
	if (!readValue(jsonObj, key, jsonVal)) {
		return false;
	}
	if (jsonVal.isNull()) {
		arr = QJsonArray(); /* Null array. */
		return acceptNull;
	}
	if (!jsonVal.isArray()) {
		logErrorNL("Value related to key '%s' is not an array.",
		    key.toUtf8().constData());
		return false;
	}

	arr = jsonVal.toArray();
	return true;
}

bool JsonHelper::readStringList(const QJsonObject &jsonObj, const QString &key,
    QStringList &val, bool acceptNull)
{
	QJsonArray jsonArr;
	if (!readArray(jsonObj, key, jsonArr, acceptNull)) {
		return false;
	}

	QStringList tmpList;

	foreach (const QJsonValue &jsonVal, jsonArr) {
		if (jsonVal.isNull()) {
			logErrorNL("%s", "Found null value in array.");
			return false;
		}
		if (!jsonVal.isString()) {
			logErrorNL("%s", "Found non-string value in array.");
			return false;
		}

		tmpList.append(jsonVal.toString());
	}

	val = tmpList;
	return true;
}

QString JsonHelper::toIndentedString(const QByteArray &json)
{
	if (json.isEmpty()) {
		return QString();
	}

	QJsonDocument jsonDoc;
	{
		QJsonParseError parseErr;
		jsonDoc = QJsonDocument::fromJson(json, &parseErr);
		if (jsonDoc.isNull()) {
			logErrorNL("Error parsing JSON: %s",
			    parseErr.errorString().toUtf8().constData());
			return QString();
		}
	}

	return QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Indented));
}
