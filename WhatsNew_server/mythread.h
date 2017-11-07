#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDataStream>
#include <QMessageBox>

extern const quint16 client_port;

class MyThread : public QThread
{
    Q_OBJECT

public:
    explicit MyThread(qintptr ID, QObject *parent = 0);
    void run();

signals:
    void error(QAbstractSocket::SocketError);
    void dbChanged();
    void sendRequest(int, int, const QString, const QString);
    void requestAgreed(int, int);

public slots:
    void readData(); //读取接受到的数据
    void disconnected();
    void displayError(QAbstractSocket::SocketError);

private:
    QTcpSocket *socket;
    qintptr socketDescriptor;
    QSqlDatabase db;
    qint8 messageType; //数据类型号
    qint64 totalBytes; //数据总大小

    qint64 fileNameSize; //文件名大小
    QString fileName; //文件名
    QFile *localFile; //文件
    QByteArray inBlock; //数据缓冲区

    bool signup(const QString&, const QString&, const  QString&, QString&); //处理注册业务
    bool login(const QString&, const QString&, const QString&, int&, QString&); //处理登录业务
    bool receiveMessage(int, int, const QString&, const QString&); //接收消息
    void receiveFile(); //接收文件
    void retrievePasswd(); //找回密码
    void logout(int); //用户下线
};

#endif // MYTHREAD_H
