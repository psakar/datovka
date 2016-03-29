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


#ifndef _DLG_MSG_SEARCH_H_
#define _DLG_MSG_SEARCH_H_


#include <QDialog>

#include "src/io/message_db_set.h"
#include "ui_dlg_msg_search.h"


/* tooltip is generated for every item in the search result table */
#define ENABLE_TOOLTIP 1

class DlgMsgSearch : public QDialog, public Ui::msgSearchDialog {
	Q_OBJECT

public:
	DlgMsgSearch(const QList< QPair<QString, MessageDbSet *> > messageDbSetList,
	    const QString &userName, QWidget *parent = 0,
	    Qt::WindowFlags f = 0);

private slots:
	void checkInputFields(void);
	void searchMessages(void);
	void setFirtsColumnActive(void);
	void getSelectedMsg(int row, int column);

signals:
	void focusSelectedMsg(QString, qint64, QString, int);

private:
	const QList< QPair<QString, MessageDbSet *> > m_messageDbSetList;
	const QString m_userName;
	int itemFillCnt;

	void initSearchWindow(void);
	int howManyFieldsAreFill(void);
	void appendMsgsToTable(
	    const QPair<QString, MessageDbSet *> &usrNmAndMsgDbSet,
	    const QList<MessageDb::SoughtMsg> &msgDataList);
};

#endif /* _DLG_MSG_SEARCH_H_ */
