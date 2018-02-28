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

#include <QFileDialog>
#include <QMessageBox>

#include "src/common.h"
#include "src/global.h"
#include "src/gui/dlg_account_from_db.h"
#include "src/log/log.h"
#include "src/settings/account.h"
#include "src/settings/preferences.h"
#include "ui_dlg_account_from_db.h"

DlgCreateAccountFromDb::DlgCreateAccountFromDb(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgCreateAccountFromDb)
{
	m_ui->setupUi(this);
	m_ui->info->setText(tr(
	    "A new account will be created according to the name and the content of the database file. "
	    "This account will operate over the selected database. "
	    "Should such an account or database file already exist in Datovka then the association will fail. "
	    "During the association no database file copy is created nor is the content of the database file modified. "
	    "Nevertheless, we strongly advice you to back-up all important files before associating a database file. "
	    "In order for the association to succeed you will need an active connection to the ISDS server."));
}

DlgCreateAccountFromDb::~DlgCreateAccountFromDb(void)
{
	delete m_ui;
}

/*!
 * @brief Obtain full path to directory where a file lies in.
 *
 * @param[in] filePath File path.
 * @return Absolute directory path.
 */
#define absoluteDirPath(filePath) \
	QFileInfo((filePath)).absoluteDir().absolutePath()

/*!
 * @brief Get all database files in user-selected location.
 *
 * @param[in]     fromDirectory True if whole directories should be scanned,
 *                              false if only selected files should be used.
 * @param[in,out] lastImportDir Import location directory.
 * @param[in]     parent Parent widget.
 * @return List of database files.
 */
static
QStringList getDatabaseFilesFromLocation(bool fromDirectory,
    QString &lastImportDir, QWidget *parent = Q_NULLPTR)
{
	QStringList filePathList;

	if (fromDirectory) {
		QString importDir = QFileDialog::getExistingDirectory(parent,
		    DlgCreateAccountFromDb::tr("Select directory"), lastImportDir,
		    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

		if (importDir.isEmpty()) {
			return QStringList();
		}

		lastImportDir = importDir;
		QStringList fileList(
		    QDir(importDir).entryList(QStringList("*.db")));

		if (fileList.isEmpty()) {
			logWarningNL("%s", "No selected *.db files.");
			QMessageBox::warning(parent,
			    DlgCreateAccountFromDb::tr("No database file found"),
			    DlgCreateAccountFromDb::tr("No database file found in selected directory '%1'.")
			        .arg(importDir),
			    QMessageBox::Ok);
			return QStringList();
		}

		foreach (const QString &fileName, fileList) {
			filePathList.append(importDir + "/" + fileName);
		}
	} else {
		filePathList = QFileDialog::getOpenFileNames(parent,
		    DlgCreateAccountFromDb::tr("Select database files"), lastImportDir,
		    DlgCreateAccountFromDb::tr("Database file (*.db)"));

		if (filePathList.isEmpty()) {
			logWarningNL("%s", "No selected *.db files.");
			return QStringList();
		}

		lastImportDir = absoluteDirPath(filePathList.at(0));
	}

	return filePathList;
}

/*!
 * @brief Create accounts from supplied database files.
 *
 * @param[in,out] accountModel Account model to add account data into.
 * @param[in]     filePathList List of database file paths.
 * @param[in]     parent Parent widget.
 * @return List of user names of newly created accounts.
 */
static
QStringList createAccountsFromDatabaseFiles(AccountModel &accountModel,
    const QStringList &filePathList, QWidget *parent = Q_NULLPTR)
{
	if (filePathList.isEmpty()) {
		return QStringList();
	}

	QStringList accountUserNames;
	const int accountCount = accountModel.rowCount();
	for (int i = 0; i < accountCount; ++i) {
		const QModelIndex index(accountModel.index(i, 0));
		const QString userName(accountModel.userName(index));
		Q_ASSERT(!userName.isEmpty());
		accountUserNames.append(userName);
	}

	QStringList createdAccountUserNames;

	foreach (const QString &filePath, filePathList) {
		const QString dbFileName(QFileInfo(filePath).fileName());
		QString dbUserName;
		QString dbYearFlag;
		bool dbTestingFlag;
		QString errMsg;

		/* Split and check the database file name. */
		if (!isValidDatabaseFileName(dbFileName, dbUserName,
		    dbYearFlag, dbTestingFlag, errMsg)) {
			QMessageBox::warning(parent,
			    DlgCreateAccountFromDb::tr("Create account: %1")
			        .arg(dbUserName),
			    DlgCreateAccountFromDb::tr("File") + ": " +
			        filePath + "\n\n" + errMsg,
			    QMessageBox::Ok);
			continue;
		}

		/* Check whether account already exists. */
		bool exists = false;
		for (int j = 0; j < accountCount; ++j) {
			if (accountUserNames.at(j) == dbUserName) {
				exists = true;
				break;
			}
		}
		if (exists) {
			errMsg = DlgCreateAccountFromDb::tr(
			    "Account with user name '%1' and its message database already exist. "
			    "New account was not created and selected database file was not associated with this account.")
			        .arg(dbUserName);
			QMessageBox::warning(parent,
			    DlgCreateAccountFromDb::tr("Create account: %1")
			        .arg(dbUserName),
			    DlgCreateAccountFromDb::tr("File") + ": " +
			        filePath + "\n\n" + errMsg,
			    QMessageBox::Ok);
			continue;
		}

		AcntSettings itemSettings;
		itemSettings.setTestAccount(dbTestingFlag);
		itemSettings.setAccountName(dbUserName);
		itemSettings.setUserName(dbUserName);
		itemSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD);
		itemSettings.setPassword("");
		itemSettings.setRememberPwd(false);
		itemSettings.setSyncWithAll(false);
		itemSettings.setDbDir(absoluteDirPath(filePath),
		    GlobInstcs::prefsPtr->confDir());
		accountModel.addAccount(itemSettings);
		errMsg = DlgCreateAccountFromDb::tr(
		        "Account with name '%1' has been created (user name '%1').")
		        .arg(dbUserName) + " " +
		    DlgCreateAccountFromDb::tr(
		        "This database file has been set as the actual message database for this account. "
		        "You'll probably have to modify the account properties in order to log in to the ISDS server correctly.");

		QMessageBox::information(parent,
		    DlgCreateAccountFromDb::tr("Create account: %1")
		        .arg(dbUserName),
		    DlgCreateAccountFromDb::tr("File") + ": " +
		        filePath + "\n\n" + errMsg,
		    QMessageBox::Ok);

		createdAccountUserNames.append(dbUserName);
	}

	return createdAccountUserNames;
}

QStringList DlgCreateAccountFromDb::createAccount(AccountModel &accountModel,
    QString &lastImportDir, QWidget *parent)
{
	QStringList dbFilePathList;

	switch (chooseAction(parent)) {
	case ACT_FROM_DIRECTORY:
		dbFilePathList = getDatabaseFilesFromLocation(true,
		    lastImportDir, parent);
		break;
	case ACT_FROM_FILES:
		dbFilePathList = getDatabaseFilesFromLocation(false,
		    lastImportDir, parent);
		break;
	default:
		/* Do nothing. */
		return QStringList();
	}

	return createAccountsFromDatabaseFiles(accountModel, dbFilePathList,
	    parent);
}

enum DlgCreateAccountFromDb::Action DlgCreateAccountFromDb::chooseAction(
    QWidget *parent)
{
	DlgCreateAccountFromDb dlg(parent);
	if (QDialog::Accepted == dlg.exec()) {
		return dlg.m_ui->directory->isChecked() ? ACT_FROM_DIRECTORY :
		    ACT_FROM_FILES;
	} else {
		return ACT_NOTHING;
	}
}
