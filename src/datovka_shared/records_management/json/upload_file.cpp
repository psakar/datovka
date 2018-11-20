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
#include <QJsonObject>
#include <QJsonValue>

#include "src/datovka_shared/records_management/json/entry_error.h"
#include "src/datovka_shared/records_management/json/helper.h"
#include "src/datovka_shared/records_management/json/upload_file.h"

static
const QString keyIds("ids");
static
const QString keyFileName("file_name");
static
const QString keyFileContent("file_content");

static
const QString keyId("id");
static
const QString keyError("error");
static
const QString keyLocations("locations");

RecMgmt::UploadFileReq::UploadFileReq(void)
    : m_ids(),
    m_fileName(),
    m_fileContent()
{
}

RecMgmt::UploadFileReq::UploadFileReq(const QStringList &ids,
    const QString &fileName, const QByteArray &fileContent)
    : m_ids(ids),
    m_fileName(fileName),
    m_fileContent(fileContent)
{
}

RecMgmt::UploadFileReq::UploadFileReq(const UploadFileReq &other)
    : m_ids(other.m_ids),
    m_fileName(other.m_fileName),
    m_fileContent(other.m_fileContent)
{
}

const QStringList &RecMgmt::UploadFileReq::ids(void) const
{
	return m_ids;
}

const QString &RecMgmt::UploadFileReq::fileName(void) const
{
	return m_fileName;
}

const QByteArray &RecMgmt::UploadFileReq::fileContent(void) const
{
	return m_fileContent;
}

bool RecMgmt::UploadFileReq::isValid(void) const
{
	bool valid = !m_ids.isEmpty() && !m_fileName.isEmpty() &&
	    !m_fileContent.isEmpty();
	if (!valid) {
		return false;
	}

	foreach (const QString &id, m_ids) {
		if (id.isEmpty()) {
			return false;
		}
	}

	return true;
}

RecMgmt::UploadFileReq RecMgmt::UploadFileReq::fromJson(const QByteArray &json,
    bool *ok)
{
	QJsonObject jsonObj;
	if (!JsonHelper::readRootObject(json, jsonObj)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return UploadFileReq();
	}

	UploadFileReq ufr;

	if (!JsonHelper::readStringList(jsonObj, keyIds, ufr.m_ids, false)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return UploadFileReq();
	}
	if (!JsonHelper::readString(jsonObj, keyFileName, ufr.m_fileName,
	        false)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return UploadFileReq();
	}

	{
		QString valStr;
		if (!JsonHelper::readString(jsonObj, keyFileContent, valStr,
		        false)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return UploadFileReq();
		}

		ufr.m_fileContent = QByteArray::fromBase64(valStr.toUtf8());
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ufr;
}

QByteArray RecMgmt::UploadFileReq::toJson(void) const
{
	QJsonObject jsonObj;
	jsonObj.insert(keyIds, QJsonArray::fromStringList(m_ids));
	jsonObj.insert(keyFileName, !m_fileName.isNull() ?
	    m_fileName : QJsonValue());
	jsonObj.insert(keyFileContent, !m_fileContent.isNull() ?
	    QString::fromUtf8(m_fileContent.toBase64()) : QJsonValue());

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}

RecMgmt::UploadFileResp::UploadFileResp(void)
    : m_id(),
    m_error(),
    m_locations()
{
}

RecMgmt::UploadFileResp::UploadFileResp(const QString &id,
    const ErrorEntry &error, const QStringList &locations)
    : m_id(id),
    m_error(error),
    m_locations(locations)
{
}

RecMgmt::UploadFileResp::UploadFileResp(const UploadFileResp &other)
    : m_id(other.m_id),
    m_error(other.m_error),
    m_locations(other.m_locations)
{
}

const QString &RecMgmt::UploadFileResp::id(void) const
{
	return m_id;
}

const RecMgmt::ErrorEntry &RecMgmt::UploadFileResp::error(void) const
{
	return m_error;
}

const QStringList &RecMgmt::UploadFileResp::locations(void) const
{
	return m_locations;
}

bool RecMgmt::UploadFileResp::isValid(void) const
{
	return (!m_id.isEmpty() && !m_locations.isEmpty()) ||
	    (m_error.code() != ErrorEntry::ERR_NO_ERROR);
}

RecMgmt::UploadFileResp RecMgmt::UploadFileResp::fromJson(
    const QByteArray &json, bool *ok)
{
	QJsonObject jsonObj;
	if (!JsonHelper::readRootObject(json, jsonObj)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return UploadFileResp();
	}

	UploadFileResp ufr;

	if (!JsonHelper::readString(jsonObj, keyId, ufr.m_id, true)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return UploadFileResp();
	}

	{
		QJsonValue jsonVal;
		if (!JsonHelper::readValue(jsonObj, keyError, jsonVal)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return UploadFileResp();
		}
		if (!ufr.m_error.fromJsonVal(&jsonVal)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return UploadFileResp();
		}
	}

	if (!JsonHelper::readStringList(jsonObj, keyLocations, ufr.m_locations,
	        false)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return UploadFileResp();
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ufr;
}

QByteArray RecMgmt::UploadFileResp::toJson(void) const
{
	QJsonObject jsonObj;
	jsonObj.insert(keyId, !m_id.isNull() ? m_id : QJsonValue());
	QJsonValue jsonVal;
	m_error.toJsonVal(&jsonVal);
	jsonObj.insert(keyError, jsonVal);
	jsonObj.insert(keyLocations, QJsonArray::fromStringList(m_locations));

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}
