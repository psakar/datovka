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

#ifndef _ATTACHMENT_MODEL_H_
#define _ATTACHMENT_MODEL_H_

#include <QAbstractTableModel>

#include "src/io/isds_sessions.h"

#define COL_NUM 2
#define FNAME_COL 0
#define FSIZE_COL 1

/*!
 * @brief Custom attachment table model class.
 */
class AttachmentModel : public QAbstractTableModel {
	Q_OBJECT /* Not supported for nested classes. */

public:
	/*!
	 * @brief Constructor.
	 */
	AttachmentModel(QObject *parent = 0);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~AttachmentModel(void);

	/*!
	 * @brief Returns row count.
	 */
	virtual
	int rowCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Returns column count.
	 */
	virtual
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Returns data.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const;

	/*!
	 * @brief Returns header data.
	 */
	virtual
	QVariant headerData(int section, Qt::Orientation orientation,
	    int role) const;

	/*!
	 * @brief Set attachment model according to message content.
	 *
	 * @param[in] message Pointer to ISDS message.
	 * @return True on success.
	 */
	bool setModelData(const isds_message *message);

	/*!
	 * @brief Get attachment content.
	 */
	QByteArray attachmentData(int indexRow) const;

private:
	static
	const QVector<QString> m_headerLabels; /*!< Header labels. */

	QVector<const struct isds_document *> m_docs; /*!< Pointers. */
};

#endif /* _ATTACHMENT_MODEL_H_ */
