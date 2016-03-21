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

#ifndef _TAGS_DELEGATE_H_
#define _TAGS_DELEGATE_H_

#include <QStyledItemDelegate>

/*!
 * @brief Draws tag entries.
 */
class TagsDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @brief[in] parent Parent widget.
	 */
	explicit TagsDelegate(QWidget *parent = 0);

	/*!
	 * @brief Paint the item.
	 *
	 * @param[in,out] painter Painter to be used.
	 * @param[in]     option  Drawing parameters.
	 * @param[in]     index   Specifies the item to be drawn.
	 */
	virtual
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
	    const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns the size needed to display the data.
	 *
	 * @param[in] option Drawing parameters.
	 * @param[in] index  Specifies the item to be drawn.
	 * @return Size needed to display the data.
	 */
	virtual
	QSize sizeHint(const QStyleOptionViewItem &option,
	    const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*
	 * The mode data are read-only. There is no implementation of createEditor()
	 * destroyEditor(), setEditorData() and setModelData().
	 */

};

#endif /* _TAGS_DELEGATE_H_ */
