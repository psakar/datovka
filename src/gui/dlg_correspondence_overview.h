

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
	class ExportedMessageList {
	public:
		QList<int> sentdmIDs;
		QList<int> receivedmIDs;
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
	ExportedMessageList m_messages;
	QString &m_exportCorrespondDir;

	void getMsgListFromDates(const QDate &fromDate, const QDate &toDate);
	QString msgInCsv(int dmId) const;
	QString msgInHtml(int dmId) const;
	bool exportMessageAsZFO(int dmId, const QString &fileName) const;
	bool exportMessagesToCsv(const QString &fileName) const;
	bool exportMessagesToHtml(const QString &fileName) const;

};

#endif // _DLG_CORRESPONDENCE_OVERVIEW_H_
