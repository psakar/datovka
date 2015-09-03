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

#include "files_model.h"

QVariant DbFlsTblModel::data(const QModelIndex &index, int role) const
{
	if ((Qt::DisplayRole == role) && (4 == index.column())) {
		/* Compute attachment size from base64 length. */
		QByteArray b64 = QSqlQueryModel::data(index.sibling(
		    index.row(), 2), role).toByteArray();
		int b64size = b64.size();
		int cnt = 0;
		if (b64size >= 3) {
			for (int i = 1; i <= 3; ++i) {
				if ('=' == b64[b64size - i]) {
					++cnt;
				}
			}
		}
		return QSqlQueryModel::data(index, role).toInt() * 3 / 4 - cnt;
		/* old solution */
		//const QByteArray &b64 = QSqlQueryModel::data(
		//    index.sibling(index.row(), 2), role).toByteArray();
		//return QByteArray::fromBase64(b64).size();
	} else {
		return QSqlQueryModel::data(index, role);
	}
}
