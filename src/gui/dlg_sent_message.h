#ifndef DLG_SENT_MESSAGE_H
#define DLG_SENT_MESSAGE_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_sent_message.h"

class dlg_sent_message : public QDialog, public Ui::sentMessageDialog {
    Q_OBJECT

public:
	explicit dlg_sent_message(QWidget *parent = 0);

private slots:
	void on_cancelButton_clicked();
	void showOptionalForm(int);

private:
	void initNewMessageDialog(void);
	void sendMessage(void);

};

#endif // DLG_SENT_MESSAGE_H
