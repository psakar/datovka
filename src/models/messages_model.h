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

#pragma once

#include <QIcon>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVector>

#include "src/common.h" /* enum MessageProcessState */
#include "src/io/message_db.h"
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
	 *
	 * @Note Non-negative values are directly derived from the message
	 *     database. Columns identified by negative values are added and
	 *     are kept in separate database files.
	 *
	 * @todo Processing state should be separated from the message database
	 *     and should be kept in a separate database in a similar fashion
	 *     as tags or records management information.
	 */
	enum ColumnNumbers {
		DMID_COL = 0, /* Message identifier. */
		PERSDELIV_COL = 1, /* Personal delivery. */
		ANNOT_COL = 2, /* Annotation column. */
		SENDER_COL = 3, /* Sender name column. */
		RECIP_COL = 4, /* Recipient name column. */
		DELIVERY_COL = 5, /* Delivery time column. */
		ACCEPT_COL = 6, /* Acceptance time column. */
		READLOC_COL = 7, /* Read locally. */
		MSGSTAT_COL = 8, /* Message status. */
		ATTDOWN_COL = 9, /* Attachments downloaded. */
		PROCSNG_COL = 10, /* Processing state. */
		RECMGMT_NEG_COL = -2, /* 11 */ /* Records management service. */
		TAGS_NEG_COL = -1, /* 12 */ /* Tags. */
		BASIC_COLNUM = 11, /* Number of basic columns (convenience value). */
		MAX_COLNUM = 13 /* Maximal number of columns (convenience value). */
	};

	/*!
	 * @brief Specifies the type of the table model.
	 *
	 * @note Dummies are used to fake empty models.
	 */
	enum Type {
		RCVD_MODEL = 0, /*!< Model created from SQL query result. */
		SNT_MODEL /*!< Model created from SQL query result. */
	};

	/*!
	 * @brief Describes appended header entry.
	 */
	class AppendedCol {
	public:
		/*!
		 * @brief Constructor.
		 */
		explicit AppendedCol(const QString &dis,
		    const QIcon &dec = QIcon(), const QString &tip = QString())
		    : display(dis), decoration(dec), toolTip(tip)
		{
		}

		QString display; /*!< What to show on display role. */
		QIcon decoration; /*!< What to display on decoration role. */
		QString toolTip; /*!< What to display in tool tip. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] type   Type of the table model.
	 * @param[in] parent Parent object.
	 */
	explicit DbMsgsTblModel(enum Type type = RCVD_MODEL,
	    QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Returns the data stored under the given role.
	 *
	 * @param[in] index Position.
	 * @param[in] role  Role of the position.
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
	 * @brief Appends received entry data into the model.
	 *
	 * @param[in] entryList List of entries to append into the model.
	 * @param[in] appendedColsNum Number of added empty columns.
	 */
	void appendData(const QList<MessageDb::RcvdEntry> &entryList,
	    int appendedColsNum);

	/*!
	 * @brief Appends sent entry data into the model.
	 *
	 * @param[in] entryList List of entries to append into the model.
	 * @param[in] appendedColsNum Number of added empty columns.
	 */
	void appendData(const QList<MessageDb::SntEntry> &entryList,
	    int appendedColsNum);

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
	 * @brief Get model type.
	 *
	 * @return Model type.
	 */
	enum Type type(void) const;

	/*!
	 * @brief Set header data.
	 *
	 * @return False on error.
	 */
	bool setHeader(const QList<AppendedCol> &appendedCols);

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
	 * @note Emits dataChanged signal.
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
	 * @note Emits dataChanged signal.
	 *
	 * @param[in] dmId       Message id.
	 * @param[in] forceState Set forced value.
	 * @return True on success.
	 */
	bool overrideProcessing(qint64 dmId,
	    enum MessageProcessState forceState);

	/*!
	 * @brief Fills the model with tag information.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] col      Negative number specifying the column to write
	 *                     into.
	 * @return True on success.
	 */
	bool fillTagsColumn(const QString &userName, int col);

	/*!
	 * @brief Reload tags in given rows.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] dmIds    List of message ids for which to load tags.
	 * @param[in] col      Negative number specifying the column to write
	 *                     into.
	 * @return True on success.
	 */
	bool refillTagsColumn(const QString &userName,
	    const QList<qint64> &dmIds, int col);

	/*!
	 * @brief Sets records management service notification icon.
	 * @return True on success.
	 */
	bool setRecordsManagementIcon(void);

	/*!
	 * @brief Fills the model with records management service information.
	 *
	 * @param[in] col Negative number specifying the column to write into.
	 * @return True on success.
	 */
	bool fillRecordsManagementColumn(int col);

	/*!
	 * @brief Reload records management service information in given rows.
	 *
	 * @param[in] dmIds List of message ids for which to load tags.
	 * @param[in] col Negative number specifying the column to write into.
	 * @return True on success.
	 */
	bool refillRecordsManagementColumn(const QList<qint64> &dmIds, int col);

private:
	/*!
	 * @brief Return records management model data.
	 *
	 * @param[in] index Position.
	 * @param[in] role Role of the position.
	 * @return Data to be displayed.
	 */
	QVariant recMgmtData(const QModelIndex &index, int role) const;

	/*!
	 * @brief Return tags model data.
	 *
	 * @param[in] index Position.
	 * @param[in] role Role of the position.
	 * @return Data to be displayed.
	 */
	QVariant tagsData(const QModelIndex &index, int role) const;

	/*!
	 * @brief Construct a numeric sorting identifier.
	 *
	 * @param[in] num Small number with highest significance.
	 * @param[in] index Index which is used to take message identifier from.
	 *                  The message identifier has a low significance.
	 * @return A number that can be used for sorting purposes.
	 */
	qint64 sortRank(qint16 num, const QModelIndex &index) const;

	enum Type m_type; /*!< Specifies type of data held in the model. */

	QIcon m_rmIco; /*!< Records management icon. */
};
