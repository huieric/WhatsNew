#include "server.h"

Server::Server(QObject *parent)
    : QTcpServer(parent)
{

}

//void Server::stop() //公有方法stop()改变私有标志stopped以停止server线程
//{
//    tcpServer->close();
//    qDebug() << "Server closed.";
//}

void Server::startServer()
{
    if(!this->listen(QHostAddress::Any, server_port))
    {
        qDebug() << "Could not start server...";
    }
    else
    {
        qDebug() << "Listening to port " << server_port << "...";
    }
}

// This function is called by QTcpServer when a new connection is available.
void Server::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << socketDescriptor <<" Connecting...";
    MyThread *thread = new MyThread(socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(dbChanged()), this, SLOT(emit_dbChanged()));
    connect(thread, SIGNAL(sendRequest(int,int,QString,QString)), this, SLOT(sendRequest(int,int,QString,QString)));
    connect(thread, SIGNAL(requestAgreed(int,int)), this, SLOT(notice(int, int)));
    thread->start();
}

void Server::emit_dbChanged()
{
    emit dbChanged();
}

void Server::sendRequest(int user_from, int user_to, const QString nickname, const QString reason)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "Server");
    db.setDatabaseName("WhatsNew.db");
    if(!db.open())
    {
        qDebug() << "Error opening database";
        return;
    }
    qDebug() << "Successfully open database...";
    QSqlQuery query(db);
    qDebug() << "Connecting to added client...";
    query.prepare("select ip from User where user_id = ?");
    query.addBindValue(user_to);
    if(!query.exec()) qDebug() << query.lastError().text();
    query.next();
    const QString ip = query.value(0).toString();
    const int Timeout = 5 * 1000;
    QByteArray outBlock;
    QDataStream out(&outBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);
    QTcpSocket *socket = new QTcpSocket();
    socket->abort();
    socket->connectToHost(ip, client_port);
    if(!socket->waitForConnected(Timeout))
        qDebug() << "Client timeout...";
    else
    {
        qint8 messageType = 6;
        qint64 messageSize = sizeof(nickname) + sizeof(reason) + sizeof(user_from);
        out << messageType << messageSize << user_from << nickname << reason;
        socket->write(outBlock);
        socket->disconnectFromHost();
    }
    db.close();
}

void Server::notice(int user_from, int user_to)
{
    QString connection_name = QString("Server")+QString::number(user_from, 10)+QString::number(user_to, 10);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection_name);
    db.setDatabaseName("WhatsNew.db");
    if(!db.open())
    {
        qDebug() << "Error opening database";
        return;
    }
    qDebug() << "Successfully open database...";
    QByteArray outBlock;
    QDataStream out(&outBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);
    QSqlQuery query(db);
    query.prepare("select ip from User where user_id = ?");
    query.addBindValue(user_from);
    if(!query.exec()) qDebug() << query.lastError().text();
    query.next();
    const QString ip = query.value(0).toString();
    query.prepare("select user_id, nickname, email, gender, region, avatar, isOnline, ip from User where user_id = ?");
    query.addBindValue(user_to);
    if(!query.exec()) qDebug() << query.lastError().text();
    query.next();
    const int Timeout = 5 * 1000;
    QTcpSocket *socket = new QTcpSocket();
    socket->abort();
    socket->connectToHost(ip, client_port);
    if(!socket->waitForConnected(Timeout))
        qDebug() << "Client timeout...";
    else
    {
        qint8 messageType = 7;
        qint64 messageSize = sizeof(query.value(0).toInt()) + sizeof(query.value(1).toString()) +
                             sizeof(query.value(2).toString()) + sizeof(query.value(3).toInt()) +
                             sizeof(query.value(4).toString()) + sizeof(query.value(5).toString()) +
                             sizeof(query.value(6).toInt()) + sizeof(query.value(7).toString());
        out << messageType << messageSize
            << query.value(0).toInt() << query.value(1).toString() << query.value(2).toString()
            << query.value(3).toInt() << query.value(4).toString() << query.value(5).toString()
            << query.value(6).toInt() << query.value(7).toString();
        socket->write(outBlock);
        socket->disconnectFromHost();
    }
    db.close();
}
