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


#ifndef _DLG_DS_SEARCH_H_
#define _DLG_DS_SEARCH_H_


#include <QDialog>
#include <QTimer>

#include "src/common.h"
#include "src/models/accounts_model.h"
#include "ui_dlg_ds_search.h"


class DlgDsSearch : public QDialog, public Ui::DsSearch {
	Q_OBJECT

public:
	enum Action {
		ACT_BLANK,
		ACT_ADDNEW
	};

	DlgDsSearch(Action action, QTableWidget *recipientTableWidget,
	    QString dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
	    QWidget *parent = 0, QString useName = "");

private slots:
	void checkInputFields(void);
	void insertDsItems(void);
	void enableOkButton(void);
	void searchDataBox(void);
	void pingIsdsServer(void);

private:
	QTimer *pingTimer;
	bool isInRecipientTable(const QString &idDs) const;
	void initSearchWindow(void);
	void addContactsToTable(const QList< QVector<QString> > &contactList);

	Action m_action;
	QTableWidget *m_recipientTableWidget;
	QString m_dbType;
	bool m_dbEffectiveOVM;
	bool m_dbOpenAddressing;
	const QString m_userName;
	bool m_showInfoLabel;
};


#endif /* DLG_DS_SEARCH_H */
