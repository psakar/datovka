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

#include <QEvent>
#include <QKeyEvent>

#include "src/views/table_tab_ignore_filter.h"

TableTabIgnoreFilter::TableTabIgnoreFilter(QObject *parent)
    : QObject(parent)
{
}

TableTabIgnoreFilter::~TableTabIgnoreFilter(void)
{
}

bool TableTabIgnoreFilter::eventFilter(QObject *object, QEvent *event)
{
	Q_UNUSED(object);

	QKeyEvent *ke = Q_NULLPTR;

	if (event->type() == QEvent::KeyPress) {
		ke = (QKeyEvent *)event;
	}

	if (Q_NULLPTR != ke) {
		switch (ke->key()) {
		case Qt::Key_Tab:
			ke->ignore();
			break;
		default:
			break;
		}
	}

	return QObject::eventFilter(object, event);
}
