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

#include <QEvent>
#include <QKeyEvent>
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
 * @brief Performs space bar selection on a QTableWidget.
 *
 * @param[in,out] tw Non-null pointer to table widget.
 * @param[in]     ke Non-null pointer to key event.
 * @return True when filter applied.
 */
static
bool widgetFilter(QTableWidget *tw, const QKeyEvent *ke)
{
	if (0 == tw) {
		Q_ASSERT(0);
		return false;
	}
	if (0 == ke) {
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

			if (0 == frstItem) {
				Q_ASSERT(0);
				return false;
			}

			bool checked = frstItem->checkState() == Qt::Checked;

			foreach (QTableWidgetItem *const item, selectedItems) {
				if (0 == item) {
					continue;
				}
				QTableWidgetItem *checkItem =
				    tw->item(item->row(), 0);

				if (0 == checkItem) {
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
	const QKeyEvent *ke = 0;
	QTableWidget *tw = 0;
	if (event->type() == QEvent::KeyPress) {
		ke = (QKeyEvent *) event;
	}

	if (0 != ke) {
		tw = dynamic_cast<QTableWidget *>(object);
	}

	if (0 != tw) {
		if (widgetFilter(tw, ke)) {
			return true;
		}
	}

	return QObject::eventFilter(object, event);
}
