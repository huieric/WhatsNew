#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QSqlDatabase&, QWidget *parent = 0);
    ~MainWindow();

signals:


public slots:
    void update();

private:
    Ui::MainWindow *ui;
    QSqlTableModel *user_model, *message_model, *file_model;
};

#endif // MAINWINDOW_H
