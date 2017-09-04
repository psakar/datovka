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

#include <QFileDialog>
#include <QMessageBox>

#include "src/gui/dlg_change_directory.h"
#include "src/io/message_db_set.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"
#include "ui_dlg_change_directory.h"

namespace Ui {
	class DlgChangeDirectory;
}

DlgChangeDirectory::DlgChangeDirectory(const QString &currentDir,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgChangeDirectory)
{
	m_ui->setupUi(this);

	m_ui->newPath->setText("");
	m_ui->currentPath->setText(currentDir);
	m_ui->labelWarning->setStyleSheet("QLabel { color: red }");

	if (m_ui->newPath->text().isEmpty()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else if (m_ui->currentPath->text() != m_ui->newPath->text()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	} else {
		m_ui->labelWarning->show();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}

	connect(m_ui->chooseButton, SIGNAL(clicked()), this,
	    SLOT(chooseNewDirectory(void)));
}

DlgChangeDirectory::~DlgChangeDirectory(void)
{
	delete m_ui;
}

bool DlgChangeDirectory::changeDataDirectory(const QString &userName,
    MessageDbSet *dbSet, QWidget *parent)
{
	if (Q_UNLIKELY(userName.isEmpty() || (dbSet == Q_NULLPTR))) {
		Q_ASSERT(0);
		return false;
	}

	enum Action action = ACT_MOVE;
	QString newDirPath;

	/* Get current settings. */
	const AcntSettings &itemSettings(globAccounts[userName]);

	QString oldDbDir(itemSettings.dbDir());
	if (oldDbDir.isEmpty()) {
		/* Set default directory name. */
		oldDbDir = globPref.confDir();
	}

	if (!chooseAction(oldDbDir, newDirPath, action, parent)) {
		return false;
	}

	if (Q_UNLIKELY(newDirPath.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}

	return relocateDatabase(userName, dbSet, oldDbDir, newDirPath, action,
	    parent);
}

void DlgChangeDirectory::chooseNewDirectory(void)
{
	QString newDir(QFileDialog::getExistingDirectory(this,
	    tr("Open Directory"), QString(),
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks));
	m_ui->newPath->setText(newDir);

	if (m_ui->newPath->text().isEmpty()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else if (m_ui->currentPath->text() != m_ui->newPath->text()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	} else {
		m_ui->labelWarning->show();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}
}

bool DlgChangeDirectory::chooseAction(const QString &currentDir,
    QString &newDir, enum Action &action, QWidget *parent)
{
	DlgChangeDirectory dlg(currentDir, parent);
	if (QDialog::Accepted == dlg.exec()) {
		newDir = dlg.m_ui->newPath->text();
		if (dlg.m_ui->moveDataRadioButton->isChecked()) {
			action = ACT_MOVE;
		} else if (dlg.m_ui->copyDataRadioButton->isChecked()) {
			action = ACT_COPY;
		} else {
			action = ACT_NEW;
		}
		return true;
	} else {
		return false;
	}
}

bool DlgChangeDirectory::relocateDatabase(const QString &userName,
    MessageDbSet *dbSet, const QString &oldDir, const QString &newDir,
    enum Action action, QWidget *parent)
{
	if (Q_UNLIKELY(userName.isEmpty()) || (dbSet == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}

	/* Get current settings. */
	AcntSettings &itemSettings(globAccounts[userName]);

	switch (action) {
	case ACT_MOVE:
		/* Move account database into new directory. */
		if (dbSet->moveToLocation(newDir)) {
			itemSettings.setDbDir(newDir);

			logInfo("Database files for '%s' have been moved from '%s' to '%s'.\n",
			    userName.toUtf8().constData(),
			    oldDir.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(parent,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' have been successfully moved to\n\n'%2'.")
			        .arg(userName).arg(newDir),
			    QMessageBox::Ok);

			return true;
		} else {
			QMessageBox::critical(parent,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' could not be moved to\n\n'%2'.")
			        .arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}
		break;
	case ACT_COPY:
		/* Copy account database into new directory. */
		if (dbSet->copyToLocation(newDir)) {
			itemSettings.setDbDir(newDir);

			logInfo("Database files for '%s' have been copied from '%s' to '%s'.\n",
			    userName.toUtf8().constData(),
			    oldDir.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(parent,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' have been successfully copied to\n\n'%2'.")
			        .arg(userName).arg(newDir),
			    QMessageBox::Ok);

			return true;
		} else {
			QMessageBox::critical(parent,
			    tr("Change data directory for current account"),
			    tr("Database files for '%1' could not be copied to\n\n'%2'.")
			        .arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}
		break;
	case ACT_NEW:
		/* Create a new account database into new directory. */
		if (dbSet->reopenLocation(newDir, MessageDbSet::DO_YEARLY,
		        MessageDbSet::CM_CREATE_EMPTY_CURRENT)) {
			itemSettings.setDbDir(newDir);

			logInfo("Database files for '%s' have been created in '%s'.\n",
			    userName.toUtf8().constData(),
			    newDir.toUtf8().constData());

			QMessageBox::information(parent,
			    tr("Change data directory for current account"),
			    tr("New database files for '%1' have been successfully created in\n\n'%2'.")
			        .arg(userName).arg(newDir),
			    QMessageBox::Ok);

			return true;
		} else {
			QMessageBox::critical(parent,
			    tr("Change data directory for current account"),
			    tr("New database files for '%1' could not be created in\n\n'%2'.")
			        .arg(userName).arg(newDir),
			    QMessageBox::Ok);
		}
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}
