/*
 * Copyright (C) 2014-2018 CZ.NIC
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
#include <QDropEvent>
#include <QProxyStyle>

#include "src/datovka_shared/log/log.h"
#include "src/models/attachments_model.h"
#if 0
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPixmap>

#include "src/common.h"
#include "src/models/files_model.h"
#endif
#include "src/views/attachment_table_view.h"

/*!
 * @brief Style used to override default drop indicator.
 */
class AttachmentViewStyle : public QProxyStyle {
public:
	/*!
	 * @brief Constructor.
	 */
	explicit AttachmentViewStyle(QStyle *style = Q_NULLPTR);

	/*!
	 * @brief Draws primitive element.
	 *
	 * @note Used to draw drop indicator as whole line through the entire
	 *     row.
	 */
	virtual
	void drawPrimitive(PrimitiveElement element,
	    const QStyleOption *option, QPainter *painter,
	    const QWidget *widget = Q_NULLPTR) const Q_DECL_OVERRIDE;
};

AttachmentViewStyle::AttachmentViewStyle(QStyle *style)
    : QProxyStyle(style)
{
}

void AttachmentViewStyle::drawPrimitive(PrimitiveElement element,
    const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if ((element == QStyle::PE_IndicatorItemViewItemDrop) &&
	    (!option->rect.isNull())) {
		QStyleOption opt(*option);
		opt.rect.setLeft(0);
		if (widget != Q_NULLPTR) {
			 opt.rect.setRight(widget->width());
		}
		QProxyStyle::drawPrimitive(element, &opt, painter, widget);
		return;
	}
	QProxyStyle::drawPrimitive(element, option, painter, widget);
}

AttachmentTableView::AttachmentTableView(QWidget *parent)
    : LoweredTableView(parent)
{
	/* Override default style. */
	setStyle(new AttachmentViewStyle(style()));

	setDragEnabled(true);
}

#if 0
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
#endif

#if 0
void AttachmentTableView::dragMoveEvent(QDragMoveEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	event->acceptProposedAction();
}
#endif

void AttachmentTableView::dropEvent(QDropEvent *event)
{
	/*
	 * Don't know what's happening.
	 * The default view/mode behaviour is somewhat strange.
	 * When dragging data originating from the model, if the model
	 * signals elements added while processing the event then the original
	 * data are not removed.
	 */

	/* TODO -- Access the model in a more transparent way. */
	AttachmentTblModel *attachmentModel =
	    qobject_cast<AttachmentTblModel *>(model());
	if (attachmentModel == Q_NULLPTR) {
		return;
	}

	bool dropOriginatesHere = (event->source() == this);
	if (!dropOriginatesHere) {
		LoweredTableView::dropEvent(event);
	} else {
		shuffleOnDrop(event);
	}
}

#if 0
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
	AttachmentTblModel *attachmentModel =
	    qobject_cast<AttachmentTblModel *>(model());
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
#endif

#if 0
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
#endif

void AttachmentTableView::shuffleOnDrop(QDropEvent *event)
{
	Q_ASSERT(event->source() == this);

	/* TODO -- Access the model in a more transparent way. */
	AttachmentTblModel *attachmentModel =
	    qobject_cast<AttachmentTblModel *>(model());
	if (attachmentModel == Q_NULLPTR) {
		Q_ASSERT(0);
		return;
	}

	int row = -1;
	int col = -1;
	QModelIndex dropIndex;
	if (!dropOn(event, &row, &col, &dropIndex)) {
		return;
	}

	if (dropIndex.isValid()) {
		/* Must be dropped on root. */
		return;
	}

	if (row < 0) {
		/* Dropping on root is treated as dropping on end. */
		row = attachmentModel->rowCount();
	}

	const QModelIndexList selectedSorted(
	    AttachmentTblModel::sortedUniqueLineIndexes(
	        selectionModel()->selectedIndexes(), 0));

	if (selectedSorted.isEmpty()) {
		return;
	}

	int firstRow = selectedSorted.first().row();
	int count = selectedSorted.size();
	int lastRow = selectedSorted.last().row();
	if ((lastRow - firstRow + 1) > count) {
		/* TODO -- Allow discontinuous selection. */
		logWarningNL("%s", "Discontinuous selection, aborting.");
		return;
	}

	attachmentModel->moveRows(QModelIndex(), firstRow, count,
	    QModelIndex(), row);

}

bool AttachmentTableView::droppingOnItself(QDropEvent *event,
    const QModelIndex &index)
{
	Qt::DropAction dropAction = event->dropAction();
	if (dragDropMode() == QAbstractItemView::InternalMove) {
		dropAction = Qt::MoveAction;
	}
	if ((event->source() == this) &&
	    (event->possibleActions() & Qt::MoveAction) &&
	    (dropAction == Qt::MoveAction)) {
		QModelIndexList selectedIdxs = selectedIndexes();
		QModelIndex child = index;
		while (child.isValid() && child != rootIndex()) {
			if (selectedIdxs.contains(child)) {
				return true;
			}
			child = child.parent();
		}
	}
	return false;
}

bool AttachmentTableView::dropOn(QDropEvent *event, int *dropRow, int *dropCol,
    QModelIndex *dropIndex)
{
	/* Inspired y Qt sources. */

	if (event->isAccepted()) {
		return false;
	}

	QModelIndex index;
	/* rootIndex() (i.e. the viewport) might be a valid index. */
	if (viewport()->rect().contains(event->pos())) {
		index = indexAt(event->pos());
		if (!index.isValid() || !visualRect(index).contains(event->pos())) {
			index = rootIndex();
		}
	}

	/* If we are allowed to do the drop. */
	if (model()->supportedDropActions() & event->dropAction()) {
		int row = -1;
		int col = -1;
		if (index != rootIndex()) {
			//dropIndicatorPosition = position(event->pos(), visualRect(index), index);
			//switch (dropIndicatorPosition) {
			switch (position(event->pos(), visualRect(index), index)) {
			case QAbstractItemView::AboveItem:
				row = index.row();
				col = index.column();
				index = index.parent();
				break;
			case QAbstractItemView::BelowItem:
				row = index.row() + 1;
				col = index.column();
				index = index.parent();
				break;
			case QAbstractItemView::OnItem:
			case QAbstractItemView::OnViewport:
				break;
			}
		} else {
			//dropIndicatorPosition = QAbstractItemView::OnViewport;
		}
		*dropIndex = index;
		*dropRow = row;
		*dropCol = col;
		if (!droppingOnItself(event, index)) {
			return true;
		}
	}
	return false;
}

QAbstractItemView::DropIndicatorPosition AttachmentTableView::position(
    const QPoint &pos, const QRect &rect, const QModelIndex &index) const
{
	QAbstractItemView::DropIndicatorPosition r =
	    QAbstractItemView::OnViewport;
	if (!dragDropOverwriteMode()) {
		const int margin = 2;
		if (pos.y() - rect.top() < margin) {
			r = QAbstractItemView::AboveItem;
		} else if (rect.bottom() - pos.y() < margin) {
			r = QAbstractItemView::BelowItem;
		} else if (rect.contains(pos, true)) {
			r = QAbstractItemView::OnItem;
		}
	} else {
		QRect touchingRect = rect;
		touchingRect.adjust(-1, -1, 1, 1);
		if (touchingRect.contains(pos, false)) {
			r = QAbstractItemView::OnItem;
		}
	}

	if ((r == QAbstractItemView::OnItem) &&
	    (!(model()->flags(index) & Qt::ItemIsDropEnabled))) {
		r = (pos.y() < rect.center().y()) ?
		    QAbstractItemView::AboveItem : QAbstractItemView::BelowItem;
	}

    return r;
}
