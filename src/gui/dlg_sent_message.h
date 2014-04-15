#ifndef DLG_SENT_MESSAGE_H
#define DLG_SENT_MESSAGE_H

#include <QDialog>
#include <QFileDialog>
#include <QTreeView>
#include "src/common.h"
#include "ui_dlg_sent_message.h"

class dlg_sent_message : public QDialog, public Ui::sentMessageDialog {
    Q_OBJECT

public:
	explicit dlg_sent_message(QWidget *parent = 0,
	    QTreeView *accountList = 0, QTableView *messageList = 0,
	    QString action = "Add");

private slots:
	void on_cancelButton_clicked();
	void showOptionalForm(int);
	void addAttachmentFile(void);
	void deleteAttachmentFile(void);
	void openAttachmentFile(void);
	void addRecipientData(void);
	void deleteRecipientData(void);
	void findRecipientData(void);
	void recItemSelect(QTableWidgetItem *item);
	void attItemSelect(QTableWidgetItem *item);

private:
	void initNewMessageDialog(void);
	void sendMessage(void);
	QTreeView *m_accountList;
	QTableView *m_messageList;
	QString m_action;
};

#endif // DLG_SENT_MESSAGE_H
