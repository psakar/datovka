

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

	DlgCorrespondenceOverview(const MessageDb &db, const QString &dbId,
	    const AccountModel::SettingsMap &accountInfo,
	    QString &exportCorrespondDir, QWidget *parent = 0);

private slots:
	void dateCalendarsChange(const QDate &date);
	void exportData(void);

private:
	const MessageDb &m_messDb;
	const QString m_dbId;
	const AccountModel::SettingsMap m_accountInfo;
	exportMessages m_messages;
	QString &m_exportCorrespondDir;

	void initDialog(void);
	void getMsgListFromDates(const QDate &fromDate, const QDate &toDate);
	QString addMessageToCsv(const QString &dmId);
	QString addMessageToHtml(const QString &dmId);
	bool exportMessageAsZFO(QString dmId, QString exportPath);
	bool exportMessagesToCsv(const QString &fileName);
	bool exportMessagesToHtml(const QString &fileName);

};

#endif // _DLG_CORRESPONDENCE_OVERVIEW_H_
