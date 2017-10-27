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
#include <QTableView>
#include <QTableWidget>

#include "src/views/table_home_end_filter.h"

TableHomeEndFilter::TableHomeEndFilter(QObject *parent)
    : QObject(parent)
{ }

TableHomeEndFilter::~TableHomeEndFilter(void)
{ }

/*!
 * @brief Performs Home/End navigation on a QTableView.
 *
 * @param[in,out] tv Non-null pointer to table view.
 * @param[in]     ke Non-null pointer to key event.
 * @return True when filter applied.
 */
static
bool viewFilter(QTableView *tv, const QKeyEvent *ke)
{
	if (Q_UNLIKELY(Q_NULLPTR == tv)) {
		Q_ASSERT(0);
		return false;
	}
	if (Q_UNLIKELY(Q_NULLPTR == ke)) {
		Q_ASSERT(0);
		return false;
	}

	switch (ke->key()) {
	case Qt::Key_Home:
		tv->selectRow(0);
		return true;
		break;
	case Qt::Key_End:
		tv->selectRow(tv->model()->rowCount() - 1);
		return true;
		break;
	default:
		break;
	}

	return false;
}

/*!
 * @brief Performs Home/End navigation on a QTableWidget.
 *
 * @param[in,out] tw Non-null pointer to table widget.
 * @param[in]     ke Non-null pointer to key event.
 * @return True when filter applied.
 */
static
bool widgetFilter(QTableWidget *tw, const QKeyEvent *ke)
{
	if (Q_UNLIKELY(Q_NULLPTR == tw)) {
		Q_ASSERT(0);
		return false;
	}
	if (Q_UNLIKELY(Q_NULLPTR == ke)) {
		Q_ASSERT(0);
		return false;
	}

	switch (ke->key()) {
	case Qt::Key_Home:
		tw->selectRow(0);
		return true;
		break;
	case Qt::Key_End:
		tw->selectRow(tw->rowCount() - 1);
		return true;
		break;
	default:
		break;
	}

	return false;
}

bool TableHomeEndFilter::eventFilter(QObject *object, QEvent *event)
{
	const QKeyEvent *ke = Q_NULLPTR;
	QTableView *tv = Q_NULLPTR;
	QTableWidget *tw = Q_NULLPTR;
	if (event->type() == QEvent::KeyPress) {
		ke = (QKeyEvent *)event;
	}

	if (Q_NULLPTR != ke) {
		tv = qobject_cast<QTableView *>(object);
		tw = qobject_cast<QTableWidget *>(object);
	}

	if (Q_NULLPTR != tv) {
		if (viewFilter(tv, ke)) {
			return true;
		}
	} else if (Q_NULLPTR != tw) {
		if (widgetFilter(tw, ke)) {
			return true;
		}
	}

	return QObject::eventFilter(object, event);
}
