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

#ifndef _DLG_MSG_SEARCH_H_
#define _DLG_MSG_SEARCH_H_

#include <QDialog>

#include "src/io/message_db_set.h"

namespace Ui {
	class DlgMsgSearch;
}

/*!
 * @brief Message search dialogue.
 */
class DlgMsgSearch : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] msgSetEntryList List of username and database set pairs.
	 * @param[in] userName Username.
	 * @param[in] parent Parent widget.
	 * @param[in] flags Window flags.
	 */
	DlgMsgSearch(
	    const QList< QPair<QString, MessageDbSet *> > msgSetEntryList,
	    const QString &userName, QWidget *parent = Q_NULLPTR,
	    Qt::WindowFlags flags = 0);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~DlgMsgSearch(void);

private slots:
	/*!
	 * @brief Check dialogue elements and enable/disable the search button.
	 */
	void checkInputFields(void);

	/*!
	 * @brief Set first column with checkbox active if item was changed.
	 */
	void setFirtsColumnActive(void);

	/*!
	 * @brief Emit ID of message entry.
	 *
	 * @param[in] row Message entry row.
	 * @param[in] col Message entry column.
	 */
	void getSelectedMsg(int row, int col);

	/*!
	 * @brief Collects data from dialogue and searches for messages
	 */
	void searchMessages(void);

signals:
	/*!
	 * @brief Signals that a message was selected should be focused
	 *     elsewhere.
	 *
	 * @param[in] username Username identifying an account.
	 * @param[in] dmId Message identifier.
	 * @param[in] year Message delivery year.
	 * @param[in] msgType Message type.
	 */
	void focusSelectedMsg(QString username, qint64 dmId, QString year,
	    int msgType);

private:
	/*!
	 * @brief Initialise message search dialogue.
	 *
	 * @param[in] username Account username.
	 */
	void initSearchWindow(const QString &username);

	/*!
	 * @brief Computes filled-in fields except the tag field.
	 *
	 * @return Number of filled-in fields except the tag field.
	 */
	int filledInExceptTags(void) const;

	/*!
	 * @brief Append message list to result table.
	 *
	 * @param[in] msgSetEntry Pair of username and related database set.
	 * @param[in] msgDataList Message data list.
	 */
	void appendMsgsToTable(
	    const QPair<QString, MessageDbSet *> &msgSetEntry,
	    const QList<MessageDb::SoughtMsg> &msgDataList);

	Ui::DlgMsgSearch *m_ui; /*!< UI generated from UI file. */

	const QList< QPair<QString, MessageDbSet *> > m_msgSetEntryList; /*!< Usernames and database sets. */
};

#endif /* _DLG_MSG_SEARCH_H_ */
