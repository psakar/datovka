/*
 * Copyright (C) 2014-2015 CZ.NIC
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

#ifndef _FILES_MODEL_H_
#define _FILES_MODEL_H_

#include <QModelIndex>
#include <QObject>
#include <QSqlQueryModel>
#include <QVariant>

/*!
 * @brief Custom file model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 */
class DbFlsTblModel : public QSqlQueryModel {
	Q_OBJECT
public:
	/*!
	 * @brief Compute viewed data in file size column.
	 *
	 * @param[in] index Item index.
	 * @param[in] role  Display role.
	 * @return Data modified according to the given role.
	 */
	virtual QVariant data(const QModelIndex &index, int role) const;
};

#endif /* _FILES_MODEL_H_ */
