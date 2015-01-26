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
#include "src/io/message_db.h"
#include "ui_dlg_contacts.h"


class DlgContacts : public QDialog, public Ui::Contacts
{
	Q_OBJECT

public:
	DlgContacts(const MessageDb &db, const QString &dbId,
	    QTableWidget &recipientTableWidget,
	    QString dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
	    QWidget *parent = 0, QString useName = "");

private slots:
	void filterContact(const QString &text);
	void clearContactText(void);
	void fillContactsFromMessageDb(void);
	void enableOkButton(void);
	void insertDsItems(void);
	void setFirtsColumnActive(void);

private:
	bool isInRecipientTable(const QString &idDs) const;
	QString getUserInfoFormIsds(QString idDbox);

	QTableWidget &m_recipientTableWidget;
	const MessageDb &m_messDb;
	const QString m_dbId;
	QString m_dbType;
	bool m_dbEffectiveOVM;
	bool m_dbOpenAddressing;
	const QString m_userName;
};


#endif /* _DLG_CONTACTS_H_ */
