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

private:
    Ui::MainWindow *ui;
};

#endif // DATOVKA_H
