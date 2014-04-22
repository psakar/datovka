

#ifndef _DLG_SEND_MESSAGE_H_
#define _DLG_SEND_MESSAGE_H_


#include <QDialog>
#include <QFileDialog>
#include <QTreeView>

#include "src/common.h"
#include "src/io/message_db.h"
#include "ui_dlg_send_message.h"


class DlgSendMessage : public QDialog, public Ui::sentMessageDialog {
    Q_OBJECT

public:
	explicit DlgSendMessage(MessageDb &db, QWidget *parent = 0,
	    QTreeView *accountList = 0, QTableView *messageList = 0,
	    const QString &action = "New",
	    const QString &reSubject = QString(),
	    const QString &senderId = QString(),
	    const QString &sender = QString(),
	    const QString &senderAddress = QString());

private slots:
	void on_cancelButton_clicked(void);
	void showOptionalForm(int);
	void addAttachmentFile(void);
	void deleteAttachmentFile(void);
	void openAttachmentFile(void);
	void addRecipientData(void);
	void deleteRecipientData(void);
	void findRecipientData(void);
	void recItemSelect(void);
	void attItemSelect(void);
	void checkInputFields(void);
	void tableItemInsRem(void);

private:
	void initNewMessageDialog(void);
	void sendMessage(void);
	QTreeView *m_accountList;
	QTableView *m_messageList;
	QString m_action;
	QString reSubject;
	QString senderId;
	QString sender;
	QString senderAddress;

	MessageDb &m_messDb;
};


#endif /* _DLG_SEND_MESSAGE_H_ */
