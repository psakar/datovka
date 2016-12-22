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

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>

#include "src/io/filesystem.h"
#include "src/io/message_db_set_container.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/models/files_model.h"

/*!
 * @brief Returns single line selection index.
 *
 * @param[in] view View to ask for selected indexes.
 * @param[in] column Number of column to receive indexes with.
 * @return Valid selected index, or invalid index if no selection or error.
 */
static
QModelIndex selectedSingleIndex(const AttachmentTableView &view, int column)
{
	QModelIndex index;

	QModelIndexList indexes(
	    AttachmentInteraction::selectedColumnIndexes(view, column));
	if (indexes.size() == 1) {
		return indexes.at(0);
	}

	return QModelIndex();
}

bool AttachmentInteraction::openAttachment(QWidget *parent,
    const AttachmentTableView &view, QModelIndex index,
    QString *attName, QString *tmpPath)
{
	if (attName != Q_NULLPTR) {
		attName->clear();
	}
	if (tmpPath != Q_NULLPTR) {
		tmpPath->clear();
	}

	if (index.isValid()) {
		if (index.column() != DbFlsTblModel::FNAME_COL) {
			index = index.sibling(index.row(),
			    DbFlsTblModel::FNAME_COL);
		}
	} else {
		/* Determine selection. */
		index = selectedSingleIndex(view, DbFlsTblModel::FNAME_COL);
		if (!index.isValid()) {
			return false;
		}
	}

	if (!index.isValid()) {
		return false;
	}
	Q_ASSERT(index.column() == DbFlsTblModel::FNAME_COL);

	QString attachName(index.data().toString());
	if (attachName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}
	if (attName != Q_NULLPTR) {
		*attName = attachName;
	}

	attachName.replace(QRegExp("\\s"), "_").replace(
	    QRegExp("[^a-zA-Z\\d\\.\\-_]"), "x");
	/* TODO -- Add message id into file name? */
	QString fileName(TMP_ATTACHMENT_PREFIX + attachName);

	QByteArray data;
	{
		/* Get data from base64. */
		QModelIndex dataIndex(
		    index.sibling(index.row(), DbFlsTblModel::CONTENT_COL));
		if (!dataIndex.isValid()) {
			Q_ASSERT(0);
			return false;
		}
		data = QByteArray::fromBase64(dataIndex.data().toByteArray());
	}

	QString writtenFileName = writeTemporaryFile(fileName, data);
	if (!writtenFileName.isEmpty()) {
		if (tmpPath != Q_NULLPTR) {
			*tmpPath = writtenFileName;
		}
		return QDesktopServices::openUrl(
		    QUrl::fromLocalFile(writtenFileName));
	} else {
		QMessageBox::warning(parent,
		    tr("Error storing attachment."),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
		return false;
	}
}

QString AttachmentInteraction::saveAttachmentToFile(QWidget *parent,
    QModelIndex index, const QString &suggestedFilePath)
{
	if (!index.isValid()) {
		return QString();
	}

	if (index.column() != DbFlsTblModel::FNAME_COL) {
		index = index.sibling(index.row(), DbFlsTblModel::FNAME_COL);
	}

	if (!index.isValid()) {
		Q_ASSERT(0);
		return QString();
	}
	Q_ASSERT(index.column() == DbFlsTblModel::FNAME_COL);

	QString fileName;
	if (suggestedFilePath.isEmpty()) {
		fileName = index.data().toString();
		if (fileName.isEmpty()) {
			Q_ASSERT(0);
			return QString();
		}
	} else {
		fileName = suggestedFilePath;
	}

	fileName = QFileDialog::getSaveFileName(parent, tr("Save attachment"),
	    fileName);
	if (fileName.isEmpty()) {
		return QString();
	}
	/* TODO -- Remember directory? */

	QModelIndex dataIndex(index.sibling(index.row(),
	    DbFlsTblModel::CONTENT_COL));
	if (!dataIndex.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	QByteArray data(QByteArray::fromBase64(dataIndex.data().toByteArray()));

	if (WF_SUCCESS != writeFile(fileName, data)) {
		QMessageBox::warning(parent,
		    tr("Error saving attachment."),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
		return QString();
	}

	return fileName;
}

void AttachmentInteraction::saveAttachmentsToFile(QWidget *parent,
    const AttachmentTableView &view, QModelIndexList indexList)
{
	if (indexList.isEmpty()) {
		indexList = selectedColumnIndexes(view,
		    DbFlsTblModel::FNAME_COL);
	}

	if (indexList.isEmpty()) {
		return;
	}

	foreach (const QModelIndex &index, indexList) {
		saveAttachmentToFile(parent, index);
	}
}
