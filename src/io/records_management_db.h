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

#ifndef _RECORDS_MANAGEMENT_DB_H_
#define _RECORDS_MANAGEMENT_DB_H_

#include <QList>
#include <QString>
#include <QStringList>

#include "src/datovka_shared/io/sqlite/db_single.h"

/*!
 * @brief Encapsulates records management database.
 */
class RecordsManagementDb : public SQLiteDbSingle {

public:
	class ServiceInfoEntry {
	public:
		/*!
		 * @brief Constructor.
		 */
		ServiceInfoEntry(void)
		    : url(), name(), tokenName(), logoSvg()
		{
		}

		/*!
		 * @brief Return true in entry is valid.
		 */
		inline
		bool isValid(void) const {
			return !url.isEmpty();
		}

		QString url; /*!< Service URL. */
		QString name; /*!< Service name. */
		QString tokenName; /*!< Token name. */
		QByteArray logoSvg; /*!< Raw SVG data. */
	};

	/* Use parent class constructor. */
	using SQLiteDbSingle::SQLiteDbSingle;

	/*!
	 * @brief Erases all database entries.
	 *
	 * @return True on success.
	 */
	bool deleteAllEntries(void);

	/*!
	 * @brief Update service info entry.
	 *
	 * @note There can be only one service info entry.
	 *
	 * @param[in] entry Service info entry.
	 */
	bool updateServiceInfo(const ServiceInfoEntry &entry);

	/*!
	 * @brief Obtain service information from database.
	 *
	 * @return Invalid service info if no valid service information found.
	 */
	ServiceInfoEntry serviceInfo(void) const;

	/*!
	 * @brief Deletes all message-related data.
	 *
	 * @return True on success.
	 */
	bool deleteAllStoredMsg(void);

	/*!
	 * @brief Deletes stored locations for given message.
	 *
	 * @param[in] dmId Message identifier.
	 * @return True on success.
	 */
	bool deleteStoredMsg(qint64 dmId);

	/*!
	 * @brief Inserts or replaces stored locations for given message.
	 *
	 * @param[in] dmId Message identifier.
	 * @param[in] locations List of locations to be stored.
	 * @return True on success.
	 */
	bool updateStoredMsg(qint64 dmId, const QStringList &locations);

	/*!
	 * @brief Obtains all message identifiers.
	 *
	 * @return List of message identifiers, empty list on error.
	 */
	QList<qint64> getAllDmIds(void) const;

	/*!
	 * @brief Reads stored location or given message.
	 *
	 * @param[in] dmId Message identifier.
	 * @return List of locations, empty list on error.
	 */
	QStringList storedMsgLocations(qint64 dmId) const;

protected:
	/*!
	 * @brief Returns list of tables.
	 *
	 * @return List of pointers to tables.
	 */
	virtual
	QList<class SQLiteTbl *> listOfTables(void) const Q_DECL_OVERRIDE;
};

/*!
 * @brief Global records management database.
 */
extern RecordsManagementDb *globRecordsManagementDbPtr;

#endif /* _RECORDS_MANAGEMENT_DB_H_ */
