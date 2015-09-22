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

#ifndef _MESSAGES_MODEL_H_
#define _MESSAGES_MODEL_H_

#include <QMap>
#include <QModelIndex>
#include <QObject>
#include <QSqlQueryModel>
#include <QVariant>

#include "src/common.h"

/*!
 * @brief Custom message model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 *
 * @note setItemDelegate and a custom ItemDelegate would also be the solution.
 */
class DbMsgsTblModel : public QSqlQueryModel {
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
		PROCSNG_COL = 7  /* Processing state. */
	};

	/*!
	 * @brief Dummies are used to fake empty models.
	 */
	enum Type {
		WORKING = 0, /*!< Ordinary model created from SQL query. */
		DUMMY_RECEIVED, /*!< Empty received dummy. */
		DUMMY_SENT /*!< Empty sent dummy. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] type   Type of the model.
	 * @param[in] parent Parent.
	 */
	DbMsgsTblModel(enum Type type = WORKING, QObject *parent = 0);

	/*!
	 * @brief Sets the type of the model.
	 *
	 * @paran[in] type Model type.
	 */
	virtual void setType(enum Type type);

	/*!
	 * @brief Returns fake column count for dummy models.
	 *
	 * @param[in] index Parent index.
	 * @return Column count.
	 */
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Returns fake row count for dummy models.
	 *
	 * @param[in] parent Parent index.
	 * @return Row count.
	 */
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Convert viewed data in date/time columns.
	 *
	 * @param[in] index Item index.
	 * @param[in] role  Display role.
	 * @return Data modified according to the given role.
	 */
	virtual QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const;

	/*!
	 * @brief Convert viewed header data.
	 *
	 * @param[in] section     Section index.
	 * @param[in] orientation Header orientation.
	 * @param[in] role Display role.
	 * @return Header data modified according to the given role.
	 */
	virtual QVariant headerData(int section, Qt::Orientation orientation,
	    int role) const;

	/*!
	 * @brief Override message as being read.
	 *
	 * @param[in] dmId      Message id.
	 * @param[in] forceRead Set whether to force read state.
	 * @return True on success.
	 */
	virtual bool overrideRead(qint64 dmId, bool forceRead = true);

	/*!
	 * @brief Override message as having its attachments having downloaded.
	 *
	 * @param[in] dmId            Message id.
	 * @param[in] forceDownloaded Set whether to force attachments
	 *                            downloaded state.
	 * @return True on success.
	 */
	virtual bool overrideDownloaded(qint64 dmId,
	    bool forceDownloaded = true);

	/*!
	 * @brief Override message processing state.
	 *
	 * @param[in] dmId       Message id.
	 * @param[in] forceState Set forced value.
	 * @return True on success.
	 */
	virtual bool overrideProcessing(qint64 dmId,
	    enum MessageProcessState forceState);

	/*!
	 * @brief Clear all overriding data.
	 */
	virtual void clearOverridingData(void);

	/*!
	 * @brief Set header data for received model.
	 */
	bool setRcvdHeader(void);

	/*!
	 * @brief Set header data for sent model.
	 */
	bool setSntHeader(void);

	/* Methods behaving as singletons. */
	static
	const QVector<QString> &rcvdItemIds(void);
	static
	const QVector<QString> &sntItemIds(void);

	/*
	 * Beware of the static initialization order fiasco.
	 */
	static
	DbMsgsTblModel &dummyModel(void); /*!< Dummy model. */

	/*
	 * The view's proxy model cannot be accessed, so the message must be
	 * addressed via its id rather than using the index.
	 */
private:
	QMap<qint64, bool> m_overriddenRL; /*!<
	                                    * Holds overriding information for
	                                    * read locally.
	                                    */
	QMap<qint64, bool> m_overriddenAD; /*!<
	                                    * Holds overriding information for
	                                    * downloaded attachments.
	                                    */
	QMap<qint64, int> m_overriddenPS; /*!<
	                                   * Holds overriding information for
	                                   * message processing state.
	                                   */
	Type m_type; /*!< Model type. */
};

#endif /* _MESSAGES_MODEL_H_ */
