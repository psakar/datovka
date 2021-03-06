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

#ifndef _SORT_FILTER_PROXY_MODEL_H_
#define _SORT_FILTER_PROXY_MODEL_H_

#include <QSortFilterProxyModel>

class SortFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	static
	const QString blankFilterEditStyle; /*!< Blank filter line style. */
	static
	const QString foundFilterEditStyle; /*!< Found filter line style. */
	static
	const QString notFoundFilterEditStyle; /*!< Not found filter line style. */

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit SortFilterProxyModel(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Set the column where the key is used for filtering.
	 *
	 * @param[in] column  Column number, -1 for all columns.
	 */
	void setFilterKeyColumn(int column);

	/*!
	 * @brief Set columns which are used for filtering.
	 *
	 * @param[in] columns Column list. If list contains -1 or is empty
	 *                    then all columns are used for filtering.
	 */
	void setFilterKeyColumns(const QList<int> &columns);

	/*!
	 * @brief Return columns which are used for filtering.
	 *
	 * @return List of column numbers, may be empty.
	 */
	QList<int> filterKeyColumns(void);
 
protected:
	/*!
	 * @brief Returns true if the item in the row indicated by the
	 *     given source row and source parent should be included in the
	 *     model; otherwise returns false.
	 *
	 * @param[in] sourceRow    Row number.
	 * @param[in] sourceParent Parent index.
	 * @return Whether the row indicated should be included in the model.
	 */
	virtual
	bool filterAcceptsRow(int sourceRow,
	    const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns true if the value of the item referred to by the
	 *     given left index is less than the value of the item referred to
	 *     by the given right index, otherwise returns false.
	 *
	 * @param[in] sourceLeft  Left index.
	 * @param[in] sourceRight Right index.
	 * @return Whether the left index precedes the right index.
	 */
	virtual
	bool lessThan(const QModelIndex &sourceLeft,
	    const QModelIndex &sourceRight) const Q_DECL_OVERRIDE;
 
private:
	/*!
	 * @brief Return the column where the key is used to filter the content.
	 *
	 * @note The method is private so it cannot be used.
	 *
	 * @return Column number.
	 */
	int filterKeyColumn(void) const;

	/*!
	 * @brief Returns true if the item should be included in the model.
	 *
	 * @param[in] sourceIdx Source index.
	 * @return Whether the item meets the criteria.
	 */
	bool filterAcceptsItem(const QModelIndex &sourceIdx) const;

	QList<int> m_filterColumns; /*!< Columns used for filtering. */
};

#endif /* _SORT_FILTER_PROXY_MODEL_H_ */
