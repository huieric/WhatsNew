#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QThread>
#include <QList>
#include "server.h"
#include "connection.h"

class Controller : public QObject
{
    Q_OBJECT
public:
    Controller();
    ~Controller();

signals:
    void server_closed();

public slots:
    void createConnection(QTcpSocket *);
    void removeThread(QThread *);
    void stopServer();

private:
    QThread serverThread;
    QList<QThread *> connectionPool;
};

#endif // CONTROLLER_H
