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

#include "src/views/table_key_press_filter.h"

TableKeyPressFilter::TableKeyPressFilter(QObject *parent)
    : QObject(parent),
    m_keyActions()
{ }

bool TableKeyPressFilter::eventFilter(QObject *object, QEvent *event)
{
	Q_UNUSED(object);

	const QKeyEvent *ke = Q_NULLPTR;
	QMap<int, ActionToDo>::const_iterator cIt = m_keyActions.constEnd();

	if (event->type() == QEvent::KeyPress) {
		ke = (QKeyEvent *)event;
	}

	if (Q_NULLPTR != ke) {
		int key = ke->key();

		cIt = m_keyActions.constFind(key);
	}

	if (m_keyActions.constEnd() != cIt) {
		void (*actionFuncPtr)(QObject *) = cIt->actionFuncPtr;
		QObject *actionObj = cIt->actionObj;

		Q_ASSERT(Q_NULLPTR != actionFuncPtr);

		actionFuncPtr(actionObj);
		if (cIt->blockEvent) {
			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

void TableKeyPressFilter::registerAction(int key, void (*func)(QObject *),
    QObject *obj, bool stop)
{
	if (Q_NULLPTR != func) {
		m_keyActions.insert(key, ActionToDo(func, obj, stop));
	} else {
		/* Delete antry for given key. */
		m_keyActions.remove(key);
	}
}
