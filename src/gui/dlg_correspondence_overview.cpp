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


#include <QMessageBox>
#include <QPushButton>
#include <QPrinter>
#include <QTextDocument>

#include "dlg_correspondence_overview.h"
#include "src/io/filesystem.h"
#include "src/models/accounts_model.h"
#include "src/settings/preferences.h"


/* ========================================================================= */
/*
 * Constructor.
 */
DlgCorrespondenceOverview::DlgCorrespondenceOverview(const MessageDbSet &dbSet,
    const QString &userName, QString &exportCorrespondDir, const QString &dbId,
    QWidget *parent) :
    QDialog(parent),
    m_messDbSet(dbSet),
    m_userName(userName),
    m_exportCorrespondDir(exportCorrespondDir),
    m_dbId(dbId)
/* ========================================================================= */
{
	setupUi(this);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	Q_ASSERT(!m_userName.isEmpty());

	QString accountName =
	    AccountModel::globAccounts[userName].accountName() + " (" +
	    m_userName + ")";
	this->accountName->setText(accountName);

	this->toCalendarWidget->setMinimumDate(this->fromCalendarWidget->selectedDate());

	QDate currentDate = QDate().currentDate();
	this->toCalendarWidget->setMaximumDate(currentDate);
	this->fromCalendarWidget->setMaximumDate(currentDate);

	this->outputFormatComboBox->addItem("CSV");
	this->outputFormatComboBox->addItem("HTML");

	connect(this->fromCalendarWidget, SIGNAL(clicked(QDate)), this,
	SLOT(dateCalendarsChange(QDate)));

	connect(this->toCalendarWidget, SIGNAL(clicked(QDate)), this,
	SLOT(dateCalendarsChange(QDate)));

	connect(this->sentCheckBox, SIGNAL(stateChanged(int)), this,
	SLOT(msgStateChanged(int)));

	connect(this->receivedCheckBox, SIGNAL(stateChanged(int)), this,
	SLOT(msgStateChanged(int)));

	getMsgListFromDates(this->fromCalendarWidget->selectedDate(),
	    this->toCalendarWidget->selectedDate());

	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(exportData(void)));
}


/* ========================================================================= */
/*
 * Slot: fires when message type checkboxes change state.
 */
void DlgCorrespondenceOverview::msgStateChanged(int state)
/* ========================================================================= */
{
	Q_UNUSED(state);

	if ((!this->sentCheckBox->isChecked()) &&
	    (!this->receivedCheckBox->isChecked())) {
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else {
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	}
}


/* ========================================================================= */
/*
 * Slot: fires when date was changed in CalendarWidgets
 */
void DlgCorrespondenceOverview::dateCalendarsChange(const QDate &date)
/* ========================================================================= */
{
	Q_UNUSED(date);

	this->toCalendarWidget->setMinimumDate(
	    this->fromCalendarWidget->selectedDate());

	getMsgListFromDates(this->fromCalendarWidget->selectedDate(),
	    this->toCalendarWidget->selectedDate());
}


/* ========================================================================= */
/*
 * Slot: fires when date was changed in CalendarWidgets
 */
void DlgCorrespondenceOverview::getMsgListFromDates(const QDate &fromDate,
    const QDate &toDate)
/* ========================================================================= */
{
	bool ok = false;
	int sentCnt = 0;
	int receivedCnt = 0;

	m_messages.sentdmIDs = m_messDbSet.msgsDateInterval(fromDate,
	    toDate, MSG_SENT);
	sentCnt = m_messages.sentdmIDs.count();

	m_messages.receivedmIDs = m_messDbSet.msgsDateInterval(fromDate,
	    toDate, MSG_RECEIVED);
	receivedCnt = m_messages.receivedmIDs.count();

	if (sentCnt > 0 || receivedCnt > 0) {
		ok = true;
	}

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);

	msgStateChanged(0);

	QString sentInfo = "(" + tr("messages: ") +
	    QString::number(sentCnt) + ")";
	QString receivedInfo = "(" + tr("messages: ") +
	    QString::number(receivedCnt) + ")";

	this->sentCntLabel->setText(sentInfo);
	this->receivedCntLabel->setText(receivedInfo);
}


/* ========================================================================= */
/*
 * Export into ZFO file.
 */
bool DlgCorrespondenceOverview::exportMessageAsZFO(const MessageDb::MsgId &mId,
    const QString &fileName, bool deliveryInfo) const
/* ========================================================================= */
{
	if (!mId.isValid()) {
		Q_ASSERT(0);
		return false;
	}

	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QByteArray base64;

	const MessageDb *messageDb = m_messDbSet.constAccessMessageDb(
	    mId.deliveryTime);
	Q_ASSERT(0 != messageDb);

	if (deliveryInfo) {
		base64 = messageDb->msgsGetDeliveryInfoBase64(mId.dmId);
	} else {
		base64 = messageDb->msgsMessageBase64(mId.dmId);
	}

	if (base64.isEmpty()) {
		return false;
	}

	QByteArray data = QByteArray::fromBase64(base64);

	return WF_SUCCESS == writeFile(fileName, data);
}



/* ========================================================================= */
/*
 * Export into pdf file.
 */
bool DlgCorrespondenceOverview::exportMessageAsPDF(const MessageDb::MsgId &mId,
    const QString &fileName, bool deliveryInfo) const
/* ========================================================================= */
{
	if (!mId.isValid()) {
		Q_ASSERT(0);
		return false;
	}

	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QTextDocument doc;

	const MessageDb *messageDb = m_messDbSet.constAccessMessageDb(
	    mId.deliveryTime);
	Q_ASSERT(0 != messageDb);

	if (deliveryInfo) {
		doc.setHtml(messageDb->deliveryInfoHtmlToPdf(mId.dmId));
	} else {
		doc.setHtml(messageDb->envelopeInfoHtmlToPdf(mId.dmId, ""));
	}

	if (doc.isEmpty()) {
		return false;
	}

	QPrinter printer;
	printer.setOutputFileName(fileName);
	printer.setOutputFormat(QPrinter::PdfFormat);
	doc.print(&printer);

	return true;
}


/* ========================================================================= */
/*
 * Add message to HTML.
 */
QString DlgCorrespondenceOverview::msgInHtml(const MessageDb::MsgId &mId) const
/* ========================================================================= */
{
	if (!mId.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	const MessageDb *messageDb = m_messDbSet.constAccessMessageDb(
	    mId.deliveryTime);
	Q_ASSERT(0 != messageDb);

	QStringList messageItems = messageDb->getMsgForHtmlExport(mId.dmId);
	if (messageItems.empty()) {
		return QString();
	}

	return "<div><table><tr><td><table>"
	    "<tr><td><b>"
	    + QString::number(mId.dmId) +
	    "</b></td></tr>"
	    "<tr><td class=\"smaller\">"
	    + messageItems.at(3) +
	    "</td></tr>"
	    "<tr><td class=\"smaller\">"
	    + messageItems.at(4) +
	    "</td></tr>"
	    "</table></td><td><table><tr><td>"
	     + tr("Subject:") +
	    "</td><td><i><b>"
	    + messageItems.at(2) +
	    "</b></i></td></tr><tr><td>"
	    + tr("Sender:") +
	    "</td><td><i>"
	    + messageItems.at(0) +
	    "</i></td></tr><tr><td>"
	    + tr("Recipient:") +
	    "</td><td><i>"
	    + messageItems.at(1) +
	    "</i></td></tr></table></td></tr></table></div>";
}


/* ========================================================================= */
/*
 * Add message to CSV.
 */
QString DlgCorrespondenceOverview::msgInCsv(const MessageDb::MsgId &mId) const
/* ========================================================================= */
{
	if (!mId.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	const MessageDb *messageDb = m_messDbSet.constAccessMessageDb(
	    mId.deliveryTime);
	Q_ASSERT(0 != messageDb);

	QStringList messageItems = messageDb->getMsgForCsvExport(mId.dmId);
	if (messageItems.empty()) {
		return QString();
	}

	QString content = QString::number(mId.dmId);

	for (int i = 0; i < messageItems.count(); ++i) {
		content += "," + messageItems.at(i);
	}

	return content;
}


/* ========================================================================= */
/*
 * Export messages to HTML.
 */
bool DlgCorrespondenceOverview::exportMessagesToHtml(
    const QString &fileName) const
/* ========================================================================= */
{
	qDebug() << "Files are export to HTML format";

	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream f(&fout);
	/* Generate CSV header. */
	f << "<!DOCTYPE html\n"
	    "   PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
	    "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
	    "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
	    "<head>\n"
	    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
	    "<title>"
	    + tr("Correspondence overview") +
	    "</title>\n"
	    "<style type=\"text/css\">\n"
	    "   td {padding: 0px 5px; }\n"
	    "   div { border-bottom: solid 1px black;}\n"
	    "   body { font-family:Arial, sans; font-size: 12pt;}\n"
	    "   *.smaller { font-size: smaller; }\n"
	    "</style>\n"
	    "</head>\n"
	    "<body>\n"
	    "<h1>"
	    + tr("Correspondence overview") +
	    "</h1>\n"
	    "<table><tr><td>\n"
	    + tr("From date:") +
	    "</td><td>"
	    + this->fromCalendarWidget->selectedDate().toString("dd.MM.yyyy") +
	    "</td></tr><tr><td>"
	    + tr("To date:") +
	    "</td><td>"
	    + this->toCalendarWidget->selectedDate().toString("dd.MM.yyyy") +
	    "</td></tr><tr><td>"
	    + tr("Generated:") +
	    "</td><td>"
	    + QDateTime().currentDateTime().toString("dd.MM.yyyy hh:mm:ss") +
	    "</td></tr></table>\n";


	/* sent messages */
	if (this->sentCheckBox->isChecked()) {

		f << "<h2>" << tr("Sent") << "</h2>\n";

		foreach (const MessageDb::MsgId &mId, m_messages.sentdmIDs) {
			f << msgInHtml(mId);
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {

		f << "<h2>" << tr("Received") << "</h2>\n";

		foreach (const MessageDb::MsgId &mId, m_messages.receivedmIDs) {
			f << msgInHtml(mId);
		}
	}

	f << "</body>\n</html>";

	if (!fout.flush()) {
		return false;
	}
	fout.close();

	return true;
}


/* ========================================================================= */
/*
 * Export messages to CSV.
 */
bool DlgCorrespondenceOverview::exportMessagesToCsv(
    const QString &fileName) const
/* ========================================================================= */
{
	qDebug() << "Files are export to CSV format";

	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream f(&fout);
	/* Generate CSV header. */
	f << "ID," + tr("Status") + "," + tr("Message type") + "," +
	    tr("Delivery time") + "," + tr("Acceptance time") + "," +
	    tr("Subject") + "," + tr("Sender") + "," +
	    tr("Sender Address") + "," + tr("Recipient") + "," +
	    tr("Recipient Address") + "," + tr("Our file mark") + "," +
	    tr("Our reference number") + "," + tr("Your file mark") + "," +
	    tr("Your reference number") + "\n";

	/* sent messages */
	if (this->sentCheckBox->isChecked()) {
		foreach (const MessageDb::MsgId &mId, m_messages.sentdmIDs) {
			f << msgInCsv(mId) + "\n";
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {
		foreach (const MessageDb::MsgId &mId, m_messages.receivedmIDs) {
			f << msgInCsv(mId) + "\n";
		}
	}

	if (!fout.flush()) {
		return false;
	}
	fout.close();

	return true;
}


/* ========================================================================= */
/*
 * Slot: fires when date was changed in CalendarWidgets
 */
void DlgCorrespondenceOverview::exportData(void)
/* ========================================================================= */
{
	QString tmpMsg = "";
	QString exportDir;

	QString overviewFileName = m_exportCorrespondDir + QDir::separator() +
	    tr("Overview") + "--" +
	    this->fromCalendarWidget->selectedDate().toString(Qt::ISODate) +
	    "--" +
	    this->toCalendarWidget->selectedDate().toString(Qt::ISODate) +
	    (("HTML" == this->outputFormatComboBox->currentText()) ?
	        ".html" : ".csv");

	overviewFileName = QFileDialog::getSaveFileName(this,
	    tr("Select file to save correspondence overview"),
	    overviewFileName, tr("Files") + "(*.html *.txt *.csv)");

	if (!overviewFileName.isEmpty()) {
		exportDir =
		    QFileInfo(overviewFileName).absoluteDir().absolutePath();
		m_exportCorrespondDir = exportDir;
		qDebug() << "Correspondence file is exported to:" << exportDir;

		if (this->outputFormatComboBox->currentText() == "HTML") {
			if (!exportMessagesToHtml(overviewFileName)) {
				QMessageBox::warning(this, QObject::tr(
					"Correspondence overview export error."),
				    tr("Correspondence overview file '%1' could"
				    " not be written.").arg(
				        QDir::toNativeSeparators(
				            overviewFileName)),
				    QMessageBox::Ok);
				tmpMsg += "<b>0</b> " + tr("correspondence "
				"overview file was exported to HTML.") +"<br/>";
			} else {
				tmpMsg += "<b>1</b> " + tr("correspondence "
				"overview file was exported to HTML.") +"<br/>";
			}
		} else {
			if (!exportMessagesToCsv(overviewFileName)) {
				QMessageBox::warning(this, QObject::tr(
					"Correspondence overview export error"),
				    tr("Correspondence overview file '%1' could"
				    " not be written.").arg(
				        QDir::toNativeSeparators(
				            overviewFileName)),
				    QMessageBox::Ok);
				tmpMsg += "<b>0</b> " + tr("correspondence "
				"overview file was exported to CVS.") +"<br/>";
			} else {
				tmpMsg += "<b>1</b> " + tr("correspondence "
				"overview file was exported to CVS.") +"<br/>";
			}
		}
	} else {
		tmpMsg += "<b>0</b> " + tr("correspondence overview "
		    "file was exported.") + "<br/>";
	}

	QString fileName;
	QString errorText;
	QStringList errorList;
	errorList.clear();
	int successMsgZFOCnt = 0;
	int successDelInfoZFOCnt = 0;
	int successEnvelopePdfCnt = 0;
	int successDelInfoPdfCnt = 0;

	MessageDb::FilenameEntry entry;

	//QMessageBox msgBoxProc(this);

	if (this->exportZfoCheckBox->isChecked() ||
	    this->exportDeliveryZfoCheckBox->isChecked() ||
	    this->exportMessageEnvelopePDFCheckBox->isChecked() ||
	    this->exportDeliveryPDFCheckBox->isChecked()) {
		exportDir = QFileDialog::getExistingDirectory(this,
		    tr("Select directory for export of ZFO/PDF file(s)"),
		    m_exportCorrespondDir,
		    QFileDialog::ShowDirsOnly |
		        QFileDialog::DontResolveSymlinks); 

		if (exportDir.isEmpty()) {
			tmpMsg += "<b>0</b> " + tr("messages were successfully "
			    "exported to ZFO/PDF.") + "<br/>";
			goto finish;
		} 
		m_exportCorrespondDir = exportDir; 
		qDebug() << "Files will be exported to:" << exportDir;
	} 

	/* TODO - add prograss bar intead of this */

	//msgBoxProc.setIcon(QMessageBox::Information);
	//msgBoxProc.setWindowTitle(tr("Export proccessing... Please wait."));
	//msgBoxProc.setText(tr("Export of files is proccessing... Please wait."));
	//msgBoxProc.setWindowModality(Qt::NonModal);
	//msgBoxProc.show();

	/* export message ZFO */
	if (this->exportZfoCheckBox->isChecked()) {

		/* sent ZFO */
		if (this->sentCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.sentdmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);
				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.message_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".zfo";
				if (!exportMessageAsZFO(mId, fileName, false)) {
					qDebug() << "DDZ" << mId.dmId
					    << "export error";
					errorText = tr("Message '%1' does not "
					    "contain data necessary for ZFO "
					    "export.").
					    arg(QString::number(mId.dmId));
					errorList.append(errorText);
				} else {
					successMsgZFOCnt++;
				}
			}
		}

		/* received ZFO */
		if (this->receivedCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.receivedmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);

				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.message_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".zfo";
				if (!exportMessageAsZFO(mId, fileName, false)) {
					qDebug() << "DDZ" << mId.dmId
					    << "export error";
					errorText = tr("Message '%1' does not "
					    "contain data necessary for ZFO "
					    "export.").
					    arg(QString::number(mId.dmId));
					errorList.append(errorText);
				} else {
					successMsgZFOCnt++;
				}
			}
		}
		tmpMsg += "<b>" + QString::number(successMsgZFOCnt) +
		"</b> " + tr("messages were successfully exported to ZFO.") +
		"<br/>";
	}

	/* export delivery info ZFO */
	if (this->exportDeliveryZfoCheckBox->isChecked()) {

		/* sent ZFO */
		if (this->sentCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.sentdmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);

				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.delivery_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".zfo";
				if (!exportMessageAsZFO(mId, fileName, true)) {
					qDebug() << "DDZ" << mId.dmId
					    << "export error";
					errorText = tr("Message '%1' does not "
					    "contain deivery info data "
					    "necessary for ZFO export.").
					    arg(QString::number(mId.dmId));
					errorList.append(errorText);
				} else {
					successDelInfoZFOCnt++;
				}
			}
		}

		/* received ZFO */
		if (this->receivedCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.receivedmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);

				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.delivery_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".zfo";
				if (!exportMessageAsZFO(mId, fileName, true)) {
					qDebug() << "DDZ" << mId.dmId
					    << "export error";
					errorText = tr("Message '%1' does not "
					    "contain deivery info data "
					    "necessary for ZFO export.").
					    arg(QString::number(mId.dmId));
					errorList.append(errorText);
				} else {
					successDelInfoZFOCnt++;
				}
			}
		}
		tmpMsg += "<b>" + QString::number(successDelInfoZFOCnt) +
		"</b> " + tr("delivery infos were successfully "
		"exported to ZFO.") + "<br/>";
	}

	/* export envelope to PDF */
	if (this->exportMessageEnvelopePDFCheckBox->isChecked()) {
		/* sent PDF */
		if (this->sentCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.sentdmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);

				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.message_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".pdf";
				if (!exportMessageAsPDF(mId, fileName, false)) {
					qDebug() << "OZ" << mId.dmId;
					errorText = tr("Message '%1' does not "
					    "contain message envelope data "
					    "necessary for PDF export.").
					    arg(QString::number(mId.dmId));
					errorList.append(errorText);
				} else {
					successEnvelopePdfCnt++;
				}
			}
		}

		/* received PDF */
		if (this->receivedCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.receivedmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);

				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.message_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".pdf";
				if (!exportMessageAsPDF(mId, fileName, false)) {
					qDebug() << "OZ" << mId.dmId
					    << "export error";
					errorText = tr("Message '%1' does not "
					    "contain message envelope data "
					    "necessary for PDF export.").
					    arg(QString::number(mId.dmId));
				} else {
					successEnvelopePdfCnt++;
				}
			}
		}
		tmpMsg += "<b>" + QString::number(successEnvelopePdfCnt) +
		    "</b> " + tr("message envelopes were successfully "
		    "exported to PDF.") + "<br/>";
	}

	/* export delivery info to PDF */
	if (this->exportDeliveryPDFCheckBox->isChecked()) {
		/* sent PDF */
		if (this->sentCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.sentdmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);

				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.delivery_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".pdf";
				if (!exportMessageAsPDF(mId, fileName, true)) {
					qDebug() << "DD" << mId.dmId
					    << "export error";
					errorText = tr("Message '%1' does not "
					    "contain delivery info data "
					    "necessary for PDF export.").
					    arg(QString::number(mId.dmId));
					errorList.append(errorText);
				} else {
					successDelInfoPdfCnt++;
				}
			}
		}

		/* received PDF */
		if (this->receivedCheckBox->isChecked()) {
			foreach (const MessageDb::MsgId &mId, m_messages.receivedmIDs) {
				const MessageDb *messageDb =
				    m_messDbSet.constAccessMessageDb(
				        mId.deliveryTime);
				Q_ASSERT(0 != messageDb);

				entry = messageDb->msgsGetAdditionalFilenameEntry(mId.dmId);
				fileName = fileNameFromFormat(
				    globPref.delivery_filename_format,
				    mId.dmId, m_dbId, m_userName, "",
				    entry.dmDeliveryTime,
				    entry.dmAcceptanceTime, entry.dmAnnotation,
				    entry.dmSender);

				fileName = exportDir + QDir::separator() +
				    fileName + ".pdf";
				if (!exportMessageAsPDF(mId, fileName, true)) {
					qDebug() << "DD" << mId.dmId
					    << "export error";
					errorText = tr("Message '%1' does not "
					    "contain delivery info data "
					    "necessary for PDF export.").
					    arg(QString::number(mId.dmId));
					errorList.append(errorText);
				} else {
					successDelInfoPdfCnt++;
				}
			}
		}
		tmpMsg += "<b>" + QString::number(successDelInfoPdfCnt) +
		    "</b> " + tr("delivery infos were successfully "
		    "exported to PDF.")  + "<br/>";
	}

	//msgBoxProc.close();

finish:

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setWindowTitle(tr("Export results"));
	msgBox.setText(tr("Export of correspondence overview finished "
	    "with these results:"));

	if (!errorList.isEmpty()) {
		tmpMsg += "<br/><b>" + tr("Some errors occurred "
		    "during export.")  + "</b><br/>" +
		    tr("See detail for more info...") + "<br/><br/>";
	}
	msgBox.setInformativeText(tmpMsg);

	QString msg = "";
	if (!errorList.isEmpty()) {
		for (int i = 0; i < errorList.count(); ++i) {
			msg += errorList.at(i) + "\n";
		}
		msgBox.setDetailedText(msg);
	}

	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
}
