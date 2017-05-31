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

#ifndef _DLG_CORRESPONDENCE_OVERVIEW_H_
#define _DLG_CORRESPONDENCE_OVERVIEW_H_

#include <QDialog>

#include "src/io/message_db_set.h"
#include "ui_dlg_correspondence_overview.h"

class DlgCorrespondenceOverview : public QDialog,
    public Ui::CorrespondenceOverview {
    Q_OBJECT

public:
	/*!
	 * @brief Holds list of exported messages.
	 */
	class ExportedMessageList {
	public:
		QList<MessageDb::MsgId> sentDmIDs; /*!< Sent massage identifiers. */
		QList<MessageDb::MsgId> receivedDmIDs; /*!< Received message identifiers. */
	};

	/*!
	 * @brief Constructor.
	 */
	DlgCorrespondenceOverview(const MessageDbSet &dbSet,
	    const QString &userName, QString &exportCorrespondDir,
	    const QString &dbId, QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Check message type selection and enable/disable acceptance
	 *     button.
	 */
	void checkMsgTypeSelection(void);

	void dateCalendarsChange(const QDate &date);
	void exportData(void);

private:
	const MessageDbSet &m_messDbSet;
	const QString m_userName;
	ExportedMessageList m_messages;
	QString &m_exportCorrespondDir;
	const QString &m_dbId;

	void getMsgListFromDates(const QDate &fromDate, const QDate &toDate);
	QString msgInCsv(const MessageDb::MsgId &mId) const;
	QString msgInHtml(const MessageDb::MsgId &mId) const;
	bool exportMessagesToCsv(const QString &fileName) const;
	bool exportMessagesToHtml(const QString &fileName) const;
};

#endif /* _DLG_CORRESPONDENCE_OVERVIEW_H_ */
