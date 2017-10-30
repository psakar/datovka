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

#ifndef _TABLE_KEY_PRESS_FILTER_H_
#define _TABLE_KEY_PRESS_FILTER_H_

#include <QMap>
#include <QObject>

/*!
 * @brief This object is used as to tweak the behaviour of a QTableView and
 *    QTableWidget when selected key is pressed.
 */
class TableKeyPressFilter : public QObject {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit TableKeyPressFilter(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Event filter function.
	 *
	 * @note The function catches the given keys and calls given action
	 *     functions.
	 *
	 * @param[in,out] object View object.
	 * @param[in]     event Caught event.
	 * @return True if further processing of event should be blocked.
	 */
	virtual
	bool eventFilter(QObject *object, QEvent *event) Q_DECL_OVERRIDE;

	/*!
	 * @brief Register an action.
	 *
	 * @note Should \p func be 0 then the action is deleted.
	 *
	 * @param[in] key Code of the key (Qt::Key) to register the action for.
	 * @param[in] func Action function.
	 * @param[in] obj Action function parameter.
	 * @param[in] stop Set true if related event should be prevented
	 *                 from further processing.
	 */
	void registerAction(int key, void (*func)(QObject *), QObject *obj,
	    bool stop);

private:
	/*!
	 * @brief Holds function and object for specified action.
	 *
	 * @note The class does not take possession of the pointers, it only
	 *     uses the copies.
	 */
	class ActionToDo {
	public:
		/*!
		 * @brief Constructor.
		 *
		 * @param[in] func Action function.
		 * @param[in] obj Action function parameter.
		 * @param[in] stop Set true if related event should be prevented
		 *                 from further processing.
		 */
		ActionToDo(void (*func)(QObject *), QObject *obj, bool stop)
		    : actionFuncPtr(func), actionObj(obj), blockEvent(stop)
		{ }

		/*!
		 * @brief Constructor.
		 *
		 * @param[in] act Action description.
		 */
		ActionToDo(const ActionToDo &act)
		    : actionFuncPtr(act.actionFuncPtr),
		    actionObj(act.actionObj),
		    blockEvent(act.blockEvent)
		{ }

		void (*actionFuncPtr)(QObject *); /*!< Pointer to a function. */
		QObject *actionObj; /*!< Parameter of the action function. */
		bool blockEvent; /*!< Set true to stop the key event being handled further. */
	};

	QMap<int, ActionToDo> m_keyActions; /*!< Registered actions. */
};

#endif /* _TABLE_KEY_PRESS_FILTER_H_ */
