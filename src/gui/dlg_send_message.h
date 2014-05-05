

#ifndef _DLG_SEND_MESSAGE_H_
#define _DLG_SEND_MESSAGE_H_


#include <QDialog>
#include <QFileDialog>
#include <QTreeView>

#include "src/common.h"
#include "src/io/message_db.h"
#include "ui_dlg_send_message.h"


class DlgSendMessage : public QDialog, public Ui::SendMessage {
    Q_OBJECT

public:
	enum Action {
		ACT_NEW,
		ACT_REPLY
	};

	DlgSendMessage(MessageDb &db, Action action,
	    QTreeView &accountList, QTableView &messageList,
	    QWidget *parent = 0,
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

	QTreeView &m_accountList;
	QTableView &m_messageList;
	const Action m_action;
	QString m_reSubject;
	QString m_senderId;
	QString m_sender;
	QString m_senderAddress;
	QString m_userName;

	MessageDb &m_messDb;
};


#endif /* _DLG_SEND_MESSAGE_H_ */
