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

#ifndef _LOWERED_TREE_VIEW_H_
#define _LOWERED_TREE_VIEW_H_

#include <QItemDelegate>
#include <QTreeView>

/*!
 * @brief Used to enforce row height.
 *
 * @note QTreeView normally takes row height from sizeHint().
 */
class LoweredItemDelegate : public QItemDelegate {
	Q_OBJECT

public:
	/*!
	 * @brief Returns size hint with enforced row height.
	 *
	 * @param[in] option Style option.
	 * @param[in] index Index identifying the displayed element.
	 * @return Size hint with enforced row height.
	 */
	virtual
	QSize sizeHint(const QStyleOptionViewItem &option,
	    const QModelIndex &index) const Q_DECL_OVERRIDE;
};

/*!
 * @brief Tree view with lowered row height.
 */
class LoweredTreeView : public QTreeView {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit LoweredTreeView(QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Sets narrowed row size.
	 */
	void setNarrowedLineHeight(void);
};

#endif /* _LOWERED_TREE_VIEW_H_ */
