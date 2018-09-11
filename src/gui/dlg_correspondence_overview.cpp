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
#include <QPrinter>
#include <QPushButton>
#include <QTextStream>

#include "src/delegates/tag_item.h"
#include "src/global.h"
#include "src/gui/dlg_correspondence_overview.h"
#include "src/gui/dlg_msg_box_informative.h"
#include "src/io/exports.h"
#include "src/io/filesystem.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"
#include "ui_dlg_correspondence_overview.h"

#define CSV_LITERAL QStringLiteral("CSV")
#define HTML_LITERAL QStringLiteral("HTML")

DlgCorrespondenceOverview::DlgCorrespondenceOverview(const MessageDbSet &dbSet,
    const QString &dbId, const QString &userName, TagDb &tagDb, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgCorrespondenceOverview),
    m_messDbSet(dbSet),
    m_dbId(dbId),
    m_tagDb(tagDb),
    m_exportedMsgs()
{
	m_ui->setupUi(this);
	/* Tab order is defined in UI file. */

	Q_ASSERT(!userName.isEmpty());

	m_ui->accountName->setText(
	    (*GlobInstcs::acntMapPtr)[userName].accountName() +
	    QStringLiteral(" (") + userName + QStringLiteral(")"));

	m_ui->toCalendarWidget->setMinimumDate(
	    m_ui->fromCalendarWidget->selectedDate());

	{
		const QDate currentDate(QDate::currentDate());
		m_ui->toCalendarWidget->setMaximumDate(currentDate);
		m_ui->fromCalendarWidget->setMaximumDate(currentDate);
	}

	m_ui->outputFormatComboBox->addItem(CSV_LITERAL);
	m_ui->outputFormatComboBox->addItem(HTML_LITERAL);

	connect(m_ui->outputFormatComboBox, SIGNAL(currentIndexChanged(QString)),
	    this, SLOT(reftectOverviewTypeChange(QString)));

	connect(m_ui->fromCalendarWidget, SIGNAL(clicked(QDate)),
	    this, SLOT(reftectCalendarChange()));

	connect(m_ui->toCalendarWidget, SIGNAL(clicked(QDate)),
	    this, SLOT(reftectCalendarChange()));

	connect(m_ui->sentCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(checkMsgTypeSelection()));

	connect(m_ui->receivedCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(checkMsgTypeSelection()));

	m_ui->groupBox->setEnabled(false);

	updateExportedMsgList(m_ui->fromCalendarWidget->selectedDate(),
	    m_ui->toCalendarWidget->selectedDate());
	updateOkButtonActivity();
}

DlgCorrespondenceOverview::~DlgCorrespondenceOverview(void)
{
	delete m_ui;
}

void DlgCorrespondenceOverview::exportData(const MessageDbSet &dbSet,
    const QString &dbId, const QString &userName, TagDb &tagDb,
    QString &exportCorrespondDir, QWidget *parent)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	DlgCorrespondenceOverview dlg(dbSet, dbId, userName, tagDb, parent);
	if (QDialog::Accepted != dlg.exec()) {
		return;
	}

	dlg.exportChosenData(userName, exportCorrespondDir);
}

void DlgCorrespondenceOverview::reftectOverviewTypeChange(const QString &text)
{
	m_ui->groupBox->setEnabled(text == HTML_LITERAL);
}

void DlgCorrespondenceOverview::checkMsgTypeSelection(void)
{
	updateOkButtonActivity();
}

void DlgCorrespondenceOverview::reftectCalendarChange(void)
{
	m_ui->toCalendarWidget->setMinimumDate(
	    m_ui->fromCalendarWidget->selectedDate());

	updateExportedMsgList(m_ui->fromCalendarWidget->selectedDate(),
	    m_ui->toCalendarWidget->selectedDate());
	updateOkButtonActivity();
}

void DlgCorrespondenceOverview::updateOkButtonActivity(void)
{
	/* Enabled the button if there are some messages to be exported. */
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    (m_ui->sentCheckBox->isChecked() &&
	     (m_exportedMsgs.sentDmIDs.count() > 0)) ||
	    (m_ui->receivedCheckBox->isChecked() &&
	     (m_exportedMsgs.receivedDmIDs.count() > 0)));
}

void DlgCorrespondenceOverview::updateExportedMsgList(const QDate &fromDate,
    const QDate &toDate)
{
	m_exportedMsgs.sentDmIDs = m_messDbSet.msgsDateInterval(fromDate,
	    toDate, MSG_SENT);
	m_exportedMsgs.receivedDmIDs = m_messDbSet.msgsDateInterval(fromDate,
	    toDate, MSG_RECEIVED);

	m_ui->sentCntLabel->setText(QStringLiteral("(") + tr("messages: ") +
	    QString::number(m_exportedMsgs.sentDmIDs.count()) +
	    QStringLiteral(")"));
	m_ui->receivedCntLabel->setText(QStringLiteral("(") + tr("messages: ") +
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

	QStringList messageItems(messageDb->getMessageForCsvExport(mId.dmId));
	if (messageItems.empty()) {
		return QString();
	}

	QString content(QString::number(mId.dmId));

	for (int i = 0; i < messageItems.count(); ++i) {
		content += QStringLiteral(",") + messageItems.at(i);
	}

	return content;
}

/*!
 * @brief Creates HTML string containing tag list.
 *
 * @param[in] tagList List of tags.
 * @param[in] useColours If colour entries should be added.
 * @return HTML string.
 */
static
QString tagHtmlString(const TagItemList &tagList, bool useColours)
{
	QStringList tagStrings;

	if (!useColours) {
		foreach (const TagItem &tag, tagList) {
			tagStrings.append(tag.name);
		}
	} else {
		foreach (const TagItem &tag, tagList) {
			QColor bgCol(QStringLiteral("#") + tag.colour);
			QColor textCol(
			    TagItem::adjustForegroundColour(Qt::black, bgCol));
			QString tagStr(
			    QStringLiteral("<span style=\"background-color: "));
			tagStr += bgCol.name();
			tagStr += QStringLiteral("; color: ");
			tagStr += textCol.name();
			tagStr += QStringLiteral(";\">");
			tagStr += QStringLiteral("&nbsp;");
			tagStr += tag.name;
			tagStr += QStringLiteral("&nbsp;");
			tagStr += QStringLiteral("</span>");
			tagStrings.append(tagStr);
		}
	}

	return tagStrings.join(QStringLiteral(", "));
}

/*!
 * @brief Creates tag-related HTML entry.
 *
 * @param[in] tagDb Tag database.
 * @param[in] userName User name identifying account.
 * @param[in] msgId Message identifier.
 * @param[in] useColours True if coloured tags should be generated.
 * @return Tag entry if some tags found. Return empty string on error or when
 *     no tags found.
 */
static
QString tagHtmlEntry(TagDb &tagDb, const QString &userName, qint64 msgId,
    bool useColours)
{
	if (userName.isEmpty() || (msgId < 0)) {
		Q_ASSERT(0);
		return QString();
	}

	TagItemList tagList(tagDb.getMessageTags(userName, msgId));
	if (tagList.isEmpty()) {
		return QString();
	}
	tagList.sortNames();

	QStringList tagStrings;
	foreach (const TagItem &tag, tagList) {
		tagStrings.append(tag.name);
	}

	QString retStr(
	    QStringLiteral("<table><tr><td><table><tr><td valign=\"top\">")
	    + QObject::tr("Tags") +
	    QStringLiteral(":</td><td>"));
	retStr += tagHtmlString(tagList, useColours);
	retStr += QStringLiteral("</td></tr></table></td></tr></table>");

	return retStr;
}

QString DlgCorrespondenceOverview::msgHtmlEntry(const QString &userName,
    const MessageDb::MsgId &mId) const
{
	if (!mId.isValid()) {
		Q_ASSERT(0);
		return QString();
	}

	const MessageDb *messageDb = m_messDbSet.constAccessMessageDb(
	    mId.deliveryTime);
	Q_ASSERT(Q_NULLPTR != messageDb);

	QStringList messageItems(messageDb->getMessageForHtmlExport(mId.dmId));
	if (messageItems.empty()) {
		return QString();
	}

	QString retStr(
	    QStringLiteral("<div><table><tr><td><table>"
	                   "<tr><td>")
	    + QStringLiteral("Id:") +
	    QStringLiteral("</td><td><b>")
	    + QString::number(mId.dmId) +
	    QStringLiteral("</b></td></tr>"
	                   "<tr><td>")
	    + tr("Delivery") +
	    QStringLiteral(":</td><td class=\"smaller\">")
	    + messageItems.at(3) +
	    QStringLiteral("</td></tr>"
	                   "<tr><td>")
	    + tr("Acceptance") +
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
	    QStringLiteral("</i></td></tr></table></td></tr></table>"));
	if (m_ui->addTagsCheckBox->checkState() == Qt::Checked) {
		retStr += tagHtmlEntry(m_tagDb, userName, mId.dmId,
		    m_ui->colourTagsCheckBox->checkState() == Qt::Checked);
	}
	retStr += QStringLiteral("</div>");

	return retStr;
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
	if (m_ui->sentCheckBox->isChecked()) {
		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.sentDmIDs) {
			f << msgCsvEntry(mId) + QStringLiteral("\n");
		}
	}

	/* Received messages. */
	if (m_ui->receivedCheckBox->isChecked()) {
		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.receivedDmIDs) {
			f << msgCsvEntry(mId) + QStringLiteral("\n");
		}
	}

	fout.flush();
	fout.close();

	return true;
}

bool DlgCorrespondenceOverview::writeHtmlOverview(const QString &userName,
    const QString &fileName) const
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
	    "   PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
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
	    + m_ui->fromCalendarWidget->selectedDate().toString("dd.MM.yyyy") +
	    QStringLiteral("</td></tr><tr><td>")
	    + tr("To date:") +
	    QStringLiteral("</td><td>")
	    + m_ui->toCalendarWidget->selectedDate().toString("dd.MM.yyyy") +
	    QStringLiteral("</td></tr><tr><td>")
	    + tr("Generated:") +
	    QStringLiteral("</td><td>")
	    + QDateTime().currentDateTime().toString("dd.MM.yyyy hh:mm:ss") +
	    QStringLiteral("</td></tr></table>\n");

	/* Sent messages. */
	if (m_ui->sentCheckBox->isChecked()) {
		f << QStringLiteral("<h2>") << tr("Sent")
		    << QStringLiteral("</h2>\n");

		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.sentDmIDs) {
			f << msgHtmlEntry(userName, mId);
		}
	}

	/* Received messages. */
	if (m_ui->receivedCheckBox->isChecked()) {
		f << QStringLiteral("<h2>") << tr("Received")
		    << QStringLiteral("</h2>\n");

		foreach (const MessageDb::MsgId &mId, m_exportedMsgs.receivedDmIDs) {
			f << msgHtmlEntry(userName, mId);
		}
	}

	f << QStringLiteral("</body>\n</html>");

	fout.flush();
	fout.close();

	return true;
}

QString DlgCorrespondenceOverview::exportOverview(const QString &userName,
    const QString &dir, QString &summary)
{
	QString exportDir;

	QString overviewFileName(dir + QDir::separator() + tr("Overview") +
	    QStringLiteral("--") +
	    m_ui->fromCalendarWidget->selectedDate().toString(Qt::ISODate) +
	    QStringLiteral("--") +
	    m_ui->toCalendarWidget->selectedDate().toString(Qt::ISODate));
	overviewFileName +=
	    (m_ui->outputFormatComboBox->currentText() == HTML_LITERAL) ?
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
		    m_ui->outputFormatComboBox->currentText() == HTML_LITERAL;
		bool overviewWritten = writeHtml ?
		    writeHtmlOverview(userName, overviewFileName) :
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
		qWarning("%s",
		    QString("DZ '%1' export error.").arg(dmId).toUtf8().constData());
		errList.append(
		    QObject::tr("Message '%1' does not contain data necessary for ZFO export.")
		        .arg(dmId));
		break;
	case Exports::ZFO_DELIVERY:
		qWarning("%s",
		    QString("DZ '%1' export error").arg(dmId).toUtf8().constData());
		errList.append(
		    QObject::tr("Message '%1' does not contain acceptance info data necessary for ZFO export.")
		        .arg(dmId));
		break;
	case Exports::PDF_ENVELOPE:
		qWarning("%s",
		    QString("OZ '%1' export error").arg(dmId).toUtf8().constData());
		errList.append(
		    QObject::tr("Message '%1' does not contain message envelope data necessary for PDF export.")
		        .arg(dmId));
		break;
	case Exports::PDF_DELIVERY:
		qWarning("%s",
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
	Exports::ExportError ret;

	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[userName].accountName());

	foreach (const MessageDb::MsgId &mId, mIds) {
		ret = Exports::exportAs(parent, dbSet, fileType, targetPath,
		    QString(), userName, accountName, dbId, mId, false,
		    lastPath, errStr);
		if (Exports::EXP_SUCCESS == ret) {
			++successCnt;
		} else if (Exports::EXP_CANCELED == ret) {
			break;
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
		    exportOverview(userName, exportCorrespondDir, summaryMsg));
		if (!saveDir.isEmpty()) {
			exportCorrespondDir = saveDir;
		}
	}

	QStringList errorList;
	int successMsgZFOCnt = 0;
	int successDelInfoZFOCnt = 0;
	int successEnvelopePdfCnt = 0;
	int successDelInfoPdfCnt = 0;

	if (m_ui->exportZfoCheckBox->isChecked() ||
	    m_ui->exportDeliveryZfoCheckBox->isChecked() ||
	    m_ui->exportMessageEnvelopePDFCheckBox->isChecked() ||
	    m_ui->exportDeliveryPDFCheckBox->isChecked()) {
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
	if (m_ui->exportZfoCheckBox->isChecked()) {
		if (m_ui->sentCheckBox->isChecked()) {
			successMsgZFOCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::ZFO_MESSAGE, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (m_ui->receivedCheckBox->isChecked()) {
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
	if (m_ui->exportDeliveryZfoCheckBox->isChecked()) {
		if (m_ui->sentCheckBox->isChecked()) {
			successDelInfoZFOCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::ZFO_DELIVERY, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (m_ui->receivedCheckBox->isChecked()) {
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
	if (m_ui->exportMessageEnvelopePDFCheckBox->isChecked()) {
		if (m_ui->sentCheckBox->isChecked()) {
			successEnvelopePdfCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::PDF_ENVELOPE, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (m_ui->receivedCheckBox->isChecked()) {
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
	if (m_ui->exportDeliveryPDFCheckBox->isChecked()) {
		if (m_ui->sentCheckBox->isChecked()) {
			successDelInfoPdfCnt += exportMessageData(
			    m_exportedMsgs.sentDmIDs, this, m_messDbSet,
			    Exports::PDF_DELIVERY, exportDir, userName,
			    m_dbId, lastPath, errorList);
		}
		if (m_ui->receivedCheckBox->isChecked()) {
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
	if (!errorList.isEmpty()) {
		summaryMsg += QStringLiteral("<br/><b>") +
		    tr("Some errors occurred during export.") +
		    QStringLiteral("</b><br/>") +
		    tr("See detail for more info...") +
		    QStringLiteral("<br/><br/>");
	}
	QString detailMsg;
	if (!errorList.isEmpty()) {
		for (int i = 0; i < errorList.count(); ++i) {
			detailMsg += errorList.at(i) + QStringLiteral("\n");
		}
	}
	DlgMsgBox::message(this, QMessageBox::Information, tr("Export results"),
	    tr("Export of correspondence overview finished with these results:"),
	    summaryMsg, detailMsg, QMessageBox::Ok, QMessageBox::Ok);
}
