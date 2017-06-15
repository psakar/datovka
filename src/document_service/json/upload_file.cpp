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

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "src/document_service/json/entry_error.h"
#include "src/document_service/json/helper.h"
#include "src/document_service/json/upload_file.h"

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

UploadFileReq::UploadFileReq(void)
    : m_ids(),
    m_fileName(),
    m_fileContent()
{
}

UploadFileReq::UploadFileReq(const QStringList &ids, const QString &fileName,
    const QByteArray &fileContent)
    : m_ids(ids),
    m_fileName(fileName),
    m_fileContent(fileContent)
{
}

UploadFileReq::UploadFileReq(const UploadFileReq &ufr)
    : m_ids(ufr.m_ids),
    m_fileName(ufr.m_fileName),
    m_fileContent(ufr.m_fileContent)
{
}

const QStringList &UploadFileReq::ids(void) const
{
	return m_ids;
}

const QString &UploadFileReq::fileName(void) const
{
	return m_fileName;
}

const QByteArray &UploadFileReq::fileContent(void) const
{
	return m_fileContent;
}

bool UploadFileReq::isValid(void) const
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

UploadFileReq UploadFileReq::fromJson(const QByteArray &json, bool *ok)
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

QByteArray UploadFileReq::toJson(void) const
{
	QJsonObject jsonObj;
	jsonObj.insert(keyIds, QJsonArray::fromStringList(m_ids));
	jsonObj.insert(keyFileName, !m_fileName.isNull() ?
	    m_fileName : QJsonValue());
	jsonObj.insert(keyFileContent, !m_fileContent.isNull() ?
	    QString::fromUtf8(m_fileContent.toBase64()) : QJsonValue());

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}

UploadFileReq uploadFileRequest(const QStringList &ids, const QString &filePath)
{
	if (ids.isEmpty()) {
		qCritical("%s", "No identifier provided.");
		return UploadFileReq(QStringList(), QString(), QByteArray());
	}
	foreach (const QString &id, ids) {
		if (id.isEmpty()) {
			qCritical("%s", "Empty identifier provided.");
			return UploadFileReq(QStringList(), QString(),
			    QByteArray());
		}
	}

	if (filePath.isEmpty()) {
		qCritical("%s", "No path to SVG file.");
		return UploadFileReq(QStringList(), QString(), QByteArray());
	}

	QByteArray fileContent;
	{
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly)) {
			qCritical("Cannot open file '%s',",
			    filePath.toUtf8().constData());
			return UploadFileReq(QStringList(), QString(),
			    QByteArray());
		}

		fileContent = file.readAll();

		file.close();
	}

	QString fileName;
	{
		QFileInfo fileInfo(filePath);
		fileName = fileInfo.fileName();
	}

	return UploadFileReq(ids, fileName, fileContent);
}

UploadFileResp::UploadFileResp(void)
    : m_id(),
    m_error(),
    m_locations()
{
}

UploadFileResp::UploadFileResp(const QString &id, const ErrorEntry &error,
    const QStringList &locations)
    : m_id(id),
    m_error(error),
    m_locations(locations)
{
}

UploadFileResp::UploadFileResp(const UploadFileResp &ufr)
    : m_id(ufr.m_id),
    m_error(ufr.m_error),
    m_locations(ufr.m_locations)
{
}

const QString &UploadFileResp::id(void) const
{
	return m_id;
}

const ErrorEntry &UploadFileResp::error(void) const
{
	return m_error;
}

const QStringList &UploadFileResp::locations(void) const
{
	return m_locations;
}

bool UploadFileResp::isValid(void) const
{
	return (!m_id.isEmpty() && !m_locations.isEmpty()) ||
	    (m_error.code() != ErrorEntry::ERR_NO_ERROR);
}

UploadFileResp UploadFileResp::fromJson(const QByteArray &json, bool *ok)
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

QByteArray UploadFileResp::toJson(void) const
{
	QJsonObject jsonObj;
	jsonObj.insert(keyId, !m_id.isNull() ? m_id : QJsonValue());
	QJsonValue jsonVal;
	m_error.toJsonVal(&jsonVal);
	jsonObj.insert(keyError, jsonVal);
	jsonObj.insert(keyLocations, QJsonArray::fromStringList(m_locations));

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}

QByteArray jsonUploadFileResp(void)
{
	QJsonObject jsonObj;
	jsonObj.insert(keyId, QStringLiteral("aabbccddee"));
	jsonObj.insert(keyError, QJsonValue());
	QStringList locationList;
	locationList << "tady" << "a tady" << "a také tady";
	jsonObj.insert(keyLocations, QJsonArray::fromStringList(locationList));

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}
