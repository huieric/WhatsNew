#include "controller.h"

Controller::Controller()
{
    Server *server = new Server;
    server->moveToThread(&serverThread);
    connect(server, SIGNAL(newConnection(QTcpSocket*)),
            this, SLOT(createConnection(QTcpSocket*)));
    connect(this, SIGNAL(server_closed()), server, SLOT(stop()));
    connect(&serverThread, &QThread::finished, server, &QObject::deleteLater);
    connect(&serverThread, &QThread::finished, &serverThread, &QObject::deleteLater);
    serverThread.start();
}

Controller::~Controller()
{
    serverThread.quit();
    serverThread.wait();
    for(auto it=connectionPool.begin(); it<connectionPool.end(); it++)
    {
        (*it)->quit();
        (*it)->wait();
        delete *it;
    }
    connectionPool.clear();
}

void Controller::createConnection(QTcpSocket *tcpConnection)
{
    QThread *thread = new QThread;
    Connection *connection = new Connection(thread, tcpConnection);
    connection->moveToThread(thread);
    connect(connection, SIGNAL(destroyed(QThread*)),
            this, SLOT(removeThread(QThread*)));
    connect(thread, &QThread::finished, connection, &QObject::deleteLater);
    thread->start();
    connectionPool.append(thread);
}

void Controller::removeThread(QThread *thread)
{
    connectionPool.removeOne(thread);
}

void Controller::stopServer()
{
    emit server_closed();
}
