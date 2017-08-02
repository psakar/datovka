/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#ifndef _TABLE_MODEL_H_
#define _TABLE_MODEL_H_

#include <QAbstractTableModel>
#include <QMap>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>

class TblModel : public QAbstractTableModel {
    Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit TblModel(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Returns number of rows under given parent.
	 *
	 * @param[in] parent Parent index.
	 * @return Number of rows.
	 */
	virtual
	int rowCount(const QModelIndex &parent = QModelIndex()) const
	    Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns number of columns (for the children of given parent).
	 *
	 * @param[in] parent Parent index.
	 * @return Number of columns.
	 */
	virtual
	int columnCount(const QModelIndex &parent = QModelIndex()) const
	    Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns the data stored under the given role.
	 *
	 * @param[in] index Position.
	 * @param[in] role  Role if the position.
	 * @return Data or invalid QVariant if no matching data found.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const Q_DECL_OVERRIDE = 0;

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
	    const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

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
	 * @brief Move rows.
	 *
	 * @param[in] sourceParent Source parent.
	 * @param[in] sourceRow Source row.
	 * @param[in] count Number of rows to be moved.
	 * @param[in] destinationParent Destination parent.
	 * @param[in] destinationChild Row to move data into.
	 * @return If move performed.
	 */
	virtual
	bool moveRows(const QModelIndex &sourceParent, int sourceRow,
	    int count, const QModelIndex &destinationParent,
	    int destinationChild) Q_DECL_OVERRIDE;

	/*!
	 * @brief Remove rows.
	 *
	 * @param[in] row Starting row.
	 * @param[in] count Number of rows to be removed.
	 * @param[in] parent Parent item the row is relative to.
	 * @return True if the rows were successfully removed.
	 */
	virtual
	bool removeRows(int row, int count,
	    const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;

protected:
	/*!
	 * @brief Sets the content of the model according to the supplied query.
	 *
	 * @param[in,out] qyery SQL query result.
	 */
	virtual
	void setQuery(QSqlQuery &query);

	/*!
	 * @brief Appends data from the supplied query to the model.
	 *
	 * @param[in,out] query SQL query result.
	 * @return True on success.
	 */
	virtual
	bool appendQueryData(QSqlQuery &query);

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
	 * @brief Returns raw data stored under the given role.
	 *
	 * @param[in] row   Position.
	 * @param[in] col   Position.
	 * @param[in] role  Role if the position (accepts only display role).
	 * @return Data or invalid QVariant if no matching data found.
	 */
	QVariant _data(int row, int col, int role = Qt::DisplayRole) const;

	/*!
	 * @brief Returns raw header data as they have been stored.
	 *
	 * @param[in] section     Position.
	 * @param[in] orientation Orientation of the header (is ignored).
	 * @param[in] role        Role of the data.
	 * @return Data or invalid QVariant in no matching data found.
	 */
	QVariant _headerData(int section, Qt::Orientation orientation,
	    int role = Qt::DisplayRole) const;

	/*!
	 * @brief Inflate data container.
	 */
	void reserveSpace(void);

	/*!
	 * @brief Copies query result into vector.
	 *
	 * @param[out] vect Vector to write data into.
	 * @param[in]  query Query to copy data from.
	 */
	static
	void queryToVector(QVector<QVariant> &vect, const QSqlQuery &query);

	/*
	 * Data are organised using a two-dimensional array.
	 * The size of the array is incremented using several lines at once.
	 */
	QVector< QVector<QVariant> > m_data; /*!< Model data. */
	int m_rowsAllocated; /*!< Number of rows allocated. */

	int m_rowCount; /*!< Number of used rows.*/
	int m_columnCount; /*!< Number of columns. */

	static
	const int m_rowAllocationIncrement; /*!<
	                                     * Number of lines to be added
	                                     * in a single resize attempt.
	                                     */

private:
	/*
	 * Maps are organised in this order from outside:
	 * key[section] -> key[role] -> value[data]
	 * Orientation is ignored.
	 */
	QMap< int, QMap<int, QVariant> > m_headerData; /*!< Header data. */
};

#endif /* _TABLE_MODEL_H_ */
