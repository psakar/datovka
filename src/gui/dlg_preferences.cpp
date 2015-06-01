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

#include <QFileDialog>

#include "dlg_preferences.h"
#include "ui_dlg_preferences.h"

DlgPreferences::DlgPreferences(QWidget * parent)
    : QDialog(parent)
{
	setupUi(this);
	initPrefDialog();
}

void DlgPreferences::initPrefDialog(void)
{
	this->download_at_start->setChecked(globPref.download_at_start);
	this->auto_download_whole_messages->
	    setChecked(globPref.auto_download_whole_messages);
	this->download_on_background->
	    setChecked(globPref.download_on_background);
	this->timerSpinBox->setValue(globPref.timer_value);
	this->timestampExpirSpinBox->setValue(
	    globPref.timestamp_expir_before_days);
	this->timeoutMinSpinBox->setValue(
	    globPref.isds_download_timeout_ms / 60000);
	this->labelTimeoutNote->setText(
	    tr("Note: If you have a slow network connection or you cannot "
	    "download complete messages, here you can increase "
	    "the connection timeout. Default value is %1 minutes. "
	    "Use 0 to disable timeout limit (not recommended).").arg(
	        ISDS_DOWNLOAD_TIMEOUT_MS / 60000));

	this->check_new_versions->setChecked(globPref.check_new_versions);
	this->store_messages_on_disk->
	    setChecked(globPref.store_messages_on_disk);
	this->store_additional_data_on_disk->
	    setChecked(globPref.store_additional_data_on_disk);
	this->check_crl->setChecked(globPref.check_crl);
	this->language->setCurrentIndex(getLangugeIndex(globPref.language));
	this->timerLabelPre->
	    setEnabled(this->download_on_background->isChecked());
	this->timerLabelPost->
	    setEnabled(this->download_on_background->isChecked());
	this->timerSpinBox->
	    setEnabled(this->download_on_background->isChecked());
	this->enableGlobalPaths->setChecked(globPref.use_global_paths);
	this->savePath->setText(globPref.save_attachments_path);
	this->addFilePath->setText(globPref.add_file_to_attachments_path);
	this->all_attachments_save_zfo_msg->
	    setChecked(globPref.all_attachments_save_zfo_msg);
	this->all_attachments_save_zfo_delinfo->
	    setChecked(globPref.all_attachments_save_zfo_delinfo);
	this->all_attachments_save_pdf_msgenvel->
	    setChecked(globPref.all_attachments_save_pdf_msgenvel);
	this->all_attachments_save_pdf_delinfo->
	    setChecked(globPref.all_attachments_save_pdf_delinfo);
	this->delivery_info_for_every_file->
	    setChecked(globPref.delivery_info_for_every_file);

	this->message_filename_format->setText(
	    globPref.message_filename_format);
	this->delivery_filename_format->setText(
	    globPref.delivery_filename_format);
	this->attachment_filename_format->setText(
	    globPref.attachment_filename_format);

	this->delivery_filename_format_all_attach->setText(
	    globPref.delivery_filename_format_all_attach);


	/* TODO - this choice must be disabled */
//	this->send_stats_with_version_checks->
//	    setChecked(globPref.send_stats_with_version_checks);
//	this->send_stats_with_version_checks->
//	    setEnabled(this->check_new_versions->isChecked());

	connect(this->check_new_versions, SIGNAL(stateChanged(int)),
	    this, SLOT(setActiveCheckBox(int)));
	connect(this->download_on_background, SIGNAL(stateChanged(int)),
	    this, SLOT(setActiveTimerSetup(int)));
	connect(this->savePathPushButton, SIGNAL(clicked()),
	    this, SLOT(setSavePath()));
	connect(this->addFilePathPushButton, SIGNAL(clicked()),
	    this, SLOT(setAddFilePath()));
	connect(this->prefButtonBox, SIGNAL(accepted()),
	    this, SLOT(saveChanges(void)));

	if (GlobPreferences::SELECT_NEWEST == globPref.after_start_select) {
		this->after_start_select_1->setChecked(true);
		this->after_start_select_2->setChecked(false);
		this->after_start_select_3->setChecked(false);
	} else if (GlobPreferences::SELECT_LAST_VISITED ==
	   globPref.after_start_select) {
		this->after_start_select_1->setChecked(false);
		this->after_start_select_2->setChecked(true);
		this->after_start_select_3->setChecked(false);
	} else if (GlobPreferences::SELECT_NOTHING ==
	    globPref.after_start_select) {
		this->after_start_select_1->setChecked(false);
		this->after_start_select_2->setChecked(false);
		this->after_start_select_3->setChecked(true);
	} else {
		Q_ASSERT(0);
	}

	if (GlobPreferences::DOWNLOAD_DATE ==
	    globPref.certificate_validation_date) {
		this->certificate_validation_date_1->setChecked(true);
		this->certificate_validation_date_2->setChecked(false);
	} else if (GlobPreferences::CURRENT_DATE ==
	    globPref.certificate_validation_date) {
		this->certificate_validation_date_1->setChecked(false);
		this->certificate_validation_date_2->setChecked(true);
	} else {
		Q_ASSERT(0);
	}
}

int DlgPreferences::getLangugeIndex(const QString &language)
{
	if (langCs == language) {
		return 1;
	} else if (langEn == language) {
		return 2;
	} else {
		return 0;
	}
}


const QString & DlgPreferences::getIndexFromLanguge(int index)
{
	switch (index) {
	case 1:
		return langCs;
		break;
	case 2:
		return langEn;
		break;
	default:
		return langSystem;
		break;
	}
}

const QString DlgPreferences::langCs("cs");
const QString DlgPreferences::langEn("en");
const QString DlgPreferences::langSystem("system");


void DlgPreferences::setActiveTimerSetup(int state)
{
	this->timerLabelPre->setEnabled(Qt::Checked == state);
	this->timerLabelPost->setEnabled(Qt::Checked == state);
	this->timerSpinBox->setEnabled(Qt::Checked == state);
}


void DlgPreferences::setActiveCheckBox(int state)
{
	(void) state;
	/* TODO - this choice must be disabled */
//	this->send_stats_with_version_checks->setEnabled(Qt::Checked == state);
}


void DlgPreferences::setSavePath(void)
{
	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Select directory"), this->savePath->text(),
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (!newDir.isEmpty()) {
		this->savePath->setText(newDir);
	}
}


void DlgPreferences::setAddFilePath(void)
{
	QString newDir = QFileDialog::getExistingDirectory(this,
	    tr("Select directory"), this->addFilePath->text(),
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!newDir.isEmpty()) {
		this->addFilePath->setText(newDir);
	}
}


void DlgPreferences::saveChanges(void) const
{
	globPref.auto_download_whole_messages =
	    this->auto_download_whole_messages->isChecked();
	globPref.download_on_background =
	    this->download_on_background->isChecked();
	globPref.download_at_start =
	    this->download_at_start->isChecked();
	globPref.store_messages_on_disk =
	    this->store_messages_on_disk->isChecked();
	globPref.store_additional_data_on_disk =
	    this->store_additional_data_on_disk->isChecked();
	globPref.certificate_validation_date =
	    this->certificate_validation_date_1->isChecked() ?
	        GlobPreferences::DOWNLOAD_DATE :
	        GlobPreferences::CURRENT_DATE;
	globPref.check_crl = this->check_crl->isChecked();
	globPref.check_new_versions = this->check_new_versions->isChecked();
	globPref.send_stats_with_version_checks =
	    this->send_stats_with_version_checks->isChecked();
	globPref.timer_value = this->timerSpinBox->value();
	globPref.isds_download_timeout_ms =
	    this->timeoutMinSpinBox->value() * 60000;
	globPref.timestamp_expir_before_days =
	    this->timestampExpirSpinBox->value();
	globPref.language =
	    getIndexFromLanguge(this->language->currentIndex());
	if (this->after_start_select_1->isChecked()) {
		globPref.after_start_select = GlobPreferences::SELECT_NEWEST;
	} else if (this->after_start_select_2->isChecked()) {
		globPref.after_start_select =
		    GlobPreferences::SELECT_LAST_VISITED;
	} else {
		globPref.after_start_select = GlobPreferences::SELECT_NOTHING;
	}

	globPref.use_global_paths = this->enableGlobalPaths->isChecked();
	globPref.save_attachments_path = this->savePath->text();
	globPref.add_file_to_attachments_path = this->addFilePath->text();
	globPref.all_attachments_save_zfo_msg =
	    this->all_attachments_save_zfo_msg->isChecked();
	globPref.all_attachments_save_zfo_delinfo =
	    this->all_attachments_save_zfo_delinfo->isChecked();
	globPref.all_attachments_save_pdf_msgenvel =
	    this->all_attachments_save_pdf_msgenvel->isChecked();
	globPref.all_attachments_save_pdf_delinfo =
	    this->all_attachments_save_pdf_delinfo->isChecked();
	globPref.delivery_info_for_every_file =
	    this->delivery_info_for_every_file->isChecked();

	globPref.message_filename_format =
	    this->message_filename_format->text();
	globPref.delivery_filename_format =
	    this->delivery_filename_format->text();
	globPref.attachment_filename_format =
	    this->attachment_filename_format->text();
	globPref.delivery_filename_format_all_attach =
	    this->delivery_filename_format_all_attach->text();
}
