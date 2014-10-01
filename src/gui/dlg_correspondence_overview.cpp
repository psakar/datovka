#include <QPushButton>

#include "dlg_correspondence_overview.h"

DlgCorrespondenceOverview::DlgCorrespondenceOverview(
    MessageDb &db, QString &dbId,
    QTreeView &accountList, QTableView &messageList,
    const AccountModel::SettingsMap &accountInfo, QWidget *parent) :
    QDialog(parent),
    m_messDb(db),
    m_dbId(dbId),
    m_accountList(accountList),
    m_messageList(messageList),
    m_accountInfo(accountInfo)
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
void DlgCorrespondenceOverview::dateCalendarsChange(QDate date)
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
void DlgCorrespondenceOverview::getMsgListFromDates(QDate fromDate, QDate toDate)
/* ========================================================================= */
{
	bool ok = false;
	int sentCnt = 0;
	int receivedCnt = 0;

	messages.sentdmIDs = m_messDb.msgsDateInterval(fromDate,
	    toDate, true);
	sentCnt = messages.sentdmIDs.count();

	messages.receivedmIDs = m_messDb.msgsDateInterval(fromDate,
	    toDate, false);
	receivedCnt = messages.receivedmIDs.count();

	if (sentCnt > 0 || receivedCnt > 0) {
		ok = true;
	}

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);

	QString sentInfo = tr("(messages: ") +
	    QString::number(sentCnt) + ")";
	QString receivedInfo = tr("(messages: ") +
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
	int dmID;
	QString fileName = "DDZ_" + dmId + ".zfo";

	if (dmId.isNull() || dmId.isEmpty()) {
		return false;
	}

	dmID = atoi(dmId.toStdString().c_str());

	QString raw = QString(m_messDb.msgsGetMessageRaw(dmID)).toUtf8();
	if (raw.isEmpty()) {
		return false;
	}

	fileName = exportPath + "/" + fileName;

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly)) {
		return false;
	}

	QByteArray rawutf8= QString(raw).toUtf8();
	QByteArray data = QByteArray::fromBase64(rawutf8);

	int written = fout.write(data);
	if (written != data.size()) {

	}

	fout.close();

	return true;
}


/* ========================================================================= */
/*
 * Add message to HTML.
 */
QString DlgCorrespondenceOverview::addMessageToHtml(QString dmId)
/* ========================================================================= */
{
	if (dmId.isNull() || dmId.isEmpty()) {
		return "";
	}

	int dmID;
	dmID = atoi(dmId.toStdString().c_str());
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
QString DlgCorrespondenceOverview::addMessageToCsv(QString dmId)
/* ========================================================================= */
{
	QString content;
	int dmID;

	if (dmId.isNull() || dmId.isEmpty()) {
		return "";
	}

	dmID = atoi(dmId.toStdString().c_str());
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
bool DlgCorrespondenceOverview::exportMessagesToHtml(QString exportPath)
/* ========================================================================= */
{
	qDebug() << "Files are export to HTML format";

	QString fileName = QString(tr("Overview-")) +
	    this->toCalendarWidget->selectedDate().toString(Qt::ISODate) +
	    ".html";

	fileName = exportPath + "/" + fileName;

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream f(&fout);
	f << addHeaderToHtml();


	/* sent messages */
	if (this->sentCheckBox->isChecked()) {

		f << "<h2>" << tr("Sent") << "</h2>\n";

		for (int i = 0; i < messages.sentdmIDs.count(); ++i) {
			f << addMessageToHtml(messages.sentdmIDs.at(i));
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {

		f << "<h2>" << tr("Received") << "</h2>\n";

		for (int i = 0; i < messages.receivedmIDs.count(); ++i){
			f << addMessageToHtml(messages.receivedmIDs.at(i));
		}
	}

	f << "</body>\n</html>";

	fout.close();

	return true;
}


/* ========================================================================= */
/*
 * Add header to html.
 */
QString DlgCorrespondenceOverview::addHeaderToHtml(void)
/* ========================================================================= */
{
	QDateTime now = QDateTime().currentDateTime();

	return "<!DOCTYPE html\n"
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
	    + now.toString("dd.MM.yyyy hh:mm:ss") +
	    "</td></tr></table>\n";
}


/* ========================================================================= */
/*
 * Add header to CSV.
 */
QString DlgCorrespondenceOverview::addHeaderToCsv(void)
/* ========================================================================= */
{
	return "ID," +
	    tr("Status") + "," +
	    tr("Message type") + "," +
	    tr("Delivery time") + "," +
	    tr("Acceptance time") + "," +
	    tr("Subject") + "," +
	    tr("Sender") + "," +
	    tr("Sender Address") + "," +
	    tr("Recipient") + "," +
	    tr("Recipient Address") + "," +
	    tr("Our file mark") + "," +
	    tr("Our reference number") + "," +
	    tr("Your file mark") + "," +
	    tr("Your reference number") + "\n";
}


/* ========================================================================= */
/*
 * Export messages to CSV.
 */
bool DlgCorrespondenceOverview::exportMessagesToCsv(QString exportPath)
/* ========================================================================= */
{
	qDebug() << "Files are export to CSV format";

	QString fileName = QString(tr("Overview-")) +
	    this->toCalendarWidget->selectedDate().toString(Qt::ISODate) +
	    ".txt";

	fileName = exportPath + "/" + fileName;

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream f(&fout);
	f << addHeaderToCsv();

	/* sent messages */
	if (this->sentCheckBox->isChecked()) {
		for (int i = 0; i < messages.sentdmIDs.count(); ++i) {
			f << addMessageToCsv(messages.sentdmIDs.at(i));
		}
	}

	/* received messages */
	if (this->receivedCheckBox->isChecked()) {
		for (int i = 0; i < messages.receivedmIDs.count(); ++i) {
			f << addMessageToCsv(messages.receivedmIDs.at(i));
		}
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
	QString importDir = QFileDialog::getExistingDirectory(this,
	    tr("Select directory to save correspondence"),
	    NULL, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (importDir.isEmpty() || importDir.isNull()) {
		return;
	}

	qDebug() << "Files are export to:" << importDir;

	if (this->outputFormatComboBox->currentText() == "HTML") {
		exportMessagesToHtml(importDir);
	} else {
		exportMessagesToCsv(importDir);
	}

	if (this->exportZfoCheckBox->isChecked()) {
		/* sent ZFO */
		for (int i = 0; i < messages.sentdmIDs.count(); ++i) {
			if (!exportMessageAsZFO(messages.sentdmIDs.at(i),
			    importDir)) {
				/* TODO - add dialog describes error */
				qDebug() << "DDZ" << messages.sentdmIDs.at(i)
				    << "export error";
			}
		}
		/* received ZFO */
		for (int i = 0; i < messages.receivedmIDs.count(); ++i) {
			if (!exportMessageAsZFO(messages.receivedmIDs.at(i),
			    importDir)) {
    				/* TODO - add dialog describes error */
				qDebug() << "DDZ" << messages.receivedmIDs.at(i)
				    << "export error";
			}
		}
	}
}
