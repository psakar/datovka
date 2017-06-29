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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include "src/records_management/conversion.h"
#include "src/records_management/json/helper.h"
#include "src/records_management/json/stored_files.h"

static
const QString keyDmIds("dm_ids");
static
const QString keyDiIds("di_ids");

static
const QString keyDmId("dm_id");
static
const QString keyDiId("di_id");
static
const QString keyLocations("locations");

static
const QString keyDms("dms");
static
const QString keyDis("dis");
static
const QString keyLimit("limit");
static
const QString keyError("error");

StoredFilesReq::StoredFilesReq(void)
    : m_dmIds(),
    m_diIds()
{
}

StoredFilesReq::StoredFilesReq(const QList<qint64> &dmIds,
    const QList<qint64> &diIds)
    : m_dmIds(dmIds),
    m_diIds(diIds)
{
}

StoredFilesReq::StoredFilesReq(const StoredFilesReq &sfr)
    : m_dmIds(sfr.m_dmIds),
    m_diIds(sfr.m_diIds)
{
}

const QList<qint64> &StoredFilesReq::dmIds(void) const
{
	return m_dmIds;
}

const QList<qint64> &StoredFilesReq::diIds(void) const
{
	return m_diIds;
}

bool StoredFilesReq::isValid(void) const
{
	return !m_dmIds.isEmpty() || !m_diIds.isEmpty();
}

StoredFilesReq StoredFilesReq::fromJson(const QByteArray &json, bool *ok)
{
	QJsonObject jsonObj;
	if (!JsonHelper::readRootObject(json, jsonObj)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return StoredFilesReq();
	}

	StoredFilesReq sfr;

	{
		QStringList strList;
		if (!JsonHelper::readStringList(jsonObj, keyDmIds, strList,
		        false)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesReq();
		}
		bool iOk = false;
		sfr.m_dmIds = createIdList(strList, &iOk);
		if (!iOk) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesReq();
		}
	}
	{
		QStringList strList;
		if (!JsonHelper::readStringList(jsonObj, keyDiIds, strList,
		        false)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesReq();
		}
		bool iOk = false;
		sfr.m_diIds = createIdList(strList, &iOk);
		if (!iOk) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesReq();
		}
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return sfr;
}

/*!
 * @brief Convert list of qin64 into list of strings.
 *
 * @param[in] idList List of qint64.
 * @return List of strings.
 */
static
QStringList createStrIdList(const QList<qint64> &idList)
{
	QStringList strList;
	foreach (qint64 id, idList) {
		strList.append(QString::number(id));
	}

	return strList;
}

QByteArray StoredFilesReq::toJson(void) const
{
	QJsonObject jsonObj;
	jsonObj.insert(keyDmIds,
	    QJsonArray::fromStringList(createStrIdList(m_dmIds)));
	jsonObj.insert(keyDiIds,
	    QJsonArray::fromStringList(createStrIdList(m_diIds)));

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}

DmEntry::DmEntry(void)
    : m_dmId(-1),
    m_locations()
{
}

DmEntry::DmEntry(qint64 dmId, const QStringList &locations)
    : m_dmId(dmId),
    m_locations(locations)
{
}

DmEntry::DmEntry(const DmEntry &me)
    : m_dmId(me.m_dmId),
    m_locations(me.m_locations)
{
}

qint64 DmEntry::dmId(void) const
{
	return m_dmId;
}

const QStringList &DmEntry::locations(void) const
{
	return m_locations;
}

bool DmEntry::isValid(void) const
{
	return m_dmId >= 0;
}

/*!
 * @brief Read object content from JSON value.
 *
 * @param[in]  jsnoVal JSON value.
 * @param[in]  idKey Key identifying data message or delivery info identifier.
 * @param[out] id Read id.
 * @param[out] locations Read list of locations.
 * @return True on success, false else.
 */
static
bool fromJsonValue(const QJsonValue *jsonVal, const QString &idKey,
    qint64 &id, QStringList &locations)
{
	if (jsonVal == Q_NULLPTR) {
		Q_ASSERT(0);
		return false;
	}

	if (!jsonVal->isObject()) {
		return false;
	}

	QJsonObject jsonObj(jsonVal->toObject());

	id = -1;
	locations.clear();

	QString idStr;
	if (!JsonHelper::readString(jsonObj, idKey, idStr, false)) {
		return false;
	}
	bool ok = false;
	id = idStr.toLongLong(&ok);
	if (!ok) {
		return false;
	}
	if (id < 0) {
		return false;
	}

	if (!JsonHelper::readStringList(jsonObj, keyLocations, locations,
	        false)) {
		return false;
	}

	return true;
}

bool DmEntry::fromJsonVal(const QJsonValue *jsonVal)
{
	qint64 id = -1;
	QStringList locations;
	if (!fromJsonValue(jsonVal, keyDmId, id, locations)) {
		return false;
	}

	m_dmId = id;
	m_locations = locations;
	return true;
}

/*!
 * @brief Write object to JSON value.
 *
 * @param[out] jsnoVal JSON value.
 * @param[in]  idKey Key identifying data message or delivery info identifier.
 * @param[in]  id Written id.
 * @param[in]  locations Written list of locations.
 * @return True on success, false else.
 */
static
bool toJsonValue(QJsonValue *jsonVal, const QString &idKey,
    qint64 id, const QStringList &locations)
{
	if (jsonVal == Q_NULLPTR) {
		Q_ASSERT(0);
		return false;
	}

	QJsonObject jsonObj;
	jsonObj.insert(idKey, QString::number(id));
	jsonObj.insert(keyLocations, QJsonArray::fromStringList(locations));
	*jsonVal = jsonObj;
	return true;
}

bool DmEntry::toJsonVal(QJsonValue *jsonVal) const
{
	return toJsonValue(jsonVal, keyDmId, m_dmId, m_locations);
}

DiEntry::DiEntry(void)
    : m_diId(-1),
    m_locations()
{
}

DiEntry::DiEntry(qint64 diId, const QStringList &locations)
    : m_diId(diId),
    m_locations(locations)
{
}

DiEntry::DiEntry(const DiEntry &ie)
    : m_diId(ie.m_diId),
    m_locations(ie.m_locations)
{
}

qint64 DiEntry::diId(void) const
{
	return m_diId;
}

const QStringList &DiEntry::locations(void) const
{
	return m_locations;
}

bool DiEntry::isValid(void) const
{
	return m_diId >= 0;
}

bool DiEntry::fromJsonVal(const QJsonValue *jsonVal)
{
	qint64 id = -1;
	QStringList locations;
	if (!fromJsonValue(jsonVal, keyDiId, id, locations)) {
		return false;
	}

	m_diId = id;
	m_locations = locations;
	return true;
}

bool DiEntry::toJsonVal(QJsonValue *jsonVal) const
{
	return toJsonValue(jsonVal, keyDiId, m_diId, m_locations);
}

StoredFilesResp::StoredFilesResp(void)
    : m_dms(),
    m_dis(),
    m_limit(-1),
    m_error()
{
}

StoredFilesResp::StoredFilesResp(const QList<DmEntry> &dms, const QList<DiEntry> &dis,
    int limit, const ErrorEntry &error)
    : m_dms(dms),
    m_dis(dis),
    m_limit(limit),
    m_error(error)
{
}

StoredFilesResp::StoredFilesResp(const StoredFilesResp &sfr)
    : m_dms(sfr.m_dms),
    m_dis(sfr.m_dis),
    m_limit(sfr.m_limit),
    m_error(sfr.m_error)
{
}

const QList<DmEntry> &StoredFilesResp::dms(void) const
{
	return m_dms;
}

const QList<DiEntry> &StoredFilesResp::dis(void) const
{
	return m_dis;
}

int StoredFilesResp::limit(void) const
{
	return m_limit;
}

const ErrorEntry &StoredFilesResp::error(void) const
{
	return m_error;
}

bool StoredFilesResp::isValid(void) const
{
	return (m_limit > 0) &&
	    ((!m_dms.isEmpty() || !m_dis.isEmpty()) ||
	     (m_error.code() != ErrorEntry::ERR_NO_ERROR));
}

/*!
 * @brief Template function that reads list of objects of type T from JSON
 *     array.
 *
 * @param[in]  jsonObj JSON object to read data from.
 * @param[in]  idKey Key identifying the sought array of T.
 * @param[out] list List to append read objects of type T to.
 * @return True on success, false else.
 */
template<class T>
static
bool readArrayofObjects(const QJsonObject &jsonObj, const QString &idKey,
    QList<T> &list)
{
	QJsonValue jsonVal;
	if (!JsonHelper::readValue(jsonObj, idKey, jsonVal)) {
		return false;
	}
	if (!jsonVal.isArray()) {
		return false;
	}
	foreach (const QJsonValue &jsonVal, jsonVal.toArray()) {
		T te; /* Entry of type T. */
		if (!te.fromJsonVal(&jsonVal)) {
			return false;
		}
		list.append(te);
	}
	return true;
}

StoredFilesResp StoredFilesResp::fromJson(const QByteArray &json, bool *ok)
{
	QJsonObject jsonObj;
	if (!JsonHelper::readRootObject(json, jsonObj)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return StoredFilesResp();
	}

	StoredFilesResp sfr;
	if (!readArrayofObjects<DmEntry>(jsonObj, keyDms, sfr.m_dms)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return StoredFilesResp();
	}
	if (!readArrayofObjects<DiEntry>(jsonObj, keyDis, sfr.m_dis)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return StoredFilesResp();
	}
	{
		if (!JsonHelper::readInt(jsonObj, keyLimit, sfr.m_limit,
		        false)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesResp();
		}
		if (sfr.m_limit < 0) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesResp();
		}
	}
	{
		QJsonValue jsonVal;
		if (!JsonHelper::readValue(jsonObj, keyError, jsonVal)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesResp();
		}
		if (!sfr.m_error.fromJsonVal(&jsonVal)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return StoredFilesResp();
		}
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return sfr;
}

QByteArray StoredFilesResp::toJson(void) const
{
	QJsonObject jsonObj;
	{
		QJsonArray dms;
		QJsonValue dmVal;
		foreach (const DmEntry &me, m_dms) {
			me.toJsonVal(&dmVal);
			dms.append(dmVal);
		}
		jsonObj.insert(keyDms, dms);
	}
	{
		QJsonArray dis;
		QJsonValue diVal;
		foreach (const DiEntry &ie, m_dis) {
			ie.toJsonVal(&diVal);
			dis.append(diVal);
		}
		jsonObj.insert(keyDis, dis);
	}
	jsonObj.insert(keyLimit, QString::number(m_limit));
	QJsonValue jsonVal;
	m_error.toJsonVal(&jsonVal);
	jsonObj.insert(keyError, jsonVal);

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}
