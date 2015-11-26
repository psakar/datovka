/*
 * Copyright (C) 2014-2015 CZ.NIC
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
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QMimeDatabase>
#include <QUrl>

#include "src/common.h"
#include "src/log/log.h"
#include "src/models/attachment_model.h"
#include "src/views/attachment_table_view.h"

AttachmentTableView::AttachmentTableView(QWidget *parent)
    : QTableView(parent),
    m_dragStartPosition()
{
	setDragEnabled(true);
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

	/* Create temporary directory. Automatic remove is on by default. */
	QTemporaryDir dir(QDir::tempPath() + QDir::separator() + TMP_DIR_NAME);
	if (!dir.isValid()) {
		logErrorNL("%s", "Could not create a temporary directory.");
		return;
	}
	/*
	 * Automatic removal cannot be set because his removes the files
	 * before actual copying of the file.
	 *
	 * TODO -- Represent the copied data in a different way than an URL.
	 */
	dir.setAutoRemove(false);

	QList<QString> tmpFileNames = temporaryFiles(dir);
	if (tmpFileNames.isEmpty()) {
		logErrorNL("%s", "Could not write temporary files.");
		return;
	}

	QDrag *drag = new (std::nothrow) QDrag(this);
	if (0 == drag) {
		return;
	}

	QMimeData *mimeData = new (std::nothrow) QMimeData;
	if (0 != mimeData) {
		QList<QUrl> urlList = temporaryFileUrls(tmpFileNames);
		mimeData->setUrls(urlList);
		drag->setMimeData(mimeData);
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

QList<QString> AttachmentTableView::temporaryFiles(
    const QTemporaryDir &tmpDir) const
{
	QList<QString> tmpFileList;
	int fileNumber = 0;
	const QString tmpPath(tmpDir.path());

	if (tmpPath.isEmpty()) {
		Q_ASSERT(0);
		return QList<QString>();
	}

	QModelIndexList firstMsgColumnIdxs =
	   this->selectionModel()->selectedRows(0);

	foreach (const QModelIndex &idx, firstMsgColumnIdxs) {
		QString subirPath(tmpPath + QDir::separator() +
		    QString::number(fileNumber++));
		{
			/*
			 * Create a separate subdirectory because the files
			 * may have equal names.
			 */
			QDir dir(tmpPath);
			if (!dir.mkpath(subirPath)) {
				logError("Could not create directory '%s'.",
				    subirPath.toUtf8().constData());
				return QList<QString>();
			}
		}
		QString attachAbsPath(subirPath + QDir::separator());
		{
			/* Determine full file path. */
			QModelIndex fileNameIndex = idx.sibling(idx.row(),
			    AttachmentModel::FNAME_COL);
			if(!fileNameIndex.isValid()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
			QString attachFileName(fileNameIndex.data().toString());
			if (attachFileName.isEmpty()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
			attachAbsPath += attachFileName;
		}
		QByteArray attachData;
		{
			/* Obtain data. */
			QModelIndex dataIndex = idx.sibling(idx.row(),
			    AttachmentModel::CONTENT_COL);
			if (!dataIndex.isValid()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
			attachData = QByteArray::fromBase64(
			    dataIndex.data().toByteArray());
			if (attachData.isEmpty()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
		}
		if (WF_SUCCESS != writeFile(attachAbsPath, attachData, false)) {
			return QList<QString>();
		}

		tmpFileList.append(attachAbsPath);
	}

	return tmpFileList;
}

QList<QUrl> AttachmentTableView::temporaryFileUrls(
    const QList<QString> &tmpFileNames)
{
	QList<QUrl> uriList;

	foreach (const QString &fileName, tmpFileNames) {
		uriList.append(QUrl::fromLocalFile(fileName));
	}

	return uriList;
}
