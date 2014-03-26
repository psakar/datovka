#ifndef _DATOVKA_H_
#define _DATOVKA_H_

#define ICON_16x16_PATH "/icons/16x16/"
#define ICON_24x24_PATH "/icons/24x24/"
#define ICON_3PARTY_PATH "/icons/3party/"

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionPreferences_triggered();
    void ShowOnlyInfo();
    bool AddAccountToTree(QString AccountName);
    bool AddMessageIntoRecieved(QString Id, QString Title,
    QString Sender, QString Delivered, QString Accepted);
    bool AddMessageIntoSent(QString Id, QString Title,
    QString Recipient, QString Status, QString Delivered, QString Accepted);
    void on_actionProxy_settings_triggered();

    void on_actionTest_triggered();

private:
    Ui::MainWindow *ui;
};

#endif /* _DATOVKA_H_ */
