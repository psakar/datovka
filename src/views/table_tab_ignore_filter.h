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

#ifndef _TABLE_TAB_IGNORE_FILTER_H_
#define _TABLE_TAB_IGNORE_FILTER_H_

#include <QObject>

/*!
 * @brief This object is used as to tweak the behaviour of a QTableView and
 *    QTableWidget when Tab key is pressed.
 */
class TableTabIgnoreFilter : public QObject {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	TableTabIgnoreFilter(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~TableTabIgnoreFilter(void);

	/*!
	 * @brief Event filter function.
	 *
	 * @note The function catches the Tab key and ignores the event.
	 *
	 * @param[in,out] object View object.
	 * @param[in]     event Caught event.
	 * @return What would QObject::eventFilter() return.
	 */
	virtual
	bool eventFilter(QObject *object, QEvent *event) Q_DECL_OVERRIDE;
};

#endif /* _TABLE_TAB_IGNORE_FILTER_H_ */
