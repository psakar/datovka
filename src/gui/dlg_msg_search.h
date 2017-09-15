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
	DlgMsgSearch(
	    const QList< QPair<QString, MessageDbSet *> > messageDbSetList,
	    const QString &userName, QWidget *parent = Q_NULLPTR,
	    Qt::WindowFlags f = 0);

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

	void searchMessages(void);
	void setFirtsColumnActive(void);
	void getSelectedMsg(int row, int column);

signals:
	void focusSelectedMsg(QString, qint64, QString, int);

private:
	/*!
	 * @brief Initialise message search dialogue.
	 *
	 * @param[in] username Account username.
	 */
	void initSearchWindow(const QString &username);

	int howManyFieldsAreFilledWithoutTag(void);

	void appendMsgsToTable(
	    const QPair<QString, MessageDbSet *> &usrNmAndMsgDbSet,
	    const QList<MessageDb::SoughtMsg> &msgDataList);

	Ui::DlgMsgSearch *m_ui; /*!< UI generated from UI file. */

	const QList< QPair<QString, MessageDbSet *> > m_messageDbSetList;
};

#endif /* _DLG_MSG_SEARCH_H_ */
