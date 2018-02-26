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

#pragma once

#include <QSortFilterProxyModel>

/*!
 * @brief Enables filtering according to metadata.
 */
class UploadHierarchyProxyModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit UploadHierarchyProxyModel(QObject *parent = Q_NULLPTR);

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
	 * @brief Returns true if the item should be included in the model.
	 *
	 * @param[in] sourceIdx Source index.
	 * @return Whether the item meets the criteria.
	 */
	bool filterAcceptsItem(const QModelIndex &sourceIdx) const;
};
