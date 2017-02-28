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

#ifndef _COMBO_BOX_MODEL_H_
#define _COMBO_BOX_MODEL_H_

#include <QAbstractListModel>
#include <QList>
#include <QPair>

/*!
 * @brief Combo box model class.
 */
class CBoxModel : public QAbstractListModel {
	Q_OBJECT
public:
	static
	const int valueRole; /*!< Role number under which to return entry value. */

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit CBoxModel(QObject *parent = Q_NULLPTR);

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
	 * @brief Used to enable or disable elements.
	 *
	 * @param[in] index Index which to obtain flags for.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Adds an entry into the model.
	 *
	 * @param[in] label Description string displayed in the combo box.
	 * @param[in] val Value to hold within the entry.
	 * @param[in] enabled Whether the entry should be enabled or disabled.
	 */
	void appendRow(const QString &label, int val, bool enabled = true);

	/*!
	 * @brief Enables/disabled all fields with value \a val.
	 *
	 * @note Emits dataChanged signal.
	 *
	 * @param[in] val Value to search for.
	 * @param[in] enabled Whether to enable or disable the element.
	 */
	void setEnabled(int val, bool enabled);

	/*!
	 * @brief Find first row with given \a val that is enabled.
	 *
	 * @param[in] val Value to search for.
	 * @return Number of given row or -1 if not found or on error.
	 */
	int findRow(int val) const;

private:
	/*!
	 * @brief Describes model entries.
	 */
	class Entry {
	public:
		/*!
		 * @brief Constructor.
		 */
		Entry(const QString &l, int v, bool e)
		    : label(l), value(v), enabled(e)
		{}

		QString label; /*!< Text displayed in combo box. */
		int value; /*!< Value associated with the entry. */
		bool enabled; /*!< True if value is enabled. */
	};

	QList<Entry> m_entries; /*!< List of label and value pairs. */
};

#endif /* _COMBO_BOX_MODEL_H_ */
