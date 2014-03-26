#ifndef _DATOVKA_H_
#define _DATOVKA_H_

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
    bool AddMessageIntoTable();
    void on_actionProxy_settings_triggered();

    void on_actionTest_triggered();

private:
    Ui::MainWindow *ui;
};

#endif /* _DATOVKA_H_ */
