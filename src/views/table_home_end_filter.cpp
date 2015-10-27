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

#include <QEvent>
#include <QKeyEvent>
#include <QTableView>

#include "src/views/table_home_end_filter.h"

TableHomeEndFilter::TableHomeEndFilter(QObject *parent)
    : QObject(parent)
{ }

TableHomeEndFilter::~TableHomeEndFilter(void)
{ }

bool TableHomeEndFilter::eventFilter(QObject *object, QEvent *event)
{
	QKeyEvent *ke = 0;

	QTableView *tw = dynamic_cast<QTableView *>(object);

	if ((0 != tw) && (event->type() == QEvent::KeyPress)) {
		ke = (QKeyEvent *) event;
	}

	if (0 != ke) {
		switch (ke->key()) {
		case Qt::Key_Home:
			tw->selectRow(0);
			return false;
			break;
		case Qt::Key_End:
			tw->selectRow(tw->model()->rowCount() - 1);
			return false;
			break;
		default:
			break;
		}
	}

	return QObject::eventFilter(object,event);
}
