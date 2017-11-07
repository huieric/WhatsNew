#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QMessageBox>
#include <QDebug>
#include "mythread.h"

extern const quint16 server_port;
extern const quint16 client_port;

class Server : public QTcpServer
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = 0);
    void startServer();

signals:
    void dbChanged();

public slots:
    //管理员拥有停止服务器的权限
    //void stop();
    void emit_dbChanged();
    void sendRequest(int, int, const QString, const QString);
    void notice(int, int);

protected:
    void incomingConnection(qintptr socketDescriptor);

private:

};

#endif // SERVER_H
