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
 */
static
void viewFilter(QTableView *tv, const QKeyEvent *ke)
{
	if (0 == tv) {
		Q_ASSERT(0);
		return;
	}
	if (0 == ke) {
		Q_ASSERT(0);
		return;
	}

	switch (ke->key()) {
	case Qt::Key_Home:
		tv->selectRow(0);
		break;
	case Qt::Key_End:
		tv->selectRow(tv->model()->rowCount() - 1);
		break;
	default:
		break;
	}
}

/*!
 * @brief Perfeorms Home/End navigation on a QTableWidget.
 *
 * @param[in,out] tw Non-null pointer to table widget.
 * @param[in]     ke Non-null pointer to key event.
 */
static
void widgetFilter(QTableWidget *tw, const QKeyEvent *ke)
{
	if (0 == tw) {
		Q_ASSERT(0);
		return;
	}
	if (0 == ke) {
		Q_ASSERT(0);
		return;
	}

	switch (ke->key()) {
	case Qt::Key_Home:
		tw->selectRow(0);
		break;
	case Qt::Key_End:
		tw->selectRow(tw->rowCount() - 1);
		break;
	default:
		break;
	}
}

bool TableHomeEndFilter::eventFilter(QObject *object, QEvent *event)
{
	const QKeyEvent *ke = 0;
	QTableView *tv = 0;
	QTableWidget *tw = 0;
	if (event->type() == QEvent::KeyPress) {
		ke = (QKeyEvent *) event;
	}

	if (0 != ke) {
		tv = dynamic_cast<QTableView *>(object);
		tw = dynamic_cast<QTableWidget *>(object);
	}

	if (0 != tv) {
		viewFilter(tv, ke);
		return false;
	} else if (0 != tw) {
		widgetFilter(tw, ke);
		return false;
	}

	return QObject::eventFilter(object, event);
}
