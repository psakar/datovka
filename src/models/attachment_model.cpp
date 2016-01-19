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

#include "src/io/isds_sessions.h"
#include "src/models/attachment_model.h"

const QVector<QString> AttachmentModel::m_headerLabels = {
	QString(),
	QString(),
	QString(),
	QObject::tr("File name"),
	QObject::tr("Size")
};

AttachmentModel::AttachmentModel(QObject *parent)
    : QAbstractTableModel(parent),
    m_docs()
{
}

AttachmentModel::~AttachmentModel(void)
{
}

Qt::ItemFlags AttachmentModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

	if (index.isValid()) {
		defaultFlags |= Qt::ItemIsDragEnabled;
	}

	return defaultFlags;
}

int AttachmentModel::rowCount(const QModelIndex &parent) const
{
	/* unused */
	(void) parent;

	return m_docs.size();
}

int AttachmentModel::columnCount(const QModelIndex &parent) const
{
	/* unused */
	(void) parent;

	return MAX_COL;
}

QVariant AttachmentModel::data(const QModelIndex &index, int role) const
{
	int row, col;

	switch (role) {
	case Qt::DisplayRole:
		row = index.row();
		col = index.column();
		Q_ASSERT(row < m_docs.size());
		Q_ASSERT(col < MAX_COL);

		switch (col) {
		case CONTENT_COL:
			Q_ASSERT(m_docs.size() > row);
			return QByteArray((char *) m_docs[row]->data,
			    (int) m_docs[row]->data_length).toBase64();
			break;
		case FNAME_COL:
			/* File name. */
			return QString(m_docs[row]->dmFileDescr);
			break;
		case FSIZE_COL:
			/* File size. */
			return QString::number(m_docs[row]->data_length);
			break;
		default:
			break;
		}

		return QVariant();
		break;
	case Qt::TextAlignmentRole:
		return Qt::AlignLeft;
		break;
	default:
		return QVariant();
		break;
	}
}

QVariant AttachmentModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
	(void) orientation; /* Unused. */

	if (role != Qt::DisplayRole) {
		return QVariant();
	}
	return m_headerLabels[section];
}

bool AttachmentModel::setModelData(const struct isds_message *message)
{
	const struct isds_list *docListItem;

	if (NULL == message) {
		Q_ASSERT(0);
		return false;
	}

	docListItem = message->documents;
	if (NULL == docListItem) {
		Q_ASSERT(0);
		return false;
	}

	this->beginResetModel();

	m_docs.clear();

	while (NULL != docListItem) {
		Q_ASSERT(NULL != docListItem->data);
		m_docs.append((struct isds_document *) docListItem->data);
		docListItem = docListItem->next;
	}

	this->endResetModel();

	return true;
}

QByteArray AttachmentModel::attachmentData(int indexRow) const
{
	Q_ASSERT(m_docs.size() > 0);

	QByteArray data((char *) m_docs[indexRow]->data,
	    (int) m_docs[indexRow]->data_length);

	return data;
}
