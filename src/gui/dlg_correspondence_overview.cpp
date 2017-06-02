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
#include <QPrinter>
#include <QPushButton>
#include <QTextStream>

#include "src/gui/dlg_correspondence_overview.h"
#include "src/io/exports.h"
#include "src/io/filesystem.h"
#include "src/models/accounts_model.h"
#include "src/settings/preferences.h"

#define CSV_LITERAL QStringLiteral("CSV")
#define HTML_LITERAL QStringLiteral("HTML")

DlgCorrespondenceOverview::DlgCorrespondenceOverview(const MessageDbSet &dbSet,
    const QString &dbId, const QString &userName, QWidget *parent)
    : QDialog(parent),
    m_messDbSet(dbSet),
    m_dbId(dbId),
    m_exportedMsgs()
{
	setupUi(this);

	Q_ASSERT(!userName.isEmpty());

	this->accountName->setText(
	    AccountModel::globAccounts[userName].accountName() +
	    QStringLiteral(" (") + userName + QStringLiteral(")"));

	this->toCalendarWidget->setMinimumDate(this->fromCalendarWidget->selectedDate());

	QDate currentDate(QDate().currentDate());
	this->toCalendarWidget->setMaximumDate(currentDate);
	this->fromCalendarWidget->setMaximumDate(currentDate);

	this->outputFormatComboBox->addItem(CSV_LITERAL);
	this->outputFormatComboBox->addItem(HTML_LITERAL);

	connect(this->fromCalendarWidget, SIGNAL(clicked(QDate)),
	    this, SLOT(reftectCalendarChange()));

	connect(this->toCalendarWidget, SIGNAL(clicked(QDate)),
	    this, SLOT(reftectCalendarChange()));

	connect(this->sentCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(checkMsgTypeSelection()));

	connect(this->receivedCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(checkMsgTypeSelection()));

	updateExportedMsgList(this->fromCalendarWidget->selectedDate(),
	    this->toCalendarWidget->selectedDate());
	updateOkButtonActivity();
}

void DlgCorrespondenceOverview::exportData(const MessageDbSet &dbSet,
    const QString &dbId, const QString &userName, QString &exportCorrespondDir,
    QWidget *parent)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	DlgCorrespondenceOverview dlg(dbSet, dbId, userName, parent);
	if (QDialog::Accepted != dlg.exec()) {
		return;
	}

	dlg.exportChosenData(userName, exportCorrespondDir);
}

void DlgCorrespondenceOverview::checkMsgTypeSelection(void)
{
	updateOkButtonActivity();
}

void DlgCorrespondenceOverview::reftectCalendarChange(void)
{
	this->toCalendarWidget->setMinimumDate(
	    this->fromCalendarWidget->selectedDate());

	updateExportedMsgList(this->fromCalendarWidget->selectedDate(),
	    this->toCalendarWidget->selectedDate());
	updateOkButtonActivity();
}

void DlgCorrespondenceOverview::updateOkButtonActivity(void)
{
	/* Enabled the button if there are some messages to be exported. */
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    (this->sentCheckBox->isChecked() &&
	     (m_exportedMsgs.sentDmIDs.count() > 0)) ||
	    (this->receivedCheckBox->isChecked() &&
	     (m_exportedMsgs.receivedDmIDs.count() > 0)));
}

void DlgCorrespondenceOverview::updateExportedMsgList(const QDate &fromDate,
    const QDate &toDate)
{
	m_exportedMsgs.sentDmIDs = m_messDbSet.msgsDateInterval(fromDate,
	    toDate, MSG_SENT);
	m_exportedMsgs.receivedDmIDs = m_messDbSet.msgsDateInterval(fromDate,
	    toDate, MSG_RECEIVED);

	this->sentCntLabel->setText(QStringLiteral("(") + tr("messages: ") +
	    QString::number(m_exportedMsgs.sentDmIDs.count()) +
	    QStringLiteral(")"));
	this->receivedCntLabel->setText(QStringLiteral("(") + tr("messages: ") +
	    QString::number(m_exportedMsgs.receivedDmIDs.count()) +
	    QStringLiteral(")"));
}

QString DlgCorrespondenceOverview::msgCsvEntry(
    const MessageDb::MsgId &mId) const
{
	if (!mId.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	const MessageDb *messageDb = m_messDbSet.constAccessMessageDb(
	    mId.deliveryTime);
	Q_ASSERT(Q_NULLPTR != messageDb);

	QStringList messageItems(messageDb->getMsgForCsvExport(mId.dmId));
	if (messageItems.empty()) {
		return QString();
	}

	QString content(QString::number(mId.dmId));

	for (int i = 0; i < messageItems.count(); ++i) {
		content += QStringLiteral(",") + messageItems.at(i);
	}

	return content;
}

QString DlgCorrespondenceOverview::msgHtmlEntry(
    const MessageDb::MsgId &mId) const
{
	if (!mId.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	const MessageDb *messageDb = m_messDbSet.constAccessMessageDb(
	    mId.deliveryTime);
	Q_ASSERT(Q_NULLPTR != messageDb);

	QStringList messageItems(messageDb->getMsgForHtmlExport(mId.dmId));
	if (messageItems.empty()) {
		return QString();
	}

	return
	    QStringLiteral("<div><table><tr><td><table>"
	                   "<tr><td>")
	    + QStringLiteral("Id:") +
	    QStringLiteral("</td><td><b>")
	    + QString::number(mId.dmId) +
	    QStringLiteral("</b></td></tr>"
	                   "<tr><td>")
	    + tr("Delivery time") +
	    QStringLiteral(":</td><td class=\"smaller\">")
	    + messageItems.at(3) +
	    QStringLiteral("</td></tr>"
	                   "<tr><td>")
	    + tr("Acceptance time") +
	    QStringLiteral(":</td><td class=\"smaller\">")
	    + messageItems.at(4) +
	    QStringLiteral("</td></tr>"
	                   "</table></td><td><table><tr><td>")
	    + tr("Subject") +
	    QStringLiteral(":</td><td><i><b>")
	    + messageItems.at(2) +
	    QStringLiteral("</b></i></td></tr><tr><td>")
	    + tr("Sender") +
	    QStringLiteral(":</td><td><i>")
	    + messageItems.at(0) +
	    QStringLiteral("</i></td></tr><tr><td>")
	    + tr("Recipient") +
	    QStringLiteral(":</td><td><i>")
	    + messageItems.at(1) +
	    QStringLiteral("</i></td></tr></table></td></tr></table></div>");
}

bool DlgCorrespondenceOverview::writeCsvOverview(const QString &fileName) const
{
	qDebug("Files are going be be exported to CSV file '%s'.",
	    fileName.toUtf8().constData());

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
	f << QStringLiteral("ID,") +
	    tr("Status") + QStringLiteral(",") +
	    tr("Message type") + QStringLiteral(",") +
	    tr("Delivery time") + QStringLiteral(",") +
	    tr("Acceptance time") + QStringLiteral(",") +
	    tr("Subject") + QStringLiteral(",") +
	    tr("Sender") + QStringLiteral(",") +
	    tr("Sender Address") + QStringLiteral(",") +
	    tr("Recipient") + QStringLiteral(",") +
	    tr("Recipient Address") + QStringLiteral(",") +
	    tr("Our file mark") + QStringLiteral(",") +
	    tr("Our reference number") + QStringLiteral(",") +
	    tr("Your file mark") + QStringLiteral(",") +
	    tr("Your reference number") + QStringLiteral("\n");

	/* Sent messages. */
	if (this->sentCheckBox->isChecked()) {
		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.sentDmIDs) {
			f << msgCsvEntry(mId) + QStringLiteral("\n");
		}
	}

	/* Received messages. */
	if (this->receivedCheckBox->isChecked()) {
		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.receivedDmIDs) {
			f << msgCsvEntry(mId) + QStringLiteral("\n");
		}
	}

	fout.flush();
	fout.close();

	return true;
}

bool DlgCorrespondenceOverview::writeHtmlOverview(const QString &fileName) const
{
	qDebug("Files are going be be exported to HTML file '%s'.",
	    fileName.toUtf8().constData());

	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream f(&fout);
	/*
	 * Always use UTF-8 for this HTML file as this encoding is hard-wired
	 * into its header.
	 */
	f.setCodec("UTF-8");
	/* Generate HTML header. */
	f << QStringLiteral("<!DOCTYPE html\n"
	    "   PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
	    "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
	    "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
	    "<head>\n"
	    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
	    "<title>")
	    + tr("Correspondence overview") +
	    QStringLiteral("</title>\n"
	    "<style type=\"text/css\">\n"
	    "   td {padding: 0px 5px; }\n"
	    "   div { border-bottom: solid 1px black;}\n"
	    "   body { font-family:Arial, sans; font-size: 12pt;}\n"
	    "   *.smaller { font-size: smaller; }\n"
	    "</style>\n"
	    "</head>\n"
	    "<body>\n"
	    "<h1>")
	    + tr("Correspondence overview") +
	    QStringLiteral("</h1>\n"
	    "<table><tr><td>\n")
	    + tr("From date:") +
	    QStringLiteral("</td><td>")
	    + this->fromCalendarWidget->selectedDate().toString("dd.MM.yyyy") +
	    QStringLiteral("</td></tr><tr><td>")
	    + tr("To date:") +
	    QStringLiteral("</td><td>")
	    + this->toCalendarWidget->selectedDate().toString("dd.MM.yyyy") +
	    QStringLiteral("</td></tr><tr><td>")
	    + tr("Generated:") +
	    QStringLiteral("</td><td>")
	    + QDateTime().currentDateTime().toString("dd.MM.yyyy hh:mm:ss") +
	    QStringLiteral("</td></tr></table>\n");

	/* Sent messages. */
	if (this->sentCheckBox->isChecked()) {
		f << QStringLiteral("<h2>") << tr("Sent")
		    << QStringLiteral("</h2>\n");

		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.sentDmIDs) {
			f << msgHtmlEntry(mId);
		}
	}

	/* Received messages. */
	if (this->receivedCheckBox->isChecked()) {
		f << QStringLiteral("<h2>") << tr("Received")
		    << QStringLiteral("</h2>\n");

		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.receivedDmIDs) {
			f << msgHtmlEntry(mId);
		}
	}

	f << QStringLiteral("</body>\n</html>");

	fout.flush();
	fout.close();

	return true;
}

QString DlgCorrespondenceOverview::exportOverview(const QString &dir,
    QString &summary)
{
	QString exportDir;

	QString overviewFileName(dir + QDir::separator() + tr("Overview") +
	    QStringLiteral("--") +
	    this->fromCalendarWidget->selectedDate().toString(Qt::ISODate) +
	    QStringLiteral("--") +
	    this->toCalendarWidget->selectedDate().toString(Qt::ISODate));
	overviewFileName +=
	    (this->outputFormatComboBox->currentText() == HTML_LITERAL) ?
	        QStringLiteral(".html") : QStringLiteral(".csv");

	overviewFileName = QFileDialog::getSaveFileName(this,
	    tr("Select file to save correspondence overview"), overviewFileName,
	    tr("Files") + QStringLiteral("(*.html *.txt *.csv)"));

	if (!overviewFileName.isEmpty()) {
		exportDir =
		    QFileInfo(overviewFileName).absoluteDir().absolutePath();
		qDebug("Correspondence file is going to be exported into directory '%s'.",
		    exportDir.toUtf8().constData());

		bool writeHtml =
		    this->outputFormatComboBox->currentText() == HTML_LITERAL;
		bool overviewWritten = writeHtml ?
		    writeHtmlOverview(overviewFileName) :
		    writeCsvOverview(overviewFileName);
		if (!overviewWritten) {
			QMessageBox::warning(this,
			    tr("Correspondence Overview Export Error"),
			    tr("Correspondence overview file '%1' could not be written.")
			        .arg(QDir::toNativeSeparators(overviewFileName)),
			    QMessageBox::Ok);
		}
		summary += (overviewWritten ?
		    QStringLiteral("<b>1</b> ") : QStringLiteral("<b>0</b> ")) +
		    (writeHtml ?
		        tr("correspondence overview file was exported to HTML.") :
		        tr("correspondence overview file was exported to CSV.")) +
		    QStringLiteral("<br/>");
	} else {
		summary += QStringLiteral("<b>0</b> ") +
		    tr("correspondence overview file was exported.") +
		    QStringLiteral("<br/>");
	}

	return exportDir;
}

/*!
 * @brief Appends error string to error list.
 *
 * @param[in]     fileType Type of generated file.
 * @param[in]     dmId Message identifier.
 * @param[in,out] errList String list to append error message to.
 */
static
void appendError(enum Exports::ExportFileType fileType, qint64 dmId,
    QStringList &errList)
{
	switch (fileType) {
	case Exports::ZFO_MESSAGE:
		qWarning(
		    QString("DZ '%1' export error.").arg(dmId).toUtf8().constData());
		errList.append(
		    QObject::tr("Message '%1' does not contain data necessary for ZFO export.")
		        .arg(dmId));
		break;
	case Exports::ZFO_DELIVERY:
		qWarning(
		    QString("DZ '%1' export error").arg(dmId).toUtf8().constData());
		errList.append(
		    QObject::tr("Message '%1' does not contain acceptance info data necessary for ZFO export.")
		        .arg(dmId));
		break;
	case Exports::PDF_ENVELOPE:
		qWarning(
		    QString("OZ '%1' export error").arg(dmId).toUtf8().constData());
		errList.append(
		    QObject::tr("Message '%1' does not contain message envelope data necessary for PDF export.")
		        .arg(dmId));
		break;
	case Exports::PDF_DELIVERY:
		qWarning(
		    QString("DD '%1' export error").arg(dmId).toUtf8().constData());
		errList.append(
		    QObject::tr("Message '%1' does not contain acceptance info data necessary for PDF export.")
		        .arg(dmId));
		break;
	case Exports::PDF_DELIV_ATTACH:
	default:
		Q_ASSERT(0);
		break;
	}
}

/*!
 * @brief Exports messages into files of given type.
 *
 * @param[in] mIds List of message identifiers.
 * @param[in] parent Widget parent.
 * @param[in] dbSet Database set.
 * @param[in] fileType Type of files to be generated.
 * @param[in] targetPath Location of created files.
 * @param[in] userName Login identifying the account.
 * @param[in] dbId Account database identifier.
 * @param[in,out] lastPath Last used path.
 * @param[out] errList List of error strings.
 */
static
int exportMessageData(const QList<MessageDb::MsgId> &mIds,
    QWidget *parent, const MessageDbSet &dbSet,
    enum Exports::ExportFileType fileType, const QString &targetPath,
    const QString &userName, const QString &dbId, QString &lastPath,
    QStringList &errList)
{
	QString errStr;

	int successCnt = 0;

	foreach (const MessageDb::MsgId &mId, mIds) {
		if (Exports::EXP_SUCCESS == Exports::exportAs(parent, dbSet,
		        fileType, targetPath, QString(), userName, dbId, mId,
		        false, lastPath, errStr)) {
			++successCnt;
		} else {
			appendError(fileType, mId.dmId, errList);
		}
	}

	return successCnt;
}

void DlgCorrespondenceOverview::exportChosenData(const QString &userName,
    QString &exportCorrespondDir)
{
	QString summaryMsg;
	QString exportDir;
	QString lastPath;

	{
		const QString saveDir(
		    exportOverview(exportCorrespondDir, summaryMsg));
		if (!saveDir.isEmpty()) {
			exportCorrespondDir = saveDir;
		}
	}

	QStringList errorList;
	int successMsgZFOCnt = 0;
	int successDelInfoZFOCnt = 0;
	int successEnvelopePdfCnt = 0;
	int successDelInfoPdfCnt = 0;

	if (this->exportZfoCheckBox->isChecked() ||
	    this->exportDeliveryZfoCheckBox->isChecked() ||
	    this->exportMessageEnvelopePDFCheckBox->isChecked() ||
	    this->exportDeliveryPDFCheckBox->isChecked()) {
		exportDir = QFileDialog::getExistingDirectory(this,
		    tr("Select directory for export of ZFO/PDF file(s)"),
		    exportCorrespondDir,
		    QFileDialog::ShowDirsOnly |
		        QFileDialog::DontResolveSymlinks); 

		if (exportDir.isEmpty()) {
			summaryMsg += QStringLiteral("<b>0</b> ") +
			    tr("messages were successfully exported to ZFO/PDF.") +
			    QStringLiteral("<br/>");
			goto finish;
		} 
		exportCorrespondDir = exportDir;
		qDebug("Files are going to be exported to directory '%s'.",
		    exportDir.toUtf8().constData());
	} 

	/* Export messages to ZFO. */
	if (this->exportZfoCheckBox->isChecked()) {
		if (this->sentCheckBox->isChecked()) {
			successMsgZFOCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::ZFO_MESSAGE, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (this->receivedCheckBox->isChecked()) {
			successMsgZFOCnt += exportMessageData(
			    m_exportedMsgs.receivedDmIDs, this, m_messDbSet,
			    Exports::ZFO_MESSAGE, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		summaryMsg += QStringLiteral("<b>") +
		    QString::number(successMsgZFOCnt) +
		    QStringLiteral("</b> ") +
		    tr("messages were successfully exported to ZFO.") +
		    QStringLiteral("<br/>");
	}

	/* Export delivery info ZFO. */
	if (this->exportDeliveryZfoCheckBox->isChecked()) {
		if (this->sentCheckBox->isChecked()) {
			successDelInfoZFOCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::ZFO_DELIVERY, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (this->receivedCheckBox->isChecked()) {
			successDelInfoZFOCnt += exportMessageData(
			    m_exportedMsgs.receivedDmIDs, this, m_messDbSet,
			    Exports::ZFO_DELIVERY, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		summaryMsg += QStringLiteral("<b>") +
		    QString::number(successDelInfoZFOCnt) +
		    QStringLiteral("</b> ") +
		    tr("acceptance infos were successfully exported to ZFO.") +
		    QStringLiteral("<br/>");
	}

	/* Export envelope to PDF. */
	if (this->exportMessageEnvelopePDFCheckBox->isChecked()) {
		if (this->sentCheckBox->isChecked()) {
			successEnvelopePdfCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::PDF_ENVELOPE, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (this->receivedCheckBox->isChecked()) {
			successEnvelopePdfCnt += exportMessageData(
			    m_exportedMsgs.receivedDmIDs, this, m_messDbSet,
			    Exports::PDF_ENVELOPE, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		summaryMsg += QStringLiteral("<b>") +
		    QString::number(successEnvelopePdfCnt) +
		    QStringLiteral("</b> ") +
		    tr("message envelopes were successfully exported to PDF.") +
		    QStringLiteral("<br/>");
	}

	/* Export delivery info to PDF. */
	if (this->exportDeliveryPDFCheckBox->isChecked()) {
		if (this->sentCheckBox->isChecked()) {
			successDelInfoPdfCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::PDF_DELIVERY, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (this->receivedCheckBox->isChecked()) {
			successDelInfoPdfCnt += exportMessageData(
			    m_exportedMsgs.receivedDmIDs, this, m_messDbSet,
			    Exports::PDF_DELIVERY, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		summaryMsg += QStringLiteral("<b>") +
		    QString::number(successDelInfoPdfCnt) +
		    QStringLiteral("</b> ") +
		    tr("acceptance infos were successfully exported to PDF.") +
		    QStringLiteral("<br/>");
	}

finish:
	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setWindowTitle(tr("Export results"));
	msgBox.setText(
	    tr("Export of correspondence overview finished with these results:"));

	if (!errorList.isEmpty()) {
		summaryMsg += QStringLiteral("<br/><b>") +
		    tr("Some errors occurred during export.") +
		    QStringLiteral("</b><br/>") +
		    tr("See detail for more info...") +
		    QStringLiteral("<br/><br/>");
	}
	msgBox.setInformativeText(summaryMsg);

	QString msg;
	if (!errorList.isEmpty()) {
		for (int i = 0; i < errorList.count(); ++i) {
			msg += errorList.at(i) + QStringLiteral("\n");
		}
		msgBox.setDetailedText(msg);
	}

	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
}
