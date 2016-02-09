/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QObject>
#include <QVariant>

#include "src/models/table_model.h"

/*
 * TODO -- AttachmentModel and DbFlsTblModel provide same abstraction over
 * different data (message and SQL query result).
 * These classes should probably share some code or at least share a parent
 * class.
 */

/*!
 * @brief Custom file model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 * It is also used for attachment content caching.
 */
class DbFlsTblModel : public TblModel {
    Q_OBJECT

public:
	/*!
	 * @brief Identifies the column index.
	 */
	enum ColumnNumbers {
		ATT_ID_COL = 0, /* Attachment identifier. */
		DMID_COL = 1, /* Message identifier. */
		ATT_CONT_COL = 2, /* Encoded attachment content. */
		ATT_FDESC_COL = 3, /* Attachment file name. */
		ATT_FSIZE_COL = 4 /* Attachment file size. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	DbFlsTblModel(QObject *parent = 0);

	/*!
	 * @brief Returns the data stored under the given role.
	 *
	 * @param[in] index Position.
	 * @param[in] role  Role if the position.
	 * @return Data or invalid QVariant if no matching data found.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const;

	/*!
	 * @brief Used to set items draggable.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif /* _FILES_MODEL_H_ */
