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

#ifndef _DATA_BOX_CONTACTS_MODEL_H_
#define _DATA_BOX_CONTACTS_MODEL_H_

#include "src/io/message_db.h"
#include "src/models/table_model.h"
#include "src/worker/task_search_owner.h"
#include "src/worker/task_search_owner_fulltext.h"

/*!
 * @brief List of data boxes and additional information.
 */
class BoxContactsModel : public TblModel {
	Q_OBJECT
public:
	/*!
	 * @brief Identifies the column index.
	 */
	enum ColumnNumbers {
		CHECKBOX_COL = 0, /* First column holds checkboxes. */
		BOX_ID_COL = 1, /* Data box identifier identifier. */
		BOX_TYPE_COL = 2, /* Data box type. */
		BOX_NAME_COL = 3, /* Data box name. */
		ADDRESS_COL = 4, /* Subject address. */
		POST_CODE_COL = 5, /* Postal code. */
		PDZ_COL = 6, /* Commercial message. */
		MAX_COL = 7 /* Number of columns. */
	};

	/*!
	 * @brief Used for identifying check state for entries.
	 */
	enum EntryState {
		UNCHECKED, /* Unchecked entries. */
		CHECKED, /* Checked entries. */
		ANY /* Unchecked and checked entries. */
	};

	/*!
	 * @brief Used for returning model entries.
	 */
	class PartialEntry {
	public:
		/*!
		 * @brief Constructor.
		 */
		PartialEntry(void)
		    : id(), name(), address(), pdz(false)
		{}

		QString id; /*!< Data box identifier. */
		QString name; /*!< Data box name. */
		QString address; /*!< Data box subject address. */
		bool pdz; /*!< True only if true held in the model. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit BoxContactsModel(QObject *parent = Q_NULLPTR);

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
	 * @brief Used for checkable elements.
	 *
	 * @param[in] index Index which to obtain flags for.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Used for changing the check state.
	 *
	 * @note Emits dataChanged signal.
	 *
	 * @param[in] index Index specifying the element.
	 * @param[in] value Value to be set.
	 * @param[in] role Specifies role of the modified data.
	 * @return True if check state was changed.
	 */
	virtual
	bool setData(const QModelIndex &index, const QVariant &value,
	    int role = Qt::EditRole) Q_DECL_OVERRIDE;

	/*!
	 * @brief Sets default header.
	 */
	void setHeader(void);

	/*!
	 * @brief Appends data into the model.
	 *
	 * @param[in] entryList List of entries to append into the model.
	 */
	void appendData(const QList<TaskSearchOwner::BoxEntry> &entryList);

	/*!
	 * @brief Appends data into the model.
	 *
	 * @param[in] entryList List of entries to append into the model.
	 */
	void appendData(
	    const QList<TaskSearchOwnerFulltext::BoxEntry> &entryList);

	/*!
	 * @brief Appends data into the model.
	 *
	 * @param[in] entryList List of entries to append into the model.
	 */
	void appendData(const QList<MessageDb::ContactEntry> &entryList);

	/*!
	 * @brief Returns true if there are some items checked.
	 *
	 * @return True if something checked.
	 */
	bool somethingChecked(void) const;

	/*!
	 * @brief Returns true if model contains supplied data box identifier.
	 *
	 * @oaram[in] boxId Data box identifier.
	 * @return True if model contains provided identifier.
	 */
	bool containsBoxId(const QString &boxId) const;

	/*!
	 * @brief Returns list of box identifiers according to required state.
	 *
	 * @param[in] entryState Specifies the checked state of required data.
	 * @return List of data box identifiers.
	 */
	QStringList boxIdentifiers(enum EntryState entryState) const;

	/*!
	 * @brief Returns list of entries according to required state.
	 *
	 * @Param[in] entryState Specifies the checked state of required data.
	 * @return List of partial entries.
	 */
	QList<PartialEntry> partialBoxEntries(enum EntryState entryState) const;
};

#endif /* _DATA_BOX_CONTACTS_MODEL_H_ */
