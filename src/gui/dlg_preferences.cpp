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
#include <QString>

#include "src/common.h"
#include "src/datovka_shared/localisation/localisation.h"
#include "src/dimensions/dimensions.h"
#include "src/gui/dlg_pin_setup.h"
#include "src/gui/dlg_preferences.h"
#include "ui_dlg_preferences.h"

#define MSEC_IN_SEC 1000 /* One second in milliseconds. */
#define MSEC_IN_MIN 60000 /* One minute in milliseconds. */

/*!
 * @brief Language order as they are listed in the combo box.
 */
enum LangIndex {
	LANG_SYSTEM = 0,
	LANG_CZECH = 1,
	LANG_ENGLISH = 2
};

DlgPreferences::DlgPreferences(const Preferences &prefs,
    const PinSettings &pinSett, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgPreferences),
    m_pinSett(pinSett)
{
	m_ui->setupUi(this);

	{
		/* Adjust window size according to font size. */
		QSize newSize = Dimensions::windowSize(this, 46.0, 44.0);
		if (newSize.isValid()) {
			resize(newSize);
		}
	}
	initDialogue(prefs);
	activatePinButtons(m_pinSett);
}

DlgPreferences::~DlgPreferences(void)
{
	delete m_ui;
}

bool DlgPreferences::modify(Preferences &prefs, PinSettings &pinSett,
    QWidget *parent)
{
	DlgPreferences dlg(prefs, pinSett, parent);
	if (QDialog::Accepted == dlg.exec()) {
		dlg.saveSettings(prefs, pinSett);
		return true;
	} else {
		return false;
	}
}

void DlgPreferences::activateBackgroundTimer(int checkState)
{
	const bool enable = Qt::Checked == checkState;
	m_ui->timerPreLabel->setEnabled(enable);
	m_ui->timerSpinBox->setEnabled(enable);
	m_ui->timerPostLabel->setEnabled(enable);
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

void DlgPreferences::setPin(void)
{
	if (Q_UNLIKELY(m_pinSett.pinConfigured())) {
		Q_ASSERT(0);
		return;
	}

	DlgPinSetup::change(m_pinSett, this);
	activatePinButtons(m_pinSett);
}

void DlgPreferences::changePin(void)
{
	if (Q_UNLIKELY(!m_pinSett.pinConfigured())) {
		Q_ASSERT(0);
		return;
	}

	DlgPinSetup::change(m_pinSett, this);
	activatePinButtons(m_pinSett);
}

void DlgPreferences::clearPin(void)
{
	if (Q_UNLIKELY(!m_pinSett.pinConfigured())) {
		Q_ASSERT(0);
		return;
	}

	DlgPinSetup::erase(m_pinSett, this);
	activatePinButtons(m_pinSett);
}

/*!
 * @brief Converts selected language index to language name.
 *
 * @param[in] index Language entry index.
 * @return Language name as used in settings.
 */
static
const char *indexToSettingsLang(int index)
{
	switch (index) {
	case LANG_CZECH:
		return Localisation::langCs;
		break;
	case LANG_ENGLISH:
		return Localisation::langEn;
		break;
	default:
		return Localisation::langSystem;
		break;
	}
}

/*!
 * @brief Converts language name to index.
 *
 * @param[in] lang Language name as used in settings.
 * @return Corresponding index.
 */
static
int settingsLangtoIndex(const QString &lang)
{
	if (Localisation::langCs == lang) {
		return LANG_CZECH;
	} else if (Localisation::langEn == lang) {
		return LANG_ENGLISH;
	} else {
		return LANG_SYSTEM;
	}
}

void DlgPreferences::saveSettings(Preferences &prefs,
    PinSettings &pinSett) const
{
	/* downloading */
	prefs.downloadOnBackground = m_ui->downloadOnBackground->isChecked();
	prefs.timerValue = m_ui->timerSpinBox->value();
	prefs.autoDownloadWholeMessages =
	    m_ui->autoDownloadWholeMessages->isChecked();
	prefs.downloadAtStart = m_ui->downloadAtStart->isChecked();
	prefs.isdsDownloadTimeoutMs =
	    m_ui->timeoutMinSpinBox->value() * MSEC_IN_MIN;
	prefs.messageMarkAsReadTimeout =
	    m_ui->timeoutMarkMsgSpinBox->value() * MSEC_IN_SEC;
	prefs.setCheckNewVersions(m_ui->checkNewVersions->isChecked());
	prefs.setSendStatsWithVersionChecks(
	    m_ui->sendStatsWithVersionChecks->isChecked());

	/* security */
	prefs.storeMessagesOnDisk = m_ui->storeMessagesOnDisk->isChecked();
	prefs.storeAdditionalDataOnDisk =
	    m_ui->storeAdditionalDataOnDisk->isChecked();
	prefs.certificateValidationDate =
	    m_ui->certValidationDateNow->isChecked() ?
	        Preferences::CURRENT_DATE : Preferences::DOWNLOAD_DATE;
	prefs.checkCrl = m_ui->checkCrl->isChecked();
	prefs.timestampExpirBeforeDays =
	    m_ui->timestampExpirSpinBox->value();
	pinSett = m_pinSett;

	/* navigation */
	if (m_ui->afterStartSelectNewest->isChecked()) {
		prefs.afterStartSelect = Preferences::SELECT_NEWEST;
	} else if (m_ui->afterStartSelectLast->isChecked()) {
		prefs.afterStartSelect = Preferences::SELECT_LAST_VISITED;
	} else {
		prefs.afterStartSelect = Preferences::SELECT_NOTHING;
	}

	/* interface */
	if (m_ui->toolButtonIconOnly->isChecked()) {
		prefs.toolbarButtonStyle = Preferences::ICON_ONLY;
	} else if (m_ui->toolButtonTextBesideIcon->isChecked()) {
		prefs.toolbarButtonStyle = Preferences::TEXT_BESIDE_ICON;
	} else {
		prefs.toolbarButtonStyle = Preferences::TEXT_UNDER_ICON;
	}

	/* directories */
	prefs.useGlobalPaths = m_ui->enableGlobalPaths->isChecked();
	prefs.saveAttachmentsPath = m_ui->savePath->text();
	prefs.addFileToAttachmentsPath = m_ui->addFilePath->text();

	/* saving */
	prefs.allAttachmentsSaveZfoMsg =
	    m_ui->allAttachmentsSaveZfoMsg->isChecked();
	prefs.allAttachmentsSaveZfoDelinfo =
	    m_ui->allAttachmentsSaveZfoDelInfo->isChecked();
	prefs.allAttachmentsSavePdfMsgenvel =
	    m_ui->allAttachmentsSavePdfMsgEnvel->isChecked();
	prefs.allAttachmentsSavePdfDelinfo =
	    m_ui->allAttachmentsSavePdfDelInfo->isChecked();
	prefs.messageFilenameFormat = m_ui->msgFileNameFmt->text();
	prefs.deliveryFilenameFormat = m_ui->delInfoFileNameFmt->text();
	prefs.attachmentFilenameFormat = m_ui->attachFileNameFmt->text();
	prefs.deliveryInfoForEveryFile =
	    m_ui->delInfoForEveryFile->isChecked();
	prefs.deliveryFilenameFormatAllAttach =
	    m_ui->allAttachDelInfoFileNameFmt->text();

	/* language */
	prefs.language =
	    indexToSettingsLang(m_ui->langComboBox->currentIndex());
}

void DlgPreferences::initDialogue(const Preferences &prefs)
{
	/* downloading */
	m_ui->downloadOnBackground->setChecked(prefs.downloadOnBackground);
	m_ui->timerSpinBox->setValue(prefs.timerValue);
	m_ui->autoDownloadWholeMessages->setChecked(
	    prefs.autoDownloadWholeMessages);
	m_ui->downloadAtStart->setChecked(prefs.downloadAtStart);
	m_ui->timeoutMinSpinBox->setValue(
	    prefs.isdsDownloadTimeoutMs / MSEC_IN_MIN);
	m_ui->labelTimeoutNote->setText(tr(
	    "Note: If you have a slow network connection or you cannot download complete messages, here you can increase the connection timeout. "
	    "Default value is %1 minutes. Use 0 to disable timeout limit (not recommended).")
	        .arg(ISDS_DOWNLOAD_TIMEOUT_MS / MSEC_IN_MIN));
	m_ui->timeoutMarkMsgSpinBox->setValue(
	    prefs.messageMarkAsReadTimeout / MSEC_IN_SEC);
	m_ui->noteMarkMsgLabel->setText(tr(
	    "Note: Marked unread message will be marked as read after set interval. "
	    "Default value is %1 seconds. Use -1 disable the function.")
	        .arg(TIMER_MARK_MSG_READ_MS / MSEC_IN_SEC));
	m_ui->checkNewVersions->setChecked(prefs.checkNewVersions());
	m_ui->checkNewVersions->setEnabled(
	    Preferences::canConfigureCheckNewVersions());
	/* TODO - this choice must be disabled */
//	m_ui->sendStatsWithVersionChecks->setChecked(
//	    prefs.sendStatsWithVersionChecks());
//	m_ui->sendStatsWithVersionChecks->setEnabled(
//	    m_ui->checkNewVersions->isChecked());

	connect(m_ui->downloadOnBackground, SIGNAL(stateChanged(int)),
	    this, SLOT(activateBackgroundTimer(int)));

	activateBackgroundTimer(m_ui->downloadOnBackground->checkState());

	/* security */
	m_ui->storeMessagesOnDisk->setChecked(prefs.storeMessagesOnDisk);
	m_ui->storeAdditionalDataOnDisk->setChecked(
	    prefs.storeAdditionalDataOnDisk);
	if (Preferences::CURRENT_DATE == prefs.certificateValidationDate) {
		m_ui->certValidationDateNow->setChecked(true);
		m_ui->certValidationDateDownload->setChecked(false);
	} else if (Preferences::DOWNLOAD_DATE ==
	    prefs.certificateValidationDate) {
		m_ui->certValidationDateNow->setChecked(false);
		m_ui->certValidationDateDownload->setChecked(true);
	} else {
		Q_ASSERT(0);
	}
	m_ui->checkCrl->setChecked(prefs.checkCrl);
	m_ui->timestampExpirSpinBox->setValue(prefs.timestampExpirBeforeDays);

	connect(m_ui->setPinButton, SIGNAL(clicked()), this, SLOT(setPin()));
	connect(m_ui->changePinButton, SIGNAL(clicked()),
	    this, SLOT(changePin()));
	connect(m_ui->clearPinButton, SIGNAL(clicked()),
	    this, SLOT(clearPin()));

	/* navigation */
	if (Preferences::SELECT_NEWEST == prefs.afterStartSelect) {
		m_ui->afterStartSelectNewest->setChecked(true);
		m_ui->afterStartSelectLast->setChecked(false);
		m_ui->afterStartSelectNothing->setChecked(false);
	} else if (Preferences::SELECT_LAST_VISITED ==
	    prefs.afterStartSelect) {
		m_ui->afterStartSelectNewest->setChecked(false);
		m_ui->afterStartSelectLast->setChecked(true);
		m_ui->afterStartSelectNothing->setChecked(false);
	} else if (Preferences::SELECT_NOTHING ==
	    prefs.afterStartSelect) {
		m_ui->afterStartSelectNewest->setChecked(false);
		m_ui->afterStartSelectLast->setChecked(false);
		m_ui->afterStartSelectNothing->setChecked(true);
	} else {
		Q_ASSERT(0);
	}

	/* interface */
	switch (prefs.toolbarButtonStyle) {
	case Preferences::ICON_ONLY:
		m_ui->toolButtonIconOnly->setChecked(true);
		m_ui->toolButtonTextBesideIcon->setChecked(false);
		m_ui->toolButtonTextUnderIcon->setChecked(false);
		break;
	case Preferences::TEXT_BESIDE_ICON:
		m_ui->toolButtonIconOnly->setChecked(false);
		m_ui->toolButtonTextBesideIcon->setChecked(true);
		m_ui->toolButtonTextUnderIcon->setChecked(false);
		break;
	case Preferences::TEXT_UNDER_ICON:
		m_ui->toolButtonIconOnly->setChecked(false);
		m_ui->toolButtonTextBesideIcon->setChecked(false);
		m_ui->toolButtonTextUnderIcon->setChecked(true);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	/* directories */
	m_ui->enableGlobalPaths->setChecked(prefs.useGlobalPaths);
	m_ui->savePath->setText(prefs.saveAttachmentsPath);
	m_ui->addFilePath->setText(prefs.addFileToAttachmentsPath);

	connect(m_ui->savePathPushButton, SIGNAL(clicked()),
	    this, SLOT(setSavePath()));
	connect(m_ui->addFilePathPushButton, SIGNAL(clicked()),
	    this, SLOT(setAddFilePath()));

	/* saving */
	m_ui->allAttachmentsSaveZfoMsg->setChecked(
	    prefs.allAttachmentsSaveZfoMsg);
	m_ui->allAttachmentsSaveZfoDelInfo->setChecked(
	    prefs.allAttachmentsSaveZfoDelinfo);
	m_ui->allAttachmentsSavePdfMsgEnvel->setChecked(
	    prefs.allAttachmentsSavePdfMsgenvel);
	m_ui->allAttachmentsSavePdfDelInfo->setChecked(
	    prefs.allAttachmentsSavePdfDelinfo);
	m_ui->msgFileNameFmt->setText(prefs.messageFilenameFormat);
	m_ui->delInfoFileNameFmt->setText(prefs.deliveryFilenameFormat);
	m_ui->attachFileNameFmt->setText(prefs.attachmentFilenameFormat);
	m_ui->delInfoForEveryFile->setChecked(
	    prefs.deliveryInfoForEveryFile);
	m_ui->allAttachDelInfoFileNameFmt->setText(
	    prefs.deliveryFilenameFormatAllAttach);

	/* language */
	m_ui->langComboBox->addItem(tr("Use system language"));
	m_ui->langComboBox->addItem(tr("Czech"));
	m_ui->langComboBox->addItem(tr("English"));
	m_ui->langComboBox->setCurrentIndex(
	    settingsLangtoIndex(prefs.language));
}

void DlgPreferences::activatePinButtons(const PinSettings &pinSett)
{
	m_ui->setPinButton->setEnabled(!pinSett.pinConfigured());
	m_ui->setPinButton->setVisible(!pinSett.pinConfigured());
	m_ui->changePinButton->setEnabled(pinSett.pinConfigured());
	m_ui->changePinButton->setVisible(pinSett.pinConfigured());
	m_ui->clearPinButton->setEnabled(pinSett.pinConfigured());
	m_ui->clearPinButton->setVisible(pinSett.pinConfigured());
}
