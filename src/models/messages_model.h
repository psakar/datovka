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

#ifndef _MESSAGES_MODEL_H_
#define _MESSAGES_MODEL_H_

#include <QString>
#include <QVariant>
#include <QVector>

#include "src/common.h" /* enum MessageProcessState */
#include "src/models/table_model.h"

/*!
 * @brief Custom message model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 *
 * @note setItemDelegate and a custom ItemDelegate would also be the solution.
 */
class DbMsgsTblModel : public TblModel {
    Q_OBJECT

public:
	/*!
	 * @brief Identifies the column index.
	 */
	enum ColumnNumbers {
		DMID_COL = 0, /* Message identifier. */
		ANNOT_COL = 1, /* Annotation column. */
		DELIVERY_COL = 3, /* Delivery time column. */
		ACCEPT_COL = 4, /* Acceptance time column. */
		READLOC_COL = 5, /* Read locally. */
		ATTDOWN_COL = 6, /* Attachment downloaded. */
		PROCSNG_COL = 7 /* Processing state. */
	};

	/*!
	 * @brief Specifies the type of the table model.
	 *
	 * @note Dummies are used to fake empty models.
	 */
	enum Type {
		WORKING = 0, /*!< Ordinary model created from SQL query. */
		DUMMY_RECEIVED, /*!< Empty received dummy. */
		DUMMY_SENT /*!< Empty sent dummy. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] type   Type of the table model.
	 * @param[in] parent Parent object.
	 */
	DbMsgsTblModel(enum Type type = WORKING, QObject *parent = 0);

	/*!
	 * @brief Returns the data stored under the given role.
	 *
	 * @param[in] index Position.
	 * @param[in] role  Role if the position.
	 * @return Data or invalid QVariant if no matching data found.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Obtains header data.
	 *
	 * @param[in] section     Position.
	 * @param[in] orientation Orientation of the header.
	 * @param[in] role        Role of the data.
	 * @return Data or invalid QVariant in no matching data found.
	 */
	virtual
	QVariant headerData(int section, Qt::Orientation orientation,
	    int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Sets the type of the model.
	 *
	 * @note Also sets the headers according to the dummy type.
	 *
	 * @param[in] type Type of the table model.
	 * @return True if everything has been set successfully.
	 */
	bool setType(enum Type type);

	/*!
	 * @brief Singleton method returning received column identifiers.
	 *
	 * @return List of received column identifiers.
	 */
	static
	const QVector<QString> &rcvdItemIds(void);

	/*!
	 * @brief Singleton method returning sent column identifiers.
	 *
	 * @return List of sent column identifiers.
	 */
	static
	const QVector<QString> &sntItemIds(void);

	/*!
	 * @brief Set header data for received model.
	 *
	 * @return False on error.
	 */
	bool setRcvdHeader(void);

	/*!
	 * @Brief Set header data for sent model.
	 *
	 * @return False on error.
	 */
	bool setSntHeader(void);

	/*!
	 * @brief Override message as being read.
	 *
	 * @note Emits dataChanged signal.
	 *
	 * @param[in] dmId      Message id.
	 * @param[in] forceRead Set whether to force read state.
	 * @return True on success.
	 */
	bool overrideRead(qint64 dmId, bool forceRead = true);

	/*!
	 * @brief Override message as having its attachments having downloaded.
	 *
	 * note Emits dataChanged signal.
	 *
	 * @param[in] dmId            Message id.
	 * @param[in] forceDownloaded Set whether to force attachments
	 *                            downloaded state.
	 * @return True on success.
	 */
	bool overrideDownloaded(qint64 dmId, bool forceDownloaded = true);

	/*!
	 * @brief Override message processing state.
	 *
	 * note Emits dataChanged signal.
	 *
	 * @param[in] dmId       Message id.
	 * @param[in] forceState Set forced value.
	 * @return True on success.
	 */
	bool overrideProcessing(qint64 dmId,
	    enum MessageProcessState forceState);

	/*!
	 * @brief Returns reference to a dummy model.
	 *
	 * @note Beware of the static initialization order fiasco.
	 *
	 * @param[in] type Type of the table model.
	 * @returns Reference to a static dummy model.
	 */
	static
	DbMsgsTblModel &dummyModel(enum Type type);

private:
	enum Type m_type; /*!<
	                   * Whether this is a model dummy or contains data.
	                   */
};

#endif /* _MESSAGES_MODEL_H_ */
