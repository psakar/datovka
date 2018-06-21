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

#include "src/io/message_db_set.h"
#include "src/io/tag_db.h"

namespace Ui {
	class DlgCorrespondenceOverview;
}

/*!
 * @brief Correspondence overview dialogue.
 */
class DlgCorrespondenceOverview : public QDialog {
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

private:
	/*!
	 * @brief Constructor.
	 */
	DlgCorrespondenceOverview(const MessageDbSet &dbSet,
	    const QString &dbId, const QString &userName, TagDb &tagDb,
	    QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgCorrespondenceOverview(void);

	/*!
	 * @brief Calls the dialogue and preforms export action.
	 */
	static
	void exportData(const MessageDbSet &dbSet, const QString &dbId,
	    const QString &userName, TagDb &tagDb, QString &exportCorrespondDir,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Activates/deactivates dialogue parts depending on overview
	 *     type selection.
	 *
	 * @param[in] text Displayed text.
	 */
	void reftectOverviewTypeChange(const QString &text);

	/*!
	 * @brief Check message type selection and enable/disable acceptance
	 *     button.
	 */
	void checkMsgTypeSelection(void);

	/*!
	 * @brief Obtains lists of messages according to calendar selection.
	 */
	void reftectCalendarChange(void);

private:
	/*!
	 * @brief Enables/disables the OK button according to widget content.
	 */
	void updateOkButtonActivity(void);

	/*!
	 * @brief Updates exported message list according to date selection.
	 *
	 * @param[in] fromDate Start date.
	 * @param[in] toData Stop date.
	 */
	void updateExportedMsgList(const QDate &fromDate, const QDate &toDate);

	/*!
	 * @brief Construct a CSV message entry.
	 *
	 * @param[in] mId Message identifier structure.
	 * @return String containing CSV message entry, empty string on error.
	 */
	QString msgCsvEntry(const MessageDb::MsgId &mId) const;

	/*!
	 * @brief Construct a HTML message entry.
	 *
	 * @param[in] userName User login identifying the account.
	 * @param[in] mId Message identifier structure.
	 * @return String containing HTML message entry, empty string on error.
	 */
	QString msgHtmlEntry(const QString &userName,
	    const MessageDb::MsgId &mId) const;

	/*!
	 * @brief Export message overview to CSV file.
	 *
	 * @param[in] fileName Name of the saved file.
	 * @return False on failure.
	 */
	bool writeCsvOverview(const QString &fileName) const;

	/*!
	 * @brief Export message overview to HTML file.
	 *
	 * @param[in] userName User login identifying the account.
	 * @param[in] fileName Name of the saved file.
	 * @return False on failure.
	 */
	bool writeHtmlOverview(const QString &userName,
	    const QString &fileName) const;

	/*!
	 * @brief Export correspondence overview file.
	 *
	 * @param[in] userName User login identifying the account.
	 * @param[in] dir Suggested directory where to store the file.
	 * @param[out] summary String to append summary to.
	 * @return Non-empty path where the file has been stored on success,
	 *     empty path on failure.
	 */
	QString exportOverview(const QString &userName, const QString &dir,
	    QString &summary);

	/*!
	 * @brief Export all data that have been selected in the dialogue.
	 *
	 * @param[in]     userName User login identifying the account.
	 * @param[in,out] exportCorrespondDir Location where to store the data.
	 */
	void exportChosenData(const QString &userName,
	    QString &exportCorrespondDir);

	Ui::DlgCorrespondenceOverview *m_ui; /*!< UI generated from UI file. */

	const MessageDbSet &m_messDbSet; /*!< Database set to be accessed. */
	const QString &m_dbId; /*!< Account database identifier. */
	TagDb &m_tagDb; /*!< Tag database. */
	ExportedMessageList m_exportedMsgs; /*!< List of exported messages. */
};
