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

#include "src/views/table_space_selection_filter.h"

/*
 * Check box column.
 *
 * TODO -- The position should be defined somewhere nearer the widget.
 */
#define CHECK_COL 0

TableSpaceSelectionFilter::TableSpaceSelectionFilter(QObject *parent)
    : QObject(parent)
{
}

TableSpaceSelectionFilter::~TableSpaceSelectionFilter(void)
{
}

/*!
 * @brief Performs space bar selection on a QTableView.
 *
 * @param[in,out] tv Non-null pointer to table view.
 * @param[in]     ke Non-null pointer to key event.
 * @return True when filter applied.
 */
static
bool widgetFilter(QTableView *tv, const QKeyEvent *ke)
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
	case Qt::Key_Space:
		{
			QModelIndexList checkColIdxs(
			    tv->selectionModel()->selectedRows(CHECK_COL));

			if (checkColIdxs.isEmpty()) {
				return false;
			}

			bool checked =
			    checkColIdxs.first().data(Qt::CheckStateRole) == Qt::Checked;

			foreach (const QModelIndex &idx, checkColIdxs) {
				tv->model()->setData(idx,
				    checked ? Qt::Unchecked : Qt::Checked,
				    Qt::CheckStateRole);
			}
		}
		return true;
		break;
	default:
		break;
	}

	return false;
}

/*!
 * @brief Performs space bar selection on a QTableWidget.
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
	case Qt::Key_Space:
		{
			QList<QTableWidgetItem *> selectedItems =
			    tw->selectedItems();

			if (selectedItems.isEmpty()) {
				return false;
			}

			QTableWidgetItem *frstItem = tw->item(
			    selectedItems.first()->row(), CHECK_COL);

			if (Q_UNLIKELY(Q_NULLPTR == frstItem)) {
				Q_ASSERT(0);
				return false;
			}

			bool checked = frstItem->checkState() == Qt::Checked;

			foreach (QTableWidgetItem *const item, selectedItems) {
				if (Q_UNLIKELY(Q_NULLPTR == item)) {
					continue;
				}
				QTableWidgetItem *checkItem =
				    tw->item(item->row(), CHECK_COL);

				if (Q_UNLIKELY(Q_NULLPTR == checkItem)) {
					Q_ASSERT(0);
					continue;
				}
				checkItem->setCheckState(
				    checked ? Qt::Unchecked : Qt::Checked);
			}
		}
		return true;
		break;
	default:
		break;
	}

	return false;
}

bool TableSpaceSelectionFilter::eventFilter(QObject *object, QEvent *event)
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
		if (widgetFilter(tv, ke)) {
			return true;
		}
	} else if (Q_NULLPTR != tw) {
		if (widgetFilter(tw, ke)) {
			return true;
		}
	}

	return QObject::eventFilter(object, event);
}
