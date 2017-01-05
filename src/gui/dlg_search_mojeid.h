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


#ifndef _DLG_SEARCH_MOJEID_H_
#define _DLG_SEARCH_MOJEID_H_


#include <QDialog>
#include <QTimer>

#include "src/common.h"
#include "ui_dlg_search_mojeid.h"


class DlgDsSearchMojeId : public QDialog, public Ui::DsSearchMojeId {
	Q_OBJECT

public:
	enum Action {
		ACT_BLANK,
		ACT_ADDNEW
	};

	DlgDsSearchMojeId(Action action, QTableWidget *recipientTableWidget,
	    const QString &dbType, bool dbEffectiveOVM,
	    QWidget *parent = 0, const QString &userName = QString());

private slots:
	void checkInputFields(void);
	void insertDsItems(void);
	void enableOkButton(void);
	void searchDataBox(void);
	void setFirtsColumnActive(void);
	void contactItemDoubleClicked(const QModelIndex &index);

private:

	bool isInRecipientTable(const QString &idDs) const;
	void initSearchWindow(void);
	void addContactsToTable(const QList< QVector<QString> > &contactList);
	void insertContactToRecipentTable(int selRow);

	Action m_action;
	QTableWidget *m_recipientTableWidget;
	QString m_dbType;
	bool m_dbEffectiveOVM;
	const QString m_userName;
	int m_limit;
};


#endif /* _DLG_SEARCH_MOJEID_H_ */
