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


/* ========================================================================= */
/*
 * Constructor.
 */
DlgCorrespondenceOverview::DlgCorrespondenceOverview(
    const MessageDb &db, const QString &dbId,
    const AccountModel::SettingsMap &accountInfo,
    QString &exportCorrespondDir, QWidget *parent) :
    QDialog(parent),
    m_messDb(db),
    m_dbId(dbId),
    m_accountInfo(accountInfo),
    m_exportCorrespondDir(exportCorrespondDir)
/* ========================================================================= */
{
	setupUi(this);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	QString accountName = m_accountInfo.accountName()
	    + " ("+ m_accountInfo.userName() + ")";
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
	(void) state; /* Unused. */

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
	(void) date; // not used

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

	m_messages.sentdmIDs = m_messDb.msgsDateInterval(fromDate,
	    toDate, true);
	sentCnt = m_messages.sentdmIDs.count();

	m_messages.receivedmIDs = m_messDb.msgsDateInterval(fromDate,
	    toDate, false);
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
bool DlgCorrespondenceOverview::exportMessageAsZFO(int dmId,
    const QString &fileName, bool deliveryInfo) const
/* ========================================================================= */
{
	Q_ASSERT(dmId >= 0);
	if (dmId < 0) {
		return false;
	}

	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
		return false;
	}

	QByteArray base64;

	if (deliveryInfo) {
		base64 = m_messDb.msgsGetDeliveryInfoBase64(dmId);
	} else {
		base64 = m_messDb.msgsMessageBase64(dmId);
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
bool DlgCorrespondenceOverview::exportMessageAsPDF(int dmId,
    const QString &fileName, bool deliveryInfo) const
/* ========================================================================= */
{
	Q_ASSERT(dmId >= 0);
	if (dmId < 0) {
		return false;
	}

	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
		return false;
	}

	QTextDocument doc;

	if (deliveryInfo) {
		doc.setHtml(m_messDb.deliveryInfoHtmlToPdf(dmId));
	} else {
		doc.setHtml(m_messDb.envelopeInfoHtmlToPdf(dmId, ""));
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
QString DlgCorrespondenceOverview::msgInHtml(int dmId) const
/* ========================================================================= */
{
	Q_ASSERT(dmId >= 0);
	if (dmId < 0) {
		return QString();
	}

	QStringList messageItems = m_messDb.getMsgForHtmlExport(dmId);
	if (messageItems.empty()) {
		return QString();
	}

	return "<div><table><tr><td><table>"
	    "<tr><td><b>"
	    + QString::number(dmId) +
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
QString DlgCorrespondenceOverview::msgInCsv(int dmId) const
/* ========================================================================= */
{
	Q_ASSERT(dmId >= 0);
	if (dmId < 0) {
		return QString();
	}

	QStringList messageItems = m_messDb.getMsgForCsvExport(dmId);
	if (messageItems.empty()) {
		return QString();
	}

	QString content = QString::number(dmId);

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

	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
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

		foreach (int dmId, m_messages.sentdmIDs) {
			f << msgInHtml(dmId);
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {

		f << "<h2>" << tr("Received") << "</h2>\n";

		foreach (int dmId, m_messages.receivedmIDs) {
			f << msgInHtml(dmId);
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

	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
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
		foreach (int dmId, m_messages.sentdmIDs) {
			f << msgInCsv(dmId) + "\n";
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {
		foreach (int dmId, m_messages.receivedmIDs) {
			f << msgInCsv(dmId) + "\n";
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
	QString exportDir = QFileDialog::getExistingDirectory(this,
	    tr("Select directory to save correspondence"),
	    m_exportCorrespondDir,
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (exportDir.isEmpty() || exportDir.isNull()) {
		return;
	}

	m_exportCorrespondDir = exportDir;

	qDebug() << "Files are export to:" << exportDir;

	QString overiviewFileName = exportDir + QDir::separator() +
	    tr("Overview") + "--" +
	    this->fromCalendarWidget->selectedDate().toString(Qt::ISODate) +
	    "--" +
	    this->toCalendarWidget->selectedDate().toString(Qt::ISODate);

	if (this->outputFormatComboBox->currentText() == "HTML") {
		overiviewFileName += ".html";

		if (!exportMessagesToHtml(overiviewFileName)) {
			QMessageBox::warning(this, QObject::tr(
			        "Correspondence overview export error."),
			    tr("Correspondence overview file '%1' could not "
			        "be written.").arg(overiviewFileName),
			    QMessageBox::Ok);
			return;
		}
	} else {
		overiviewFileName += ".csv";

		if (!exportMessagesToCsv(overiviewFileName)) {
			QMessageBox::warning(this, QObject::tr(
			        "Correspondence overview export error"),
			    tr("Correspondence overview file '%1' could not "
			        "be written.").arg(overiviewFileName),
			    QMessageBox::Ok);
			return;
		}
	}

	QList<int> errorDmId;
	errorDmId.clear();
	int successCnt = 0;

	/* export message ZFO */
	if (this->exportZfoCheckBox->isChecked()) {

		/* sent ZFO */
		if (this->sentCheckBox->isChecked()) {
			foreach (int dmId, m_messages.sentdmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "DDZ_" + QString::number(dmId) + ".zfo";
				if (!exportMessageAsZFO(dmId, fileName, false)) {
					/* TODO - add dialog describes error */
					qDebug() << "DDZ" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}

		/* received ZFO */
		if (this->receivedCheckBox->isChecked()) {
			foreach (int dmId, m_messages.receivedmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "DDZ_" + QString::number(dmId) + ".zfo";
				if (!exportMessageAsZFO(dmId, fileName, false)) {
					/* TODO - add dialog describes error */
					qDebug() << "DDZ" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}
	}

	/* export delivery info ZFO */
	if (this->exportDeliveryZfoCheckBox->isChecked()) {

		/* sent ZFO */
		if (this->sentCheckBox->isChecked()) {
			foreach (int dmId, m_messages.sentdmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "DDZ_" + QString::number(dmId) + "_info.zfo";
				if (!exportMessageAsZFO(dmId, fileName, true)) {
					/* TODO - add dialog describes error */
					qDebug() << "DDZ" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}

		/* received ZFO */
		if (this->receivedCheckBox->isChecked()) {
			foreach (int dmId, m_messages.receivedmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "DDZ_" + QString::number(dmId) + "_info.zfo";
				if (!exportMessageAsZFO(dmId, fileName, true)) {
					/* TODO - add dialog describes error */
					qDebug() << "DDZ" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}
	}

	/* export envelope to PDF */
	if (this->exportMessageEnvelopePDFCheckBox->isChecked()) {
		/* sent ZFO */
		if (this->sentCheckBox->isChecked()) {
			foreach (int dmId, m_messages.sentdmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "OZ_" + QString::number(dmId) + ".pdf";
				if (!exportMessageAsPDF(dmId, fileName, false)) {
					/* TODO - add dialog describes error */
					qDebug() << "OZ" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}

		/* received ZFO */
		if (this->receivedCheckBox->isChecked()) {
			foreach (int dmId, m_messages.receivedmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "OZ_" + QString::number(dmId) + ".pdf";
				if (!exportMessageAsPDF(dmId, fileName, false)) {
					/* TODO - add dialog describes error */
					qDebug() << "OZ" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}
	}

	/* export delivery info to PDF */
	if (this->exportDeliveryPDFCheckBox->isChecked()) {
		/* sent ZFO */
		if (this->sentCheckBox->isChecked()) {
			foreach (int dmId, m_messages.sentdmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "DD_" + QString::number(dmId) + ".pdf";
				if (!exportMessageAsPDF(dmId, fileName, true)) {
					/* TODO - add dialog describes error */
					qDebug() << "DD" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}

		/* received ZFO */
		if (this->receivedCheckBox->isChecked()) {
			foreach (int dmId, m_messages.receivedmIDs) {
				QString fileName =
				    exportDir + QDir::separator() +
				    "DD_" + QString::number(dmId) + ".pdf";
				if (!exportMessageAsPDF(dmId, fileName, true)) {
					/* TODO - add dialog describes error */
					qDebug() << "DD" << dmId
					    << "export error";
					errorDmId.append(dmId);
				} else {
					successCnt++;
				}
			}
		}
	}

	if (!errorDmId.isEmpty()) {
		QString msg = tr("There were some errors during saving of "
		    "the overview:") + "\n\n";

		for (int i = 0; i < errorDmId.count(); ++i) {
			msg += tr("Message") + " " +
			   QString::number(errorDmId.at(i)) + " " +
			   tr("does not contain data necessary for ZFO export") + ".\n";
			if (i > 10) {
				msg += tr("And many more") + "...\n";
				break;
			}
		}

		msg += "\n" + QString::number(successCnt) + " " +
		    tr("messages were successfully exported to ZFO") + ".\n";

		QMessageBox::warning(this,
		    QObject::tr("Correspondence export error"), msg, QMessageBox::Ok);
	}
}
