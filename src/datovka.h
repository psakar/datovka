#ifndef DATOVKA_H
#define DATOVKA_H

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
    void on_actionProxy_settings_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // DATOVKA_H
