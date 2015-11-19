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

#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QMimeDatabase>

#include "src/common.h"
#include "src/log/log.h"
#include "src/views/attachment_table_widget.h"

AttachmentTableWidget::AttachmentTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
	logDebugLv0NL("%s", "acept drops");
	setAcceptDrops(true);
}

int AttachmentTableWidget::addFile(const QString &filePath)
{
	QFile attFile(filePath);

	int fileSize = attFile.size();
	if (0 == fileSize) {
		logWarning("Ignoring file '%s' with zero size.\n",
		    filePath.toUtf8().constData());
		return 0;
	}

	QString fileName(QFileInfo(attFile.fileName()).fileName());
	QMimeType mimeType = QMimeDatabase().mimeTypeForFile(attFile);

	int row = this->rowCount();

	for (int i = 0; i < row; ++i) {
		if (this->item(i, ATW_FILE)->text() == fileName) {
			/* Already in table. */
			return -1;
		}
	}

	this->insertRow(row);

	QTableWidgetItem *item = new QTableWidgetItem;
	item->setText(fileName);
	this->setItem(row, ATW_FILE, item);
	item = new QTableWidgetItem;
	item->setText("");
	this->setItem(row, ATW_TYPE, item);
	item = new QTableWidgetItem;
	item->setText(mimeType.name());
	this->setItem(row, ATW_MIME, item);
	item = new QTableWidgetItem;
	item->setText(QString::number(fileSize));
	this->setItem(row, ATW_SIZE, item);
	item = new QTableWidgetItem;
	item->setText(filePath);
	this->setItem(row, ATW_PATH, item);
	item = new QTableWidgetItem;
	item->setData(Qt::DisplayRole, getFileBase64(fileName));
	this->setItem(row, ATW_DATA, item);

	return fileSize;
}

void AttachmentTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("%s", "A001");

	if (event->mimeData()->hasUrls()) {
		logDebugLv0NL("%s", "A002");
		event->acceptProposedAction();
	}

	logDebugLv0NL("%s", "A003");
}

void AttachmentTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("%s", "B001");

	event->acceptProposedAction();
}

void AttachmentTableWidget::dropEvent(QDropEvent *event)
{
	if (0 == event) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("%s", "C001");

	if (!event->mimeData()->hasUrls()) {
		return;
	}

	logDebugLv0NL("%s", "C002");

	QList<QString> paths = filePaths(event->mimeData()->urls());

	foreach (const QString &filePath, paths) {
		addFile(filePath);
	}
}

QList<QString> AttachmentTableWidget::filePaths(const QList<QUrl> &uriList)
{
	QList<QString> filePaths;

	foreach (const QUrl &uri, uriList) {
		if (!uri.isValid()) {
			logErrorNL("Dropped invalid URL '%s'.",
			    uri.toString().toUtf8().constData());
			return QList<QString>();
		}

		if (!uri.isLocalFile()) {
			logErrorNL("Dropped URL '%s' is not a local file.",
			    uri.toString().toUtf8().constData());
			return QList<QString>();
		}

		logDebugLv0NL("%s", uri.toLocalFile().toUtf8().constData());

		filePaths.append(uri.toLocalFile());
	}

	return filePaths;
}

QByteArray AttachmentTableWidget::getFileBase64(const QString &filePath)
 {
	QFile file(filePath);
	if (file.exists()) {
		if (!file.open(QIODevice::ReadOnly)) {
			logErrorNL("Could not open file '%s'.",
			    filePath.toUtf8().constData());
			goto fail;
		}
		return file.readAll().toBase64();
	}
fail:
	return QByteArray();
}
