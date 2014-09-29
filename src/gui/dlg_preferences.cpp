

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
	this->auto_download_whole_messages->setChecked(globPref.auto_download_whole_messages);
	this->download_on_background->setChecked(globPref.download_on_background);
	this->timerSpinBox->setValue(globPref.timer_value);
	this->send_stats_with_version_checks->setChecked(globPref.send_stats_with_version_checks);
	this->check_new_versions->setChecked(globPref.check_new_versions);
	this->store_messages_on_disk->setChecked(globPref.store_messages_on_disk);
	this->store_additional_data_on_disk->setChecked(globPref.store_additional_data_on_disk);
	this->check_crl->setChecked(globPref.check_crl);
	this->language->setCurrentIndex(getLangugeIndex(globPref.language));
	this->send_stats_with_version_checks->setEnabled(this->check_new_versions->isChecked());
	this->timerLabelPre->setEnabled(this->download_on_background->isChecked());
	this->timerLabelPost->setEnabled(this->download_on_background->isChecked());
	this->timerSpinBox->setEnabled(this->download_on_background->isChecked());

	connect(this->check_new_versions, SIGNAL(stateChanged(int)),
	    this, SLOT(setActiveCheckBox(int)));
	connect(this->download_on_background, SIGNAL(stateChanged(int)),
	    this, SLOT(setActiveTimerSetup(int)));

	connect(this->prefButtonBox, SIGNAL(accepted()), this, SLOT(saveChanges(void)));

	if (globPref.after_start_select == 1) {
		this->after_start_select_1->setChecked(true);
		this->after_start_select_2->setChecked(false);
		this->after_start_select_3->setChecked(false);
	} else if (globPref.after_start_select == 2) {
		this->after_start_select_1->setChecked(false);
		this->after_start_select_2->setChecked(true);
		this->after_start_select_3->setChecked(false);
	} else {
		this->after_start_select_1->setChecked(false);
		this->after_start_select_2->setChecked(false);
		this->after_start_select_3->setChecked(true);
	}

	if (globPref.certificate_validation_date == 1) {
		this->certificate_validation_date_1->setChecked(true);
		this->certificate_validation_date_2->setChecked(false);
	} else {
		this->certificate_validation_date_1->setChecked(false);
		this->certificate_validation_date_2->setChecked(true);
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
	this->send_stats_with_version_checks->setEnabled(Qt::Checked == state);
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
}
