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

#ifndef _TASK_SEARCH_OWNER_FULLTEXT_H_
#define _TASK_SEARCH_OWNER_FULLTEXT_H_

#include <QList>
#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing contact searching.
 */
class TaskSearchOwnerFulltext : public Task {
public:
	static const quint64 maxResponseSize; /*!< Maximal response size. */

	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		SOF_SUCCESS, /*!< Operation was successful. */
		SOF_BAD_DATA, /*!< Data related error, non-existent data. */
		SOF_COM_ERROR, /*!< Communication error. */
		SOF_ERROR /*!< Other type of error occurred. */
	};

	enum FulltextTarget {
		FT_ALL, /*!< Search in all fields. */
		FT_ADDRESS, /*!< Search in address. */
		FT_IC, /* Search in organization identifier. */
		FT_BOX_ID /* Search in box ID. */
	};

	enum BoxType {
		BT_ALL, /*!< Search all types. */
		BT_OVM,
		BT_PO,
		BT_PFO,
		BT_FO
	};

	/*!
	 * @brief Describes a data box.
	 */
	class BoxEntry {
	public:
		/*!
		 *  @brief Constructor.
		 */
		BoxEntry(const QString &i, int t, const QString &n,
		    const QString &ad, bool ovm, bool ac, bool ps, bool cs);

		QString id; /*!< Data box id. */
		int type; /*!< Data box type (as specified in libisds). */
		QString name; /*!< Data box name. */
		QString address; /*!< Post address. */
		bool effectiveOVM; /*!< Box has effective OVM role. */
		bool active; /*!< Box is active. */
		bool publicSending; /*!< Accepts non-commercial messages. */
		bool commercialSending; /*!< Box can receive PDZ from current box. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] info     Sought box identifiers.
	 */
	explicit TaskSearchOwnerFulltext(const QString &userName,
	    const QString &query, enum FulltextTarget target, enum BoxType type,
	    quint64 pageSize = maxResponseSize, quint64 pageNumber = 0);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	enum Result m_result; /*!< Return state. */
	QString m_isdsError; /*!< Error description.  */
	QString m_isdsLongError; /*!< Long error description. */
	quint64 m_pageSize; /*!< Actual requested page size. */
	quint64 m_pageNumber; /*!< Requested page number. */
	quint64 m_totalMatchingBoxes; /*!< Set to number of boxes matching request. */
	quint64 m_currentPageStart; /*!< Index of the first entry on the page. */
	quint64 m_currentPageSize; /*!< Size of the actual page. */
	bool m_isLastPage; /*!< Set true if last page acquired. */
	QList<BoxEntry> m_foundBoxes; /*!< List of found boxes. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskSearchOwnerFulltext(const TaskSearchOwnerFulltext &);
	TaskSearchOwnerFulltext &operator=(const TaskSearchOwnerFulltext &);

	/*!
	 * @brief Search for data boxes matching supplied criteria.
	 *
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]  info     Sought box identifiers.
	 * @param[out] results  List of found data boxes.
	 * @return Return code.
	 */
	static
	enum Result isdsSearch2(const QString &userName,
	    const QString &query, enum FulltextTarget target, enum BoxType type,
	    quint64 pageSize, quint64 pageNumber, quint64 &totalMatchingBoxes,
	    quint64 &currentPageStart, quint64 &currentPageSize,
	    bool &isLastPage, QList<BoxEntry> &foundBoxes,
	    QString &error, QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	const QString m_query; /*!< Search phrase. */
	const enum FulltextTarget m_target; /*!< Full-text search target. */
	const enum BoxType m_boxType; /*!< Sought box type. */
};

#endif /* _TASK_SEARCH_OWNER_FULLTEXT_H_ */
