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

#ifndef _STORED_FILES_H_
#define _STORED_FILES_H_

#include <QByteArray>
#include <QList>
#include <QStringList>

#include "src/document_service/json/entry_error.h"

class QJsonValue; /* Forward declaration. */

/*!
 * @brief Encapsulates the stored_files request.
 */
class StoredFilesReq {
private:
	/*!
	 * @brief Constructor. Creates an invalid structure.
	 */
	StoredFilesReq(void);

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] dmIds Data message identifiers.
	 * @param[in] diIds Delivery info identifiers.
	 */
	StoredFilesReq(const QList<qint64> &dmIds, const QList<qint64> &diIds);

	/*!
	 * @brief Copy constructor.
	 *
	 * @param[in] sfr Stored files request.
	 */
	StoredFilesReq(const StoredFilesReq &sfr);

	/*!
	 * @brief Return data message identifiers.
	 *
	 * @return Data message identifiers as used in ISDS.
	 */
	const QList<qint64> &dmIds(void) const;

	/*!
	 * @brief Return delivery info identifiers.
	 *
	 * @return Delivery info identifiers as used in ISDS.
	 */
	const QList<qint64> &diIds(void) const;

	/*!
	 * @brief Check whether content is valid.
	 *
	 * @return True if content is valid.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Creates a stored files request structure from supplied JSON
	 *     document.
	 *
	 * @param[in]  json JSON document.
	 * @param[out] ok Set to true on success.
	 * @return Invalid structure on error a valid structure else.
	 */
	static
	StoredFilesReq fromJson(const QByteArray &json, bool *ok = Q_NULLPTR);

	/*!
	 * @brief Converts stored files structure into a JSON document.
	 *
	 * @note Unspecified values are stores as null into the JSON document.
	 *
	 * @return JSON document containing stored data.
	 */
	QByteArray toJson(void) const;

private:
	QList<qint64> m_dmIds; /*!< Data message identifiers. */
	QList<qint64> m_diIds; /*!< Delivery info identifiers. */
};

/*!
 * @brief Encapsulates stored_files data message entry structure.
 */
class DmEntry {
public:
	/*!
	 * @brief Constructor. Constructs invalid entry.
	 */
	DmEntry(void);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] dmId Message identifier.
	 * @param[in] locations List of locations.
	 */
	DmEntry(qint64 dmId, const QStringList &locations);

	/*!
	 * @brief Copy constructor.
	 *
	 * @param[in] me Message entry.
	 */
	DmEntry(const DmEntry &me);

	/*!
	 * @brief Return message identifier.
	 *
	 * @return Data message identifier.
	 */
	qint64 dmId(void) const;

	/*!
	 * @brief Return list of locations.
	 *
	 * @return Locations.
	 */
	const QStringList &locations(void) const;

	/*!
	 * @brief Check whether content is valid.
	 *
	 * @return True if content is valid.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Set content according to JSON value.
	 *
	 * @note Method does not modify value of error entry if false returned.
	 *     JSON null values are converted into no-error entries.
	 *
	 * @param[in] jsonVal Value to read from.
	 * @return True on success, false else.
	 */
	bool fromJsonVal(const QJsonValue *jsonVal);

	/*!
	 * @brief Set content of supplied JSON value.
	 *
	 * @note No-error entries are converted into JSON null values.
	 *
	 * @param[out] jsonVal Value to store to.
	 * @return True on success, false else.
	 */
	bool toJsonVal(QJsonValue *jsonVal) const;

private:
	qint64 m_dmId; /*!< Data message identifier. */
	QStringList m_locations; /*!< Where the uploaded file is located in the service. */
};

/*!
 * @brief Encapsulates stored_files delivery info entry structure.
 */
class DiEntry {
public:
	/*!
	 * @brief Constructor. Constructs invalid entry.
	 */
	DiEntry(void);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] diId Info identifier.
	 * @param[in] locations List of locations.
	 */
	DiEntry(qint64 diId, const QStringList &locations);

	/*!
	 * @brief Copy constructor.
	 *
	 * @param[in] ie Info entry.
	 */
	DiEntry(const DiEntry &ie);

	/*!
	 * @brief Return info identifier.
	 *
	 * @return Data info identifier.
	 */
	qint64 diId(void) const;

	/*!
	 * @brief Return list of locations.
	 *
	 * @return Locations.
	 */
	const QStringList &locations(void) const;

	/*!
	 * @brief Check whether content is valid.
	 *
	 * @return True if content is valid.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Set content according to JSON value.
	 *
	 * @note Method does not modify value of error entry if false returned.
	 *     JSON null values are converted into no-error entries.
	 *
	 * @param[in] jsonVal Value to read from.
	 * @return True on success, false else.
	 */
	bool fromJsonVal(const QJsonValue *jsonVal);

	/*!
	 * @brief Set content of supplied JSON value.
	 *
	 * @note No-error entries are converted into JSON null values.
	 *
	 * @param[out] jsonVal Value to store to.
	 * @return True on success, false else.
	 */
	bool toJsonVal(QJsonValue *jsonVal) const;

private:
	qint64 m_diId; /*!< Delivery info identifier. */
	QStringList m_locations; /*!< Where the uploaded file is located in the service. */
};

/*!
 * @brief Encapsulates the stored_files response.
 */
class StoredFilesResp {
private:
	/*!
	 * @brief Constructor. Creates an invalid structure.
	 */
	StoredFilesResp(void);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] dms List of data message entries.
	 * @param[in] dis List of delivery information entries.
	 * @param[in] limit Request limit.
	 * @param[in] error Error entry.
	 */
	StoredFilesResp(const QList<DmEntry> &dms, const QList<DiEntry> &dis,
	    int limit, const ErrorEntry &error);

public:
	/*!
	 * @brief Copy constructor.
	 *
	 * @param[in] sfr Stored files response.
	 */
	StoredFilesResp(const StoredFilesResp &sfr);

	/*!
	 * @brief Return list of data message entries.
	 *
	 * @return List of held data message entries.
	 */
	const QList<DmEntry> &dms(void) const;

	/*!
	 * @brief Return list of delivery info entries.
	 *
	 * @return List of held delivery info entries.
	 */
	const QList<DiEntry> &dis(void) const;

	/*!
	 * @brief Return request limit.
	 *
	 * @return Request limit as obtained from the service.
	 */
	int limit(void) const;

	/*!
	 * @brief Return error entry.
	 *
	 * @return Error entry.
	 */
	const ErrorEntry &error(void) const;

	/*!
	 * @brief Check whether content is valid.
	 *
	 * @return True if content is valid.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Creates a stored files response structure from supplied JSON
	 *     document.
	 *
	 * @param[in]  json JSON document.
	 * @param[out] ok Set to true on success.
	 * @return Invalid structure on error a valid structure else.
	 */
	static
	StoredFilesResp fromJson(const QByteArray &json,
	    bool *ok = Q_NULLPTR);

	/*!
	 * @brief Converts stored files response structure into a JSON document.
	 *
	 * @note Unspecified values are stores as null into the JSON document.
	 *
	 * @return JSON document containing stored data.
	 */
	QByteArray toJson(void) const;

private:
	QList<DmEntry> m_dms; /*!< List of received data message entries. */
	QList<DiEntry> m_dis; /*!< List of received delivery info entries. */
	int m_limit; /*!< Request limit, must be greater than zero. */
	ErrorEntry m_error; /*!< Encountered error. */
};

#endif /* _STORED_FILES_H_ */
