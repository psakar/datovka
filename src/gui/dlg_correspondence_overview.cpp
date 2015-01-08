
#include <QMessageBox>
#include <QPushButton>

#include "dlg_correspondence_overview.h"

DlgCorrespondenceOverview::DlgCorrespondenceOverview(
    const MessageDb &db, const QString &dbId,
    const AccountModel::SettingsMap &accountInfo,
    QString &exportCorrespondDir, QWidget *parent) :
    QDialog(parent),
    m_messDb(db),
    m_dbId(dbId),
    m_accountInfo(accountInfo),
    m_exportCorrespondDir(exportCorrespondDir)
{
	setupUi(this);
	initDialog();
}


/* ========================================================================= */
/*
 * Init dialog
 */
void DlgCorrespondenceOverview::initDialog(void)
/* ========================================================================= */
{
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

	getMsgListFromDates(this->fromCalendarWidget->selectedDate(),
	    this->toCalendarWidget->selectedDate());

	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(exportData(void)));
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

	QString sentInfo = "(" + tr("messages: ") +
	    QString::number(sentCnt) + ")";
	QString receivedInfo = "(" + tr("messages: ") +
	    QString::number(receivedCnt) + ")";

	this->sentCntLabel->setText(sentInfo);
	this->receivedCntLabel->setText(receivedInfo);
}


/* ========================================================================= */
/*
 * Export message into ZFO file.
 */
bool DlgCorrespondenceOverview::exportMessageAsZFO(QString dmId,
    QString exportPath)
/* ========================================================================= */
{
	QString fileName = "DDZ_" + dmId + ".zfo";

	if (dmId.isNull() || dmId.isEmpty()) {
		return false;
	}

	int dmID = atoi(dmId.toStdString().c_str());

	QString raw = QString(m_messDb.msgsMessageBase64(dmID)).toUtf8();
	if (raw.isEmpty()) {
		return false;
	}

	fileName = exportPath + QDir::separator() + fileName;

	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	return WF_SUCCESS == writeFile(fileName, data);
}


/* ========================================================================= */
/*
 * Add message to HTML.
 */
QString DlgCorrespondenceOverview::addMessageToHtml(const QString &dmId)
/* ========================================================================= */
{
	if (dmId.isNull() || dmId.isEmpty()) {
		return "";
	}

	int dmID = atoi(dmId.toStdString().c_str());
	QList<QString> messageItems = m_messDb.getMsgForHtmlExport(dmID);

	if (messageItems.empty()) {
		return "";
	}

	return "<div><table><tr><td><table>"
	    "<tr><td><b>"
	    + dmId +
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
QString DlgCorrespondenceOverview::addMessageToCsv(const QString &dmId)
/* ========================================================================= */
{
	QString content;

	if (dmId.isNull() || dmId.isEmpty()) {
		return "";
	}

	int dmID = atoi(dmId.toStdString().c_str());
	QList<QString> messageItems = m_messDb.getMsgForCsvExport(dmID);

	content = dmId;

	for (int i = 0; i < messageItems.count(); ++i) {
		content += "," + messageItems.at(i);
	}

	content += "\n";

	return content;
}


/* ========================================================================= */
/*
 * Export messages to HTML.
 */
bool DlgCorrespondenceOverview::exportMessagesToHtml(const QString &fileName)
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

		for (int i = 0; i < m_messages.sentdmIDs.count(); ++i) {
			f << addMessageToHtml(m_messages.sentdmIDs.at(i));
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {

		f << "<h2>" << tr("Received") << "</h2>\n";

		for (int i = 0; i < m_messages.receivedmIDs.count(); ++i){
			f << addMessageToHtml(m_messages.receivedmIDs.at(i));
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
bool DlgCorrespondenceOverview::exportMessagesToCsv(const QString &fileName)
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
		for (int i = 0; i < m_messages.sentdmIDs.count(); ++i) {
			f << addMessageToCsv(m_messages.sentdmIDs.at(i));
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {
		for (int i = 0; i < m_messages.receivedmIDs.count(); ++i) {
			f << addMessageToCsv(m_messages.receivedmIDs.at(i));
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

	QString overiviewFileName;

	if (this->outputFormatComboBox->currentText() == "HTML") {
		overiviewFileName = exportDir + QDir::separator() +
		    tr("Overview-") +
		    this->toCalendarWidget->selectedDate().toString(Qt::ISODate) +
		    ".html";

		if (!exportMessagesToHtml(overiviewFileName)) {
			QMessageBox::warning(this, QObject::tr(
			        "Correspondence overview export error."),
			    tr("Correspondence overview file '%1' could not "
			        "be written.").arg(overiviewFileName),
			    QMessageBox::Ok);
		}
	} else {
		overiviewFileName = exportDir + QDir::separator() +
		    tr("Overview-") +
		    this->toCalendarWidget->selectedDate().toString(Qt::ISODate) +
		    ".txt";

		if (!exportMessagesToCsv(overiviewFileName)) {
			QMessageBox::warning(this, QObject::tr(
			        "Correspondence overview export error"),
			    tr("Correspondence overview file '%1' could not "
			        "be written.").arg(overiviewFileName),
			    QMessageBox::Ok);
		}
	}

	QList<QString> errorDmId;
	errorDmId.clear();
	int successCnt = 0;

	if (this->exportZfoCheckBox->isChecked()) {

		/* sent ZFO */
		if (this->sentCheckBox->isChecked()) {

			for (int i = 0; i < m_messages.sentdmIDs.count(); ++i) {
				if (!exportMessageAsZFO(m_messages.sentdmIDs.at(i),
				    exportDir)) {
					/* TODO - add dialog describes error */
					qDebug() << "DDZ"
					    << m_messages.sentdmIDs.at(i)
					    << "export error";
					errorDmId.append(m_messages.sentdmIDs.at(i));
				} else {
					successCnt++;
				}
			}
		}

		/* received ZFO */
		if (this->receivedCheckBox->isChecked()) {

			for (int i = 0; i < m_messages.receivedmIDs.count(); ++i) {
				if (!exportMessageAsZFO(m_messages.receivedmIDs.at(i),
				    exportDir)) {
					/* TODO - add dialog describes error */
					qDebug() << "DDZ"
					    << m_messages.receivedmIDs.at(i)
					    << "export error";
					errorDmId.append(m_messages.receivedmIDs.at(i));
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
			msg += tr("Message") + " " + errorDmId.at(i) + " " +
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
