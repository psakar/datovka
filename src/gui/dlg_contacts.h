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

#include <QDialog>
#include <QString>

#include "src/io/message_db_set.h"
#include "src/models/data_box_contacts_model.h"
#include "src/models/sort_filter_proxy_model.h"

namespace Ui {
	class DlgContacts;
}

/*!
 * @brief List of contacts generated from message database.
 */
class DlgContacts : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in]  dbSet Database container.
	 * @param[out] dbIdList List of selected box identifiers to be set.
	 * @param[in]  parent Parent object.
	 */
	DlgContacts(const MessageDbSet &dbSet,
	    QStringList *dbIdList = Q_NULLPTR, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgContacts(void);

	/*!
	 * @brief Let the user select from contacts stored in database.
	 *
	 * @param[in]  dbSet Database container.
	 * @param[out] dbIdList List of selected box identifiers to be set.
	 * @param[in]  parent Parent object.
	 * @return True if dialogue was accepted.
	 */
	static
	bool selectContacts(const MessageDbSet &dbSet,
	    QStringList *dbIdList = Q_NULLPTR, QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Activates confirmation button.
	 */
	void enableOkButton(void);

	/*!
	 * @brief Set first column containing checkboxes active.
	 *
	 * @param[in] selected Newly selected items.
	 * @param[in] deselected Deselected items.
	 */
	void setFirstColumnActive(const QItemSelection &selected,
	    const QItemSelection &deselected);

	/*!
	 * @brief Makes a selection and closes the dialogue.
	 */
	void contactItemDoubleClicked(const QModelIndex &index);

	/*!
	 * @brief Appends selected box identifiers into identifier list.
	 */
	void addSelectedDbIDs(void) const;

	/*!
	 * @brief Apply filter text on the table.
	 */
	void filterContact(const QString &text);

private:
	/*!
	 * @brief Get contacts from message database and fill table model.
	 *
	 * @param[in] dbSet Database container.
	 */
	void fillContactsFromMessageDb(const MessageDbSet &dbSet);

	Ui::DlgContacts *m_ui; /*!< UI generated from UI file. */

	SortFilterProxyModel m_contactListProxyModel; /*!<
	                                               * Used for message
	                                               * sorting and filtering.
	                                               */
	BoxContactsModel m_contactTableModel; /*!< Model of found data boxes. */

	QStringList *const m_dbIdList; /*!< List of box identifiers to append to. */
};
