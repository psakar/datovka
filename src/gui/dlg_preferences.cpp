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

#include "src/common.h"
#include "src/dimensions/dimensions.h"
#include "src/gui/dlg_preferences.h"
#include "src/localisation/localisation.h"
#include "src/settings/preferences.h"
#include "ui_dlg_preferences.h"

DlgPreferences::DlgPreferences(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgPreferences)
{
	m_ui->setupUi(this);

	{
		/* Adjust window size according to font size. */
		QSize newSize = Dimensions::windowSize(this, 46.0, 44.0);
		if (newSize.isValid()) {
			resize(newSize);
		}
	}
	initPrefDialog();
}

DlgPreferences::~DlgPreferences(void)
{
	delete m_ui;
}

void DlgPreferences::initPrefDialog(void)
{
	/* downloading */
	m_ui->downloadOnBackground->setChecked(globPref.download_on_background);
	m_ui->timerSpinBox->setValue(globPref.timer_value);
	m_ui->autoDownloadWholeMessages->setChecked(
	    globPref.auto_download_whole_messages);
	m_ui->downloadAtStart->setChecked(globPref.download_at_start);
	m_ui->timeoutMinSpinBox->setValue(
	    globPref.isds_download_timeout_ms / 60000);
	m_ui->labelTimeoutNote->setText(tr(
	    "Note: If you have a slow network connection or you cannot download complete messages, here you can increase the connection timeout. "
	    "Default value is %1 minutes. Use 0 to disable timeout limit (not recommended).")
	        .arg(ISDS_DOWNLOAD_TIMEOUT_MS / 60000));
	m_ui->timeoutMarkMsgSpinBox->setValue(
	    globPref.message_mark_as_read_timeout / 1000);
	m_ui->noteMarkMsgLabel->setText(tr(
	    "Note: Marked unread message will be marked as read after set interval. "
	    "Default value is %1 seconds. Use -1 disable the function.")
	        .arg(TIMER_MARK_MSG_READ_MS / 1000));
	m_ui->checkNewVersions->setChecked(globPref.check_new_versions);
	/* TODO - this choice must be disabled */
//	m_ui->sendStatsWithVersionChecks->setChecked(
//	    globPref.send_stats_with_version_checks);
//	m_ui->sendStatsWithVersionChecks->setEnabled(
//	    m_ui->checkNewVersions->isChecked());

	connect(m_ui->downloadOnBackground, SIGNAL(stateChanged(int)),
	    this, SLOT(setActiveTimerSetup(int)));
	connect(m_ui->checkNewVersions, SIGNAL(stateChanged(int)),
	    this, SLOT(setActiveCheckBox(int)));

	setActiveTimerSetup(m_ui->downloadOnBackground->checkState());

	/* storage */
	m_ui->storeMessagesOnDisk->setChecked(globPref.store_messages_on_disk);
	m_ui->storeAdditionalDataOnDisk->setChecked(
	    globPref.store_additional_data_on_disk);
	if (GlobPreferences::CURRENT_DATE ==
	    globPref.certificate_validation_date) {
		m_ui->certValidationDateNow->setChecked(true);
		m_ui->certValidationDateDownload->setChecked(false);
	} else if (GlobPreferences::DOWNLOAD_DATE ==
	    globPref.certificate_validation_date) {
		m_ui->certValidationDateNow->setChecked(false);
		m_ui->certValidationDateDownload->setChecked(true);
	} else {
		Q_ASSERT(0);
	}
	m_ui->checkCrl->setChecked(globPref.check_crl);
	m_ui->timestampExpirSpinBox->setValue(
	    globPref.timestamp_expir_before_days);

	/* navigation */
	if (GlobPreferences::SELECT_NEWEST == globPref.after_start_select) {
		m_ui->afterStartSelectNewest->setChecked(true);
		m_ui->afterStartSelectLast->setChecked(false);
		m_ui->afterStartSelectNothing->setChecked(false);
	} else if (GlobPreferences::SELECT_LAST_VISITED ==
	   globPref.after_start_select) {
		m_ui->afterStartSelectNewest->setChecked(false);
		m_ui->afterStartSelectLast->setChecked(true);
		m_ui->afterStartSelectNothing->setChecked(false);
	} else if (GlobPreferences::SELECT_NOTHING ==
	    globPref.after_start_select) {
		m_ui->afterStartSelectNewest->setChecked(false);
		m_ui->afterStartSelectLast->setChecked(false);
		m_ui->afterStartSelectNothing->setChecked(true);
	} else {
		Q_ASSERT(0);
	}

	/* interface */
	if (Qt::ToolButtonIconOnly == globPref.toolbar_button_style) {
		m_ui->toolButtonIconOnly->setChecked(true);
		m_ui->toolButtonTextBesideIcon->setChecked(false);
		m_ui->toolButtonTextUnderIcon->setChecked(false);
	} else if (Qt::ToolButtonTextBesideIcon ==
	   globPref.toolbar_button_style) {
		m_ui->toolButtonIconOnly->setChecked(false);
		m_ui->toolButtonTextBesideIcon->setChecked(true);
		m_ui->toolButtonTextUnderIcon->setChecked(false);
	} else if (Qt::ToolButtonTextUnderIcon ==
	    globPref.toolbar_button_style) {
		m_ui->toolButtonIconOnly->setChecked(false);
		m_ui->toolButtonTextBesideIcon->setChecked(false);
		m_ui->toolButtonTextUnderIcon->setChecked(true);
	} else {
		Q_ASSERT(0);
	}

	/* .. */
	m_ui->language->setCurrentIndex(getLangugeIndex(globPref.language));
	m_ui->enableGlobalPaths->setChecked(globPref.use_global_paths);
	m_ui->savePath->setText(globPref.save_attachments_path);
	m_ui->addFilePath->setText(globPref.add_file_to_attachments_path);
	m_ui->all_attachments_save_zfo_msg->setChecked(
	    globPref.all_attachments_save_zfo_msg);
	m_ui->all_attachments_save_zfo_delinfo->setChecked(
	    globPref.all_attachments_save_zfo_delinfo);
	m_ui->all_attachments_save_pdf_msgenvel->setChecked(
	    globPref.all_attachments_save_pdf_msgenvel);
	m_ui->all_attachments_save_pdf_delinfo->setChecked(
	    globPref.all_attachments_save_pdf_delinfo);
	m_ui->delivery_info_for_every_file->setChecked(
	    globPref.delivery_info_for_every_file);

	m_ui->message_filename_format->setText(
	    globPref.message_filename_format);
	m_ui->delivery_filename_format->setText(
	    globPref.delivery_filename_format);
	m_ui->attachment_filename_format->setText(
	    globPref.attachment_filename_format);

	m_ui->delivery_filename_format_all_attach->setText(
	    globPref.delivery_filename_format_all_attach);

	connect(m_ui->savePathPushButton, SIGNAL(clicked()),
	    this, SLOT(setSavePath()));
	connect(m_ui->addFilePathPushButton, SIGNAL(clicked()),
	    this, SLOT(setAddFilePath()));
	connect(m_ui->prefButtonBox, SIGNAL(accepted()),
	    this, SLOT(saveChanges(void)));
}

int DlgPreferences::getLangugeIndex(const QString &language)
{
	if (Localisation::langCs == language) {
		return 1;
	} else if (Localisation::langEn == language) {
		return 2;
	} else {
		return 0;
	}
}


const QString &DlgPreferences::getIndexFromLanguge(int index)
{
	switch (index) {
	case 1:
		return Localisation::langCs;
		break;
	case 2:
		return Localisation::langEn;
		break;
	default:
		return Localisation::langSystem;
		break;
	}
}

void DlgPreferences::setActiveTimerSetup(int state)
{
	m_ui->timerPreLabel->setEnabled(Qt::Checked == state);
	m_ui->timerSpinBox->setEnabled(Qt::Checked == state);
	m_ui->timerPostLabel->setEnabled(Qt::Checked == state);
}


void DlgPreferences::setActiveCheckBox(int state)
{
	Q_UNUSED(state);
	/* TODO - this choice must be disabled */
//	m_ui->sendStatsWithVersionChecks->setEnabled(Qt::Checked == state);
}


void DlgPreferences::setSavePath(void)
{
	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Select directory"), m_ui->savePath->text(),
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (!newDir.isEmpty()) {
		m_ui->savePath->setText(newDir);
	}
}


void DlgPreferences::setAddFilePath(void)
{
	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Select directory"), m_ui->addFilePath->text(),
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!newDir.isEmpty()) {
		m_ui->addFilePath->setText(newDir);
	}
}


void DlgPreferences::saveChanges(void) const
{
	/* downloading */
	globPref.download_on_background =
	    m_ui->downloadOnBackground->isChecked();
	globPref.timer_value = m_ui->timerSpinBox->value();
	globPref.auto_download_whole_messages =
	    m_ui->autoDownloadWholeMessages->isChecked();
	globPref.download_at_start = m_ui->downloadAtStart->isChecked();
	globPref.isds_download_timeout_ms =
	    m_ui->timeoutMinSpinBox->value() * 60000;
	globPref.message_mark_as_read_timeout =
	    m_ui->timeoutMarkMsgSpinBox->value() * 1000;
	globPref.check_new_versions = m_ui->checkNewVersions->isChecked();
	globPref.send_stats_with_version_checks =
	    m_ui->sendStatsWithVersionChecks->isChecked();

	/* storage */
	globPref.store_messages_on_disk =
	    m_ui->storeMessagesOnDisk->isChecked();
	globPref.store_additional_data_on_disk =
	    m_ui->storeAdditionalDataOnDisk->isChecked();
	globPref.certificate_validation_date =
	    m_ui->certValidationDateNow->isChecked() ?
	        GlobPreferences::CURRENT_DATE :
	        GlobPreferences::DOWNLOAD_DATE;
	globPref.check_crl = m_ui->checkCrl->isChecked();
	globPref.timestamp_expir_before_days =
	    m_ui->timestampExpirSpinBox->value();

	/* navigation */
	if (m_ui->afterStartSelectNewest->isChecked()) {
		globPref.after_start_select = GlobPreferences::SELECT_NEWEST;
	} else if (m_ui->afterStartSelectLast->isChecked()) {
		globPref.after_start_select =
		    GlobPreferences::SELECT_LAST_VISITED;
	} else {
		globPref.after_start_select = GlobPreferences::SELECT_NOTHING;
	}

	/* interface */
	if (m_ui->toolButtonIconOnly->isChecked()) {
		globPref.toolbar_button_style = Qt::ToolButtonIconOnly;
	} else if (m_ui->toolButtonTextBesideIcon->isChecked()) {
		globPref.toolbar_button_style =
		    Qt::ToolButtonTextBesideIcon;
	} else {
		globPref.toolbar_button_style = Qt::ToolButtonTextUnderIcon;
	}

	/* ... */
	globPref.language = getIndexFromLanguge(m_ui->language->currentIndex());

	globPref.use_global_paths = m_ui->enableGlobalPaths->isChecked();
	globPref.save_attachments_path = m_ui->savePath->text();
	globPref.add_file_to_attachments_path = m_ui->addFilePath->text();
	globPref.all_attachments_save_zfo_msg =
	    m_ui->all_attachments_save_zfo_msg->isChecked();
	globPref.all_attachments_save_zfo_delinfo =
	    m_ui->all_attachments_save_zfo_delinfo->isChecked();
	globPref.all_attachments_save_pdf_msgenvel =
	    m_ui->all_attachments_save_pdf_msgenvel->isChecked();
	globPref.all_attachments_save_pdf_delinfo =
	    m_ui->all_attachments_save_pdf_delinfo->isChecked();
	globPref.delivery_info_for_every_file =
	    m_ui->delivery_info_for_every_file->isChecked();

	globPref.message_filename_format =
	    m_ui->message_filename_format->text();
	globPref.delivery_filename_format =
	    m_ui->delivery_filename_format->text();
	globPref.attachment_filename_format =
	    m_ui->attachment_filename_format->text();
	globPref.delivery_filename_format_all_attach =
	    m_ui->delivery_filename_format_all_attach->text();
}
