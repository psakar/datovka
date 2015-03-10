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


#ifndef _DLG_CORRESPONDENCE_OVERVIEW_H_
#define _DLG_CORRESPONDENCE_OVERVIEW_H_

#include <QDialog>
#include <QFileDialog>
#include <QTreeView>
#include <QTableView>

#include "src/common.h"
#include "ui_dlg_correspondence_overview.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"


class DlgCorrespondenceOverview : public QDialog,
    public Ui::CorrespondenceOverview {
    Q_OBJECT

public:
	class ExportedMessageList {
	public:
		QList<qint64> sentdmIDs;
		QList<qint64> receivedmIDs;
	};

	DlgCorrespondenceOverview(const MessageDb &db, const QString &dbId,
	    const AccountModel::SettingsMap &accountInfo,
	    QString &exportCorrespondDir, QWidget *parent = 0);

private slots:
	void dateCalendarsChange(const QDate &date);
	void msgStateChanged(int state);
	void exportData(void);

private:
	const MessageDb &m_messDb;
	const QString m_dbId;
	const AccountModel::SettingsMap m_accountInfo;
	ExportedMessageList m_messages;
	QString &m_exportCorrespondDir;

	void getMsgListFromDates(const QDate &fromDate, const QDate &toDate);
	QString msgInCsv(qint64 dmId) const;
	QString msgInHtml(qint64 dmId) const;
	bool exportMessageAsZFO(qint64 dmId, const QString &fileName,
	    bool deliveryInfo) const;
	bool exportMessageAsPDF(qint64 dmId, const QString &fileName,
	    bool deliveryInfo) const;
	bool exportMessagesToCsv(const QString &fileName) const;
	bool exportMessagesToHtml(const QString &fileName) const;

};

#endif // _DLG_CORRESPONDENCE_OVERVIEW_H_
