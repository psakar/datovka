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

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing contact searching.
 */
class TaskSearchOwnerFulltext : public Task {
public:
	static const quint64 maxResponseSize; /*!< Maximal response size. */

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] info     Sought box identifiers.
	 */
	explicit TaskSearchOwnerFulltext(const QString &userName,
	    const QString &query, const isds_fulltext_target *target,
	    const isds_DbType *box_typ,
	    quint64 pageSize = maxResponseSize, quint64 pageNumber = 0);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~TaskSearchOwnerFulltext(void);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	int m_isdsRetError; /*!< Returned error code. */
	quint64 m_pageSize; /*!< Actual requested page size. */
	quint64 m_pageNumber; /*!< Requested page number. */
	quint64 m_totalMatchingBoxes; /*!< Set to number of boxes matching request. */
	quint64 m_currentPageStart; /*!< Index of the first entry on the page. */
	quint64 m_currentPageSize; /*!< Size of the actual page. */
	bool m_isLastPage; /*!< Set true if last page acquired. */
	struct isds_list *m_results; /*!< List of found data boxes. */

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
	 * @return Value of isds_error.
	 */
	static
	int isdsSearch2(const QString &userName,
	    const QString &query, const isds_fulltext_target *target,
	    const isds_DbType *box_type, quint64 pageSize, quint64 pageNumber,
	    quint64 &totalMatchingBoxes, quint64 &currentPageStart,
	    quint64 &currentPageSize, bool &isLastPage,
	    struct isds_list **results);

	const QString m_userName; /*!< Account identifier (user login name). */
	const QString m_query; /*!< search phrase. */
	const isds_fulltext_target *m_target; /*!< Sought box identifiers. */
	const isds_DbType *m_box_type;
};

#endif /* _TASK_SEARCH_OWNER_FULLTEXT_H_ */
