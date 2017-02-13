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


#ifndef _DLG_CONTACTS_H_
#define _DLG_CONTACTS_H_


#include <QDialog>
#include <QTableWidget>

#include "src/common.h"
#include "src/io/message_db_set.h"
#include "ui_dlg_contacts.h"


class DlgContacts : public QDialog, public Ui::Contacts
{
	Q_OBJECT

public:
	DlgContacts(const MessageDbSet &dbSet, const QString &dbId,
	    QStringList &dbIdList, QWidget *parent = 0);

private slots:

	void filterContact(const QString &text);
	void clearContactText(void);
	void fillContactsFromMessageDb(void);
	void enableOkButton(void);
	void addSelectedDbIDs(void);
	void setFirtsColumnActive(void);
	void contactItemDoubleClicked(const QModelIndex &index);

private:

	const MessageDbSet &m_dbSet;
	const QString m_dbId;
	QStringList &m_dbIdList;
};


#endif /* _DLG_CONTACTS_H_ */
