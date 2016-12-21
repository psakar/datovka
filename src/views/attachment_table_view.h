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

#ifndef _ATTACHMENT_TABLE_VIEW_H_
#define _ATTACHMENT_TABLE_VIEW_H_

#if 0
#include <QList>
#include <QMouseEvent>
#include <QString>
#include <QUrl>
#endif

#include "src/views/lowered_table_view.h"

/*!
 * @brief Custom attachment table view class with dragging enabled.
 */
class AttachmentTableView : public LoweredTableView {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit AttachmentTableView(QWidget *parent = 0);

protected:
#if 0
	/*!
	 * @brief Allows the drop.
	 */
	virtual
	void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
#endif

#if 0
	/*!
	 * @brief Allow drag move.
	 */
	virtual
	void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
#endif

	/*!
	 * @brief Processes the drop.
	 *
	 * @param[in,out] event Drag event.
	 */
	virtual
	void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

#if 0
	/*!
	 * @brief Activates the drag.
	 */
	virtual
	void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
#endif

#if 0
	/*
	 * @brief Used for storing the beginning position of the drag.
	 */
	virtual
	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
#endif

private:
	/*!
	 * @brief Preforms displayed data reordering according to drop data.
	 *
	 * @param[in] event Drop event.
	 */
	void shuffleOnDrop(QDropEvent *event);

	/*!
	 * @brief Return true if this is a move from ourself and \a index is a
	 *     child of the selection that is being moved.
	 */
	bool droppingOnItself(QDropEvent *event, const QModelIndex &index);

	/*!
	 * @brief If the event hasn't already been accepted, determines the
	 *     index to drop on.
	 *
	 * if (row == -1 && col == -1)
	 *        // append to this drop index
	 *    else
	 *        // place at row, col in drop index
	 *
	 *  If it returns \c true a drop can be done, and dropRow, dropCol
	 *  and dropIndex reflects the position of the drop.
	 */
	bool dropOn(QDropEvent *event, int *dropRow, int *dropCol,
	    QModelIndex *dropIndex);

	/*!
	 * @brief Determined drop indicator position.
	 */
	QAbstractItemView::DropIndicatorPosition position(const QPoint &pos,
	    const QRect &rect, const QModelIndex &index) const;
};

#endif /* _ATTACHMENT_TABLE_VIEW_H_ */
