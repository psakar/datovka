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

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPixmap>

#include "src/common.h"
#include "src/log/log.h"
#include "src/models/files_model.h"
#include "src/views/attachment_table_view.h"

AttachmentTableView::AttachmentTableView(QWidget *parent)
    : LoweredTableView(parent),
    m_dragStartPosition()
{
	setDragEnabled(true);
}

void AttachmentTableView::dragEnterEvent(QDragEnterEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	const QMimeData *mimeData = event->mimeData();
	if (0 == mimeData) {
		Q_ASSERT(0);
		return;
	}

	if (mimeData->hasUrls()) {
		event->acceptProposedAction();
	} else {
		logInfo("Rejecting drag enter event with mime type '%s'.\n",
		    mimeData->formats().join(" ").toUtf8().constData());
	}
}

void AttachmentTableView::dragMoveEvent(QDragMoveEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	event->acceptProposedAction();
}

void AttachmentTableView::dropEvent(QDropEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	const QMimeData *mimeData = event->mimeData();
	if (0 == mimeData) {
		Q_ASSERT(0);
		return;
	}

	if (!mimeData->hasUrls()) {
		return;
	}

	/* TODO -- Access the model in a more transparent way. */
	DbFlsTblModel *attachmentModel =
	    qobject_cast<DbFlsTblModel *>(model());
	if (0 == attachmentModel) {
		return;
	}

	attachmentModel->dropMimeData(mimeData, event->dropAction(), -1, -1, QModelIndex());
}

void AttachmentTableView::mouseMoveEvent(QMouseEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	if (!(event->buttons() & Qt::LeftButton) ||
	    (event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
		QTableView::mouseMoveEvent(event);
		return;
	}

	/* TODO -- Access the model in a more transparent way. */
	DbFlsTblModel *attachmentModel =
	    qobject_cast<DbFlsTblModel *>(model());
	if (0 == attachmentModel) {
		return;
	}

	QDrag *drag = new (std::nothrow) QDrag(this);
	if (0 == drag) {
		return;
	}

	QModelIndexList rowIdxs(this->selectionModel()->selectedRows(0));
	QMimeData *mimeData = attachmentModel->mimeData(rowIdxs);
	if (0 == mimeData) {
		delete drag;
		return;
	}
	drag->setMimeData(mimeData);

	if (rowIdxs.size() == 1) {
		drag->setDragCursor(
		    QPixmap(ICON_3PARTY_PATH "document_32.png"),
		    Qt::MoveAction);
		drag->setDragCursor(
		    QPixmap(ICON_3PARTY_PATH "document_plus_32.png"),
		    Qt::CopyAction);
	} else {
		drag->setDragCursor(
		    QPixmap(ICON_3PARTY_PATH "documents_32.png"),
		    Qt::MoveAction);
		drag->setDragCursor(
		    QPixmap(ICON_3PARTY_PATH "documents_plus_32.png"),
		    Qt::CopyAction);
	}

	/* Ignore the return value of the drop action. */
	drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void AttachmentTableView::mousePressEvent(QMouseEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	if (event->button() == Qt::LeftButton) {
		m_dragStartPosition = event->pos();
	}

	QTableView::mousePressEvent(event);
}
