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

#include "src/common.h"
#include "src/models/files_model.h"

DbFlsTblModel::DbFlsTblModel(QObject *parent)
    : TblModel(parent)
{
}

QVariant DbFlsTblModel::data(const QModelIndex &index, int role) const
{
	if ((Qt::DisplayRole == role) && (FSIZE_COL == index.column())) {
		/* Compute attachment size from base64 length. */
		QByteArray b64 = _data(index.sibling(index.row(), CONTENT_COL),
		    role).toByteArray();
		return base64RealSize(b64);
	} else {
		return _data(index, role);
	}
}

Qt::ItemFlags DbFlsTblModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = TblModel::flags(index);

	if (index.isValid()) {
		defaultFlags |= Qt::ItemIsDragEnabled;
	}

	return defaultFlags;
}
