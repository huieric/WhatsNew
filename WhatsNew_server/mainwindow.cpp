#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QSqlDatabase &db, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    user_model = new QSqlTableModel(this, db);
    message_model = new QSqlTableModel(this, db);
    file_model = new QSqlTableModel(this, db);
    user_model->setEditStrategy(QSqlTableModel::OnRowChange);
    message_model->setEditStrategy(QSqlTableModel::OnRowChange);
    file_model->setEditStrategy(QSqlTableModel::OnRowChange);

    user_model->setTable("User");
    user_model->select();
    ui->userTableView->setModel(user_model);

    message_model->setTable("FriendsList");
    message_model->select();
    ui->messageTableView->setModel(message_model);

    file_model->setTable("addList");
    file_model->select();
    ui->fileTableView->setModel(file_model);
}

void MainWindow::update()
{
    user_model->select();
    message_model->select();
    file_model->select();
}

MainWindow::~MainWindow()
{
    delete ui;
}
