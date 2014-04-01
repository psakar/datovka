#ifndef DLG_SENT_MESSAGE_H
#define DLG_SENT_MESSAGE_H

#include <QDialog>

#include "ui_dlg_sent_message.h"

class dlg_sent_message : public QDialog, public Ui::SentMessageDialog {
    Q_OBJECT

public:
    explicit dlg_sent_message(QWidget *parent = 0);
};

#endif // DLG_SENT_MESSAGE_H
