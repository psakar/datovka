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

#ifndef _TASK_SEARCH_OWNER_H_
#define _TASK_SEARCH_OWNER_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing contact searching.
 */
class TaskSearchOwner : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		SO_SUCCESS, /*!< Operation was successful. */
		SO_BAD_DATA, /*!< Data related error, non-existent data. */
		SO_COM_ERROR, /*!< Communication error. */
		SO_ERROR /*!< Other type of error occurred. */
	};

	enum BoxType {
		BT_OVM,
		BT_PO,
		BT_PFO,
		BT_FO
	};

	/*!
	 * @brief Structure encapsulating parts of isds_DbOwnerInfo.
	 */
	class SoughtOwnerInfo {
	public:
		SoughtOwnerInfo(const QString &_id, enum BoxType _type,
		    const QString &_ic, const QString &_firstName,
		    const QString &_lastName, const QString &_firmName,
		    const QString &_zipCode);

		QString id; /*!< Box identifier. */
		enum BoxType type; /*!< Box type. */
		QString ic; /*!< Subject identifier number. */
		QString firstName; /*!< Person first name. */
		QString lastName; /*!< Person last name, also last name at birth. */
		QString firmName; /*!< Firm name. */
		QString zipCode; /*!< ZIP code. */
	};

	/*!
	 * @brief Describes found data box.
	 */
	class BoxEntry {
	public:
		/*!
		 *  @brief Constructor.
		 */
		BoxEntry(const QString &i, int t, const QString &n,
		    const QString &ad, const QString &zc, bool &ovm);

		QString id; /*!< Data box id. */
		int type; /*!< Data box type (as specified in libisds). */
		QString name; /*!< Data box name. */
		QString address; /*!< Post address. */
		QString zipCode; /*!< ZIP code. */
		bool effectiveOVM; /*!< Box has OVM role. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] soughtInfo Sought box identifiers.
	 */
	explicit TaskSearchOwner(const QString &userName,
	    const SoughtOwnerInfo &soughtInfo);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	/*!
	 * @brief Search for data boxes matching supplied criteria.
	 *
	 * TODO -- This method must be private.
	 *
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]  soughtInfo Sought box identifiers.
	 * @param[out] foundBoxes List of found data boxes to append data to.
	 * @param[out] error Short error description.
	 * @param[out] longError Long error description.
	 * @return Error value.
	 */
	static
	enum Result isdsSearch(const QString &userName,
	    const SoughtOwnerInfo &soughtInfo, QList<BoxEntry> &foundBoxes,
	    QString &error, QString &longError);

	enum Result m_result; /*!< Return state. */
	QString m_isdsError; /*!< Error description.  */
	QString m_isdsLongError; /*!< Long error description. */
	QList<BoxEntry> m_foundBoxes; /*!< List of found boxes. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskSearchOwner(const TaskSearchOwner &);
	TaskSearchOwner &operator=(const TaskSearchOwner &);

	const QString m_userName; /*!< Account identifier (user login name). */
	const SoughtOwnerInfo m_soughtInfo; /*!< Sought box identifiers. */
};

#endif /* _TASK_SEARCH_OWNER_H_ */
