

#ifndef _DLG_CORRESPONDENCE_OVERVIEW_H_
#define _DLG_CORRESPONDENCE_OVERVIEW_H_

#include <QDialog>
#include <QFileDialog>
#include <QTreeView>
#include <QTableView>

#include "src/common.h"
#include "ui_dlg_correspondence_overview.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"


class DlgCorrespondenceOverview : public QDialog,
    public Ui::CorrespondenceOverview {
    Q_OBJECT

public:
	struct exportMessages {
		QList<QString> sentdmIDs;
		QList<QString> receivedmIDs;
	};

	DlgCorrespondenceOverview(MessageDb &db, QString &dbId,
	    QTreeView &accountList, QTableView &messageList,
	    const AccountModel::SettingsMap &accountInfo,
	    QString &export_correspond_dir, QWidget *parent = 0);

signals:
	void showNotificationDialog(QList<QString>, int);

private slots:
	void dateCalendarsChange(QDate date);
	void exportData(void);

private:
	MessageDb &m_messDb;
	const QString m_dbId;
	QTreeView &m_accountList;
	QTableView &m_messageList;
	AccountModel::SettingsMap m_accountInfo;
	exportMessages messages;
	QString &m_export_correspond_dir;

	void initDialog(void);
	void getMsgListFromDates(QDate fromDate, QDate toDate);
	QString  addMessageToCsv(QString dmId);
	QString  addMessageToHtml(QString dmId);
	bool exportMessageAsZFO(QString dmId, QString exportPath);
	bool exportMessagesToCsv(QString exportPath);
	bool exportMessagesToHtml(QString exportPath);
	QString addHeaderToCsv(void);
	QString addHeaderToHtml(void);

};

#endif // _DLG_CORRESPONDENCE_OVERVIEW_H_
