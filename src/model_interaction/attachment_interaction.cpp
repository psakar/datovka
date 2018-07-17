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

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>

#include "src/io/filesystem.h"
#include "src/io/message_db_set_container.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/models/attachments_model.h"

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

/*!
 * @brief Creates a temporary file for data stored in attachment.
 *
 * @param[in] index Index identifying the attachment data.
 * @return Path to written temporary file, empty string on error.
 */
static
QString createTemporaryFile(QModelIndex index)
{
	if (!index.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	if (index.column() != AttachmentTblModel::FNAME_COL) {
		index = index.sibling(index.row(),
		    AttachmentTblModel::FNAME_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			return QString();
		}
	}
	Q_ASSERT(index.column() == AttachmentTblModel::FNAME_COL);

	QString attachName(index.data().toString());
	if (attachName.isEmpty()) {
		Q_ASSERT(0);
		return QString();
	}

	attachName.replace(QRegExp("\\s"), "_").replace(
	    QRegExp("[^a-zA-Z\\d\\.\\-_]"), "x");
	/* TODO -- Add message id into file name? */
	QString fileName(TMP_ATTACHMENT_PREFIX + attachName);

	QByteArray data;
	{
		/* Get binary data. */
		QModelIndex dataIndex(index.sibling(index.row(),
		    AttachmentTblModel::BINARY_CONTENT_COL));
		if (!dataIndex.isValid()) {
			Q_ASSERT(0);
			return QString();
		}
		data = dataIndex.data().toByteArray();
	}

	return writeTemporaryFile(fileName, data);
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
		if (index.column() != AttachmentTblModel::FNAME_COL) {
			index = index.sibling(index.row(),
			    AttachmentTblModel::FNAME_COL);
		}
	} else {
		/* Determine selection. */
		index = selectedSingleIndex(view,
		    AttachmentTblModel::FNAME_COL);
		if (!index.isValid()) {
			return false;
		}
	}

	if (!index.isValid()) {
		return false;
	}
	Q_ASSERT(index.column() == AttachmentTblModel::FNAME_COL);

	QString attachName(index.data().toString());
	if (attachName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}
	if (attName != Q_NULLPTR) {
		*attName = attachName;
	}

	/* Obtain plain file name from model if contains a file name. */
	QString fileName(
	    index.sibling(index.row(), AttachmentTblModel::FPATH_COL)
	        .data(ROLE_PLAIN_DISPLAY).toString());
	if (fileName.isEmpty()) {
		fileName = createTemporaryFile(index);
	}
	if (!fileName.isEmpty()) {
		if (tmpPath != Q_NULLPTR) {
			*tmpPath = fileName;
		}
		return QDesktopServices::openUrl(
		    QUrl::fromLocalFile(fileName));
	} else {
		QMessageBox::warning(parent,
		    tr("Error storing attachment."),
		    tr("Cannot write temporary file for attachment '%1'.")
		        .arg(attachName),
		    QMessageBox::Ok);
		return false;
	}
}

QString AttachmentInteraction::saveAttachmentToFile(QWidget *parent,
    QModelIndex index, const QString &suggestedFilePath, bool askLocation)
{
	if (!index.isValid()) {
		return QString();
	}

	if (index.column() != AttachmentTblModel::FNAME_COL) {
		index = index.sibling(index.row(),
		    AttachmentTblModel::FNAME_COL);
	}

	if (!index.isValid()) {
		Q_ASSERT(0);
		return QString();
	}
	Q_ASSERT(index.column() == AttachmentTblModel::FNAME_COL);

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

	if (askLocation) {
		fileName = QFileDialog::getSaveFileName(parent,
		    tr("Save attachment"), fileName);
	}
	if (fileName.isEmpty()) {
		return QString();
	}
	/* TODO -- Remember directory? */

	QModelIndex dataIndex(index.sibling(index.row(),
	    AttachmentTblModel::BINARY_CONTENT_COL));
	if (!dataIndex.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	const QByteArray data(dataIndex.data().toByteArray());

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
		    AttachmentTblModel::FNAME_COL);
	}

	if (indexList.isEmpty()) {
		return;
	}

	foreach (const QModelIndex &index, indexList) {
		saveAttachmentToFile(parent, index);
	}
}

/*!
 * @brief Generates a list of unique (within supplied list) attachment names.
 *
 * @param[in] indexList List of indexes referring to selected rows.
 * @return List of index-name pairs, empty list on error.
 */
static
QList< QPair<QModelIndex, QString> > renameAttachments(
    const QModelIndexList &indexList)
{
	typedef QPair<QModelIndex, QString> ListContent;
	typedef QList<ListContent> ListType;
	ListType pairList;
	QSet<QString> attNames;

	foreach (const QModelIndex &index, indexList) {
		QString attName;
		if (index.column() == AttachmentTblModel::FNAME_COL) {
			attName = index.data().toString();
		} else {
			attName = index.sibling(index.row(),
			    AttachmentTblModel::FNAME_COL).data().toString();
		}
		if (attName.isEmpty()) {
			Q_ASSERT(0);
			return ListType();
		}

		if (attNames.contains(attName)) {
			int cntr = 0;
			QFileInfo fi(attName);

			const QString baseName(fi.baseName());
			const QString suffix(fi.completeSuffix());

			do {
				++cntr;
				attName = baseName + "_" +
				    QString::number(cntr) + "." + suffix;
			} while (attNames.contains(attName));
			Q_ASSERT(!attName.isEmpty());
		}

		attNames.insert(attName);
		pairList.append(ListContent(index, attName));
	}

	return pairList;
}

QString AttachmentInteraction::saveAttachmentsToDirectory(QWidget *parent,
    const AttachmentTableView &view, QModelIndexList indexList,
    QString suggestedDirPath)
{
	if (indexList.isEmpty()) {
		indexList = selectedColumnIndexes(view,
		    AttachmentTblModel::FNAME_COL);
	}

	if (indexList.isEmpty()) {
		return QString();
	}

	typedef QPair<QModelIndex, QString> ListContent;
	typedef QList<ListContent> ListType;

	/* Sorted index list with unique attachment names. */
	ListType atts(renameAttachments(
	    AttachmentTblModel::sortedUniqueLineIndexes(indexList,
	        AttachmentTblModel::FNAME_COL)));
	if (atts.isEmpty()) {
		return QString();
	}

	suggestedDirPath = QFileDialog::getExistingDirectory(parent,
	    tr("Save attachments"), suggestedDirPath,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (suggestedDirPath.isEmpty()) {
		return QString();
	}

	ListType unsuccessfulAtts;
	bool existenceWarningDisplayed = false; /* Display only once. */

	foreach (const ListContent &att, atts) {
		const QModelIndex &idx(att.first);
		QString fileName(att.second);

		fileName = suggestedDirPath + QDir::separator() + fileName;

		bool fileExists = QFileInfo::exists(fileName);
		if (fileExists) {
			if (!existenceWarningDisplayed) {
				QMessageBox::warning(parent,
				    tr("Error saving attachment."),
				    tr("Some files already exist."),
				    QMessageBox::Ok);
				existenceWarningDisplayed = true;
			}
			/* Ask for file name. */
			fileName = QFileDialog::getSaveFileName(parent,
			    tr("Save attachment"), fileName);
			if (fileName.isEmpty()) {
				unsuccessfulAtts.append(att);
				continue;
			}
		}

		QModelIndex contIdx(idx.sibling(idx.row(),
		    AttachmentTblModel::BINARY_CONTENT_COL));

		const QByteArray binaryContent(contIdx.data().toByteArray());

		if (WF_SUCCESS != writeFile(fileName, binaryContent)) {
			unsuccessfulAtts.append(att);
			continue;
		}
	}

	if (!unsuccessfulAtts.isEmpty()) {
		QString warnMsg(
		    tr("In total %1 attachment files could not be written.").
		        arg(unsuccessfulAtts.size()));
		warnMsg += "\n" + tr("These are:") + "\n";
		int i;
		for (i = 0; i < (unsuccessfulAtts.size() - 1); ++i) {
			warnMsg += "    '" + unsuccessfulAtts.at(i).second + "'\n";
		}
		warnMsg += "    '" + unsuccessfulAtts.at(i).second + "'.";
		QMessageBox::warning(parent, tr("Error saving attachments."),
		    warnMsg, QMessageBox::Ok);
	}

	if (atts.size() == unsuccessfulAtts.size()) {
		return QString();
	}

	return suggestedDirPath;
}
