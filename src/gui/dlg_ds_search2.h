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

#ifndef _DLG_DS_SEARCH2_H_
#define _DLG_DS_SEARCH2_H_

#include <QDialog>

#include "src/common.h"
#include "src/io/isds_sessions.h"
#include "ui_dlg_ds_search2.h"

class DlgSearch2 : public QDialog, public Ui::Search2 {
	Q_OBJECT
public:
	enum Action {
		ACT_BLANK,
		ACT_ADDNEW
	};

	DlgSearch2(Action action, QStringList &dbIdList, QWidget *parent = 0,
	    const QString &userName = QString());

private slots:
	void enableOkButton(void);
	void enableSearchButton(const QString &text);
	void searchNewDataboxes(void);
	void showNextDataboxes(void);
	void addSelectedDbIDs(void);
	void setFirtsColumnActive(void);
	void contactItemDoubleClicked(const QModelIndex &index);

private:
	void findDataboxes(quint64 pageNumber, isds_fulltext_target target,
	    isds_DbType box_type, const QString &phrase);

	Action m_action;
	QStringList &m_dbIdList;
	const QString m_userName;

	// hold some search settings for showing of next results
	quint64 m_currentPage;
	isds_fulltext_target m_target;
	isds_DbType m_box_type;
	QString m_phrase;
};


#endif /* _DLG_DS_SEARCH2_H_ */
