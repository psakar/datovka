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

#ifndef _LOWERED_TABLE_WIDGET_H_
#define _LOWERED_TABLE_WIDGET_H_

#include <QTableWidget>

/*!
 * @brief Table widget with lowered row height.
 */
class LoweredTableWidget : public QTableWidget {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit LoweredTableWidget(QWidget *parent = 0);

	/*!
	 * @brief Sets narrowed row size.
	 */
	void setNarrowedLineHeight(void) const;
};

#endif /* _LOWERED_TABLE_WIDGET_H_ */
