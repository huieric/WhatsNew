#include <QApplication>
#include "mainwindow.h"
#include "login.h"
#include "server.h"

const quint16 server_port = 6666;
const quint16 client_port = 8888;
static bool connectToDatabase(QSqlDatabase&);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSqlDatabase db;
    if (!connectToDatabase(db))
        return 1;

    MainWindow w(db);
    Server server;
    server.startServer();
    QObject::connect(&server, SIGNAL(dbChanged()), &w, SLOT(update()));

    w.show();

    return a.exec();
}

static bool connectToDatabase(QSqlDatabase &db)
{
    db = QSqlDatabase::addDatabase("QSQLITE", "Manager");
    db.setDatabaseName("WhatsNew.db");
    if (!db.open()) {
        QMessageBox::critical(0, "Cannot open database",
                     "Unable to establish a database connection.\n"
                     "This example needs SQLite support. Please read "
                     "the Qt SQL driver documentation for information how "
                     "to build it.\n\n"
                     "Click Cancel to exit.", QMessageBox::Cancel);
        return false;
    }
    QSqlQuery query(db);
    query.exec("create table User"
               "("
               "user_id int not null primary key,"
               "nickname varchar(10),"
               "password varchar(30),"
               "email varchar(30),"
               "gender int default -1,"
               "region varchar(100) default '这个人是流浪汉~',"
               "avatar varchar(100) default './avatar/default.png',"
               "isOnline int default 0,"
               "ip varchar(30) default null"
               ")");
    //qDebug() << query.lastError().text();
    query.exec("create table Message"
               "("
               "message_id int primary key not null,"
               "user_from int,"
               "user_to int,"
               "content varchar(1000),"
               "time timestamp"
               ")");
    //qDebug() << query.lastError().text();
    query.exec("create table File"
               "("
               "file_id int primary key not null,"
               "user_from int,"
               "user_to int,"
               "file_name varchar(50),"
               "file_size int,"
               "file varbinary(1000000000),"
               "time timestamp"
               ")");
    //qDebug() << query.lastError().text();
    query.exec("create table FriendsList"
               "("
               "id int primary key not null,"
               "user1 int,"
               "user2 int,"
               "lastChat timestamp"
               ")");
    //qDebug() << query.lastError().text();
    query.exec("create table addList"
               "("
               "id int primary key not null,"
               "user_from int not null,"
               "user_to int not null,"
               "reason varchar(100) not null,"
               "requestTime varchar(50)"
               ")");
    //query.exec("delete from Message");
    return true;
}
