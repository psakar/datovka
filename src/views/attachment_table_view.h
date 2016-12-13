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

#if 0
	/*!
	 * @brief Processes the drop.
	 */
	virtual
	void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
#endif

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
	QPoint m_dragStartPosition /*!< Holds the starting drag point. */;
};

#endif /* _ATTACHMENT_TABLE_VIEW_H_ */
