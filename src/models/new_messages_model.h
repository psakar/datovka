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

#ifndef _NEW_MESSAGES_MODEL_H_
#define _NEW_MESSAGES_MODEL_H_

#include <QAbstractTableModel>
#include <QMap>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>

/*!
 * @brief Custom message model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 *
 * @note setItemDelegate and a custom ItemDelegate would also be the solution.
 */
class NewDbMsgsTblModel : public QAbstractTableModel {
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
	 * @param[in]     type   Type of the table model.
	 * @param[parent] parent Parent object.
	 */
	NewDbMsgsTblModel(enum Type type = WORKING, QObject *parent = 0);

	/*!
	 * @brief Returns number of rows under given parent.
	 *
	 * @param[in] parent Parent index.
	 * @return Number of rows.
	 */
	virtual
	int rowCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Returns number of columns (for the children of given parent).
	 *
	 * @param[in] parent Parent index.
	 * @return Number of columns.
	 */
	virtual
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Returns the data stored under the given role.
	 *
	 * @param[in] index Position.
	 * @param[in] role  Role if the position.
	 * @return Data or invalid QVariant if no matching data found.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const;

	/*!
	 * @brief Sets header data for the given role and section.
	 *
	 * @param[in] section     Position.
	 * @param[in] orientation Orientation of the header.
	 * @param[in] value       Value to be set.
	 * @param[in] role        Role of the data.
	 * @return True when data have been set successfully.
	 */
	virtual
	bool setHeaderData(int section, Qt::Orientation orientation,
	    const QVariant &value, int role = Qt::EditRole);

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
	    int role = Qt::DisplayRole) const;

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
	 * @brief Sets the content of the model according to the supplied query.
	 *
	 * @param[in,out] qyery SQL query result.
	 */
	void setQuery(QSqlQuery &query);

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

private:
	/*!
	 * @brief Returns raw data stored under the given role.
	 *
	 * @param[in] index Position.
	 * @param[in] role  Role if the position (accepts only display role).
	 * @return Data or invalid QVariant if no matching data found.
	 */
	QVariant _data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const;

	/*!
	 * @brief Returns raw header data as they have been stored.
	 *
	 * @param[in] section     Position.
	 * @param[in] orientation Orientation of the header (is ignored).
	 * @param[in] role        Role of the data.
	 * @return Data or invalid QVariant in no matching data found.
	 */
	inline
	QVariant _headerData(int section, Qt::Orientation orientation,
	    int role = Qt::DisplayRole) const;

	/*
	 * Maps are organised in this order from outside:
	 * key[section] -> key[role] -> value[data]
	 * Orientation is ignored.
	 */
	QMap< int, QMap<int, QVariant> > m_headerData; /*!< Header data. */

	/*
	 * Data are organised using a two-dimensional array.
	 * The size of the array is incremented using several lines at once.
	 */
	QVector< QVector<QVariant> > m_data; /*!< Model data. */
	int m_rowsAllocated; /*!< Number of rows allocated. */
	static
	const int m_rowAllocationIncrement; /*!<
	                                     * Number of lines to be added
	                                     * in a single resize attempt.
	                                     */

	int m_rowCount; /*!< Number of used rows.*/
	int m_columnCount; /*!< Number of columns. */

	Type m_type; /*!< Whether this is a model dummy or contains data. */
};

#endif /* _NEW_MESSAGES_MODEL_H_ */
