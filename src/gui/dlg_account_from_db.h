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
#include <QString>

#include "src/models/accounts_model.h"

namespace Ui {
	class DlgCreateAccountFromDb;
}

/*!
 * @brief Provides interface for account creation from database.
 */
class DlgCreateAccountFromDb : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgCreateAccountFromDb(QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	virtual
	~DlgCreateAccountFromDb(void);

	/*!
	 * @brief Creates an accounts from database files and adds them into
	 *     account model.
	 *
	 * @param[in,out] accountModel Account model to add data into.
	 * @param[in,out] lastImportDir Import location directory.
	 * @param[in]     parent Parent widget.
	 * @return List of user names of newly created accounts.
	 */
	static
	QStringList createAccount(AccountModel &accountModel,
	    QString &lastImportDir, QWidget *parent = Q_NULLPTR);

private:
	/*!
	 * @brief Action chosen by the user.
	 */
	enum Action {
		ACT_NOTHING, /*!< Nothing to be performed. */
		ACT_FROM_DIRECTORY, /*!< Read directory content. */
		ACT_FROM_FILES /*!< Read selected files. */
	};

	/*!
	 * @brief Select source to import database from.
	 *
	 * @param[in] parent Parent widget.
	 * @return Chosen action.
	 */
	static
	enum Action chooseAction(QWidget *parent = Q_NULLPTR);

	Ui::DlgCreateAccountFromDb *m_ui; /*!< UI generated from UI file. */
};
