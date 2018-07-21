#include "mythread.h"

MyThread::MyThread(qintptr ID, QObject *parent)
    : QThread(parent)
{
    socketDescriptor = ID;
    db = QSqlDatabase::addDatabase("QSQLITE", QString(socketDescriptor));
    db.setDatabaseName("WhatsNew.db");
}

void MyThread::run()
{
    qDebug() << "Thread started...";
    socket = new QTcpSocket();
    //set the ID
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    if(!socket->setSocketDescriptor(socketDescriptor))
    {
        emit error(socket->error());
        return;
    }
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    qDebug() << socketDescriptor << " Client connected...";
    exec();
}

void MyThread::disconnected()
{
    if(db.isOpen()) db.close();
    qDebug() << socketDescriptor << " Disconnected...";
    socket->deleteLater();
    exit(0);
}

void MyThread::readData() //读取数据
{
    qDebug() << "Start receiving data...";
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_9);
    in.startTransaction();
    if(!in.commitTransaction())
        return;
    in >> messageType >> totalBytes;
    qDebug() << messageType << totalBytes;
    qint8 rtype, response;
    if(messageType == 0) //接收到注册数据
    {
        qDebug() << "Receiving signup data...";
        QString nickname, password, email, error;
        in >> nickname >> password >> email;
        qDebug() << nickname << password << email;
        rtype = 0;
        if(signup(nickname, password, email, error)) response = 1;
        else response = 0;
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        out << rtype << response;
        socket->write(outBlock);
        socket->disconnectFromHost();
        return;
    }
    if(messageType == 1) //接收到登录数据
    {
        qDebug() << "Receiving login data...";
        QString email, password, client_ip, error;
        int user_id;
        in >> email >> password >> client_ip;
        qDebug() << email << password << client_ip;
        rtype = 1;
        if(login(email, password, client_ip, user_id, error)) response = 1;
        else response = 0;
        qint64 messageSize = sizeof(response)+sizeof(error);
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        out << rtype << messageSize << response << user_id << error;
        socket->write(outBlock);
        socket->disconnectFromHost();
        return;
    }
    if(messageType == 2) //接收到离线消息
    {
        qDebug() << "Receiving offline message...";
        int user_from, user_to;
        QString content, time, error;
        in >> user_from >> user_to >> content >> time;
        qDebug() << user_from << user_to << content << time;
        rtype = 2;
        if(receiveMessage(user_from, user_to, content, time))
        {
            response = 1;
            error = "OK";
        }
        else
        {
            response = 0;
            error = "Server error. Please resend later.";
        }
        qint64 messageSize = sizeof(response)+sizeof(error);
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        out << rtype << messageSize << response << error;
        socket->write(outBlock);
        socket->disconnectFromHost();
        emit dbChanged();
        return;
    }
    if(messageType == 3) //接收到离线文件
    {
        qDebug() << "Receiving offline file...";

    }
    if(messageType == 4) //用户下线
    {
        qDebug() << "Receiving client offline signal...";
        int user_id;
        in >> user_id;
        logout(user_id);
        qint8 rtype = 4;
        qint64 response = 1;
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        out << rtype << response;
        socket->write(outBlock);
        socket->disconnectFromHost();
        return;
    }
    if(messageType == 5) //客户端请求好友推荐列表
    {
        qint8 rtype = 5;
        qint64 response;
        int user_id;
        in >> user_id;
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        QList<int>  friends;
        friends.append(user_id);
        if(!db.open())
        {
            QMessageBox::critical(0, "Cannot open database",
                                  "Unable to establish database connection.", QMessageBox::Cancel);
            qDebug() << "Cannot open database. Unable to establish database connection.";
            response = 0;
            out << rtype << response;
            socket->write(outBlock);
            socket->disconnectFromHost();
            return;
        }
        qDebug() << "Sucessfully open database...";
        response = 1;
        out << rtype << response;

        QSqlQuery query(db);
        query.prepare("select user1 from FriendsList where user2 = ?");
        query.addBindValue(user_id);
        if(!query.exec()) qDebug() << query.lastError().text();
        while(query.next())
            friends.append(query.value(0).toInt());
        query.prepare("select user2 from FriendsList where user1 = ?");
        query.addBindValue(user_id);
        if(!query.exec()) qDebug() << query.lastError().text();
        while(query.next())
            friends.append(query.value(0).toInt());
        query.exec("select count(*) from User");
        query.next();
        int total = query.value(0).toInt();
        int num = total - friends.size();
        out << num;
        query.exec("select nickname, user_id from User");
        while(query.next())
        {
            bool isFriend = false;
            for(int i=0; i<friends.size(); i++)
                if(query.value(1).toInt() == friends[i])
                {
                    isFriend = true;
                    break;
                }
            if(!isFriend)
                out << query.value(0).toString() << QString() << query.value(1).toInt();
        }
        socket->write(outBlock);
        socket->disconnectFromHost();
        return;
    }
    if(messageType == 6) //用户添加好友
    {
        qDebug() << "Receiving friend adding request";
        qint8 rtype = 6;
        qint64 response = 0;
        int from, to;
        QString requestTime, reason;
        in >> from >> to >> requestTime >> reason;
        qDebug() << from << to << reason;
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        if(!db.open())
        {
            qDebug() << "Error opening database";
            response = 0;
            out << rtype << response;
            socket->write(outBlock);
            socket->disconnectFromHost();
            return;
        }
        qDebug() << "Successfully open database...";
        QSqlQuery query(db);
        query.prepare("select nickname from User where user_id = ?");
        query.addBindValue(from);
        if(!query.exec()) qDebug() << query.lastError().text();
        query.next();
        QString nickname = query.value(0).toString();
        query.exec("select count(*) from addList");
        query.next();
        int numRows = query.value(0).toInt();
        query.prepare("select count(*) from addList where user_from = ? and user_to = ?");
        query.addBindValue(from);
        query.addBindValue(to);
        if(!query.exec()) qDebug() << query.lastError().text();
        query.next();
        if(query.value(0).toInt() == 0)
        {
            query.prepare("insert into addList values (?, ?, ?, ?, ?)");
            query.addBindValue(numRows+1);
            query.addBindValue(from);
            query.addBindValue(to);
            query.addBindValue(reason);
            query.addBindValue(requestTime);
            if(!query.exec()) qDebug() << query.lastError().text();
            emit sendRequest(from, to, nickname, reason);
        }
        response = 1;
        out << rtype << response;
        socket->write(outBlock);
        socket->disconnectFromHost();
        db.commit();
        db.close();
        return;
    }
    if(messageType == 7) //用户同意好友申请
    {
        qDebug() << "Receiving request agreement";
        qint8 rtype = 7;
        qint64 response = 0;
        int user_from, user_to;
        in >> user_from >> user_to;
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        if(!db.open())
        {
            qDebug() << "Error opening database";
            return;
        }
        qDebug() << "Successfully open database...";
        QSqlQuery query(db);
        int user1, user2;
        if(user_from < user_to)
        {
            user1 = user_from;
            user2 = user_to;
        }
        else
        {
            user1 = user_to;
            user2 = user_from;
        }
        query.exec("select count(*) from FriendsList");
        query.next();
        int numRows = query.value(0).toInt();
        query.prepare("insert into FriendsList (id, user1, user2) values (?, ?, ?)");
        query.addBindValue(numRows+1);
        query.addBindValue(user1);
        query.addBindValue(user2);
        if(!query.exec()) qDebug() << query.lastError().text();
        query.prepare("delete from addList where user_from = ? and user_to = ?");
        query.addBindValue(user1);
        query.addBindValue(user2);
        if(!query.exec()) qDebug() << query.lastError().text();
        query.prepare("delete from addList where user_from = ? and user_to = ?");
        query.addBindValue(user2);
        query.addBindValue(user1);
        if(!query.exec()) qDebug() << query.lastError().text();
        query.prepare("select user_id, nickname, email, gender, region, avatar, isOnline, ip from User where user_id = ?");
        query.addBindValue(user_from);
        if(!query.exec()) qDebug() << query.lastError().text();
        query.next();
        response = 1;
        out << rtype << response;
        out << query.value(0).toInt() << query.value(1).toString() << query.value(2).toString()
            << query.value(3).toInt() << query.value(4).toString() << query.value(5).toString()
            << query.value(6).toInt() << query.value(7).toString();
        socket->write(outBlock);
        socket->disconnectFromHost();
        emit dbChanged();
        emit requestAgreed(user_from, user_to);
        return;
    }
    if(messageType == 10) //客户端请求好友列表
    {
        qDebug() << "Receiving FriendsList request...";
        qint8 rtype = 10;
        qint64 response;
        int user_id;
        in >> user_id;
        QByteArray outBlock;
        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        QList<int>  friends;
        QList<QString> lastChats;
        friends.append(user_id);
        lastChats.append(QString());
        if(!db.open())
        {
            QMessageBox::critical(0, "Cannot open database",
                                  "Unable to establish database connection.", QMessageBox::Cancel);
            qDebug() << "Cannot open database. Unable to establish database connection.";
            response = 0;
            out << rtype << response;
            socket->write(outBlock);
            socket->disconnectFromHost();
            return;
        }
        qDebug() << "Sucessfully open database...";
        response = 1;
        out << rtype << response;

        QSqlQuery query(db);
        qint8 subtype;

        in >> subtype;
        if(subtype == 0)
        {
            query.prepare("select user1, lastChat from FriendsList where user2 = ?");
            query.addBindValue(user_id);
            if(!query.exec()) qDebug() << query.lastError().text();
            while(query.next())
            {
                friends.append(query.value(0).toInt());
                lastChats.append(query.value(1).toString());
            }
            query.prepare("select user2, lastChat from FriendsList where user1 = ?");
            query.addBindValue(user_id);
            if(!query.exec()) qDebug() << query.lastError().text();
            while(query.next())
            {
                friends.append(query.value(0).toInt());
                lastChats.append(query.value(1).toString());
            }
            for(int i=0; i<friends.size(); i++)
            {
                qDebug() << friends[i];
                query.prepare("select user_id, nickname, email, gender, region, avatar, isOnline, ip from User where user_id = ?");
                query.addBindValue(friends[i]);
                if(!query.exec()) qDebug() << query.lastError().text();
                query.next();
                out << query.value(0).toInt() << query.value(1).toString() << query.value(2).toString()
                    << query.value(3).toInt() << query.value(4).toString() << query.value(5).toString()
                    << query.value(6).toInt() << query.value(7).toString();
                out << lastChats[i];
            }
            out << -1;

            query.prepare("select file_id, user_from, user_to, file_name, file_size, file, time from File where user_from = ? or user_to = ?");
            query.addBindValue(user_id);
            query.addBindValue(user_id);
            if(!query.exec()) qDebug() << query.lastError().text();
            while(query.next())
                out << query.value(0).toInt() << query.value(1).toInt() << query.value(2).toInt()
                    << query.value(3).toString() << query.value(4).toInt() << query.value(5).toString()
                    << query.value(6).toString();
            out << -1;
        }

        query.exec(QString("select * from Message where user_to = %1").arg(user_id));
        qDebug() << QString("select * from Message where user_to = %1").arg(user_id);
        while(query.next())
        {
            qDebug() << query.value(0).toInt() << query.value(1).toInt() << query.value(2).toInt()
                << query.value(3).toString() << query.value(4).toString();
            out << query.value(0).toInt() << query.value(1).toInt() << query.value(2).toInt()
                << query.value(3).toString() << query.value(4).toString();
        }
        query.prepare("delete from Message where user_to = ?");
        query.addBindValue(user_id);
        if(!query.exec()) qDebug() << query.lastError().text();
        out << -1;

        query.prepare("select user_from, reason from addList where user_to = ?");
        query.addBindValue(user_id);
        if(!query.exec()) qDebug() << query.lastError().text();
        QList<int> adders;
        QList<QString> reasons;
        while(query.next())
        {
            adders.append(query.value(0).toInt());
            reasons.append(query.value(1).toString());
        }
        for(int i=0; i<adders.size(); i++)
        {
            query.prepare("select nickname from User where user_id = ?");
            query.addBindValue(adders[i]);
            if(!query.exec()) qDebug() << query.lastError().text();
            query.next();
            out << adders[i] << query.value(0).toString() << reasons[i];
        }
        out << -1;

        db.commit();
        db.close();
        emit dbChanged();
        qDebug() << "Database connection closed...";
        socket->write(outBlock);
        socket->disconnectFromHost();
        return;
    }
    socket->disconnectFromHost();
}

bool MyThread::signup(const QString &nickname, const QString &password, const QString &email, QString &error) //处理注册业务
{
    qDebug() << "Processing user signup data...";
    if(!db.open())
    {
        QMessageBox::critical(0, "Cannot open database",
                              "Unable to establish database connection.", QMessageBox::Cancel);
        QMessageBox::information(0, "Signup failed",
                                 QString("nickname: ")+nickname+QString("\npassword: ")+password+QString("\nemail: ")+email,
                                 QMessageBox::Accepted);
        error = "Server error. Please signup later.";
        return false;
    }
    qDebug() << "Sucessfully open database...";
    QSqlQuery query(db);
    query.exec("select count(*) from User");
    query.next();
    int numRows = query.value(0).toInt();
    query.prepare("insert into User (user_id, nickname, password, email) values (?, ?, ?, ?)");
    query.addBindValue(numRows+1);
    query.addBindValue(nickname);
    query.addBindValue(password);
    query.addBindValue(email);
    bool b = query.exec();
    if(!b)
    {
        qDebug() << query.lastError().text();
        return false;
    }
    db.commit();
    db.close();
    emit dbChanged();
    qDebug() << "Sucessfully insert new user data into database...";
    return true;
}

bool MyThread::login(const QString &email, const QString &password, const QString &client_ip, int &user_id, QString &error)
{
    qDebug() << "Processing login data...";
    if(!db.open())
    {
        QMessageBox::critical(0, "Cannot open database",
                              "Unable to establish database connection.", QMessageBox::Cancel);
        error = "Server error. Please login later.";
        return false;
    }
    qDebug() << "Sucessfully open database...";
    QSqlQuery query(db);
    query.prepare("select count(*) from User where email = ?");
    query.addBindValue(email);
    if(!query.exec()) qDebug() << query.lastError().text();
    query.next();
    int matched = query.value(0).toInt();
    if(matched == 0)
    {
        error = "Email not found";
        db.close();
        qDebug() << "Database connection closed...";
        return false;
    }
    query.prepare("select count(*), user_id from User where email = ? and password = ?");
    query.addBindValue(email);
    query.addBindValue(password);
    if(!query.exec()) qDebug() << query.lastError().text();
    query.next();
    matched = query.value(0).toInt();
    if(matched == 0)
    {
        error = "Password incorrect";
        db.close();
        qDebug() << "Database connection closed...";
        return false;
    }
    user_id = query.value(1).toInt();
    //update online status
    query.prepare("update User set isOnline = 1, ip = ? where email = ?");
    query.addBindValue(client_ip);
    query.addBindValue(email);
    if(!query.exec()) qDebug() << query.lastError().text();
    db.commit();
    emit dbChanged();
    qDebug() << "User status changed to online...";
    db.close();
    qDebug() << "Database connection closed...";
    return true;
}

bool MyThread::receiveMessage(int user_from, int user_to, const QString &content, const QString &time) //接收离线消息
{
    qDebug() << "Processing offline message...";
    if(!db.open())
    {
        QMessageBox::critical(0, "Cannot open database",
                              "Unable to establish database connection.", QMessageBox::Cancel);
        return false;
    }
    qDebug() << "Sucessfully open database...";
    QSqlQuery query(db);
    query.exec("select count(*) from Message");
    query.next();
    int numRows = query.value(0).toInt();
    query.prepare("insert into Message values (?, ?, ?, ?, ?)");
    query.addBindValue(numRows+1);
    query.addBindValue(user_from);
    query.addBindValue(user_to);
    query.addBindValue(content);
    query.addBindValue(time);
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        db.close();
        qDebug() << "Database connection closed...";
        return false;
    }
    db.commit();    
    qDebug() << "Successfully insert offline message into database...";
    db.close();
    qDebug() << "Database connection closed...";
    return true;
}

void MyThread::receiveFile()
{

}

void MyThread::retrievePasswd()
{

}

void MyThread::logout(int user_id)
{
    qDebug() << "User going to logout...";
    if(!db.open())
    {
        QMessageBox::critical(0, "Cannot open database",
                              "Unable to establish database connection.", QMessageBox::Cancel);
        return;
    }
    qDebug() << "Sucessfully open database...";
    QSqlQuery query(db);
    query.prepare("update User set isOnline = 0, ip = ? where user_id = ?");
    query.addBindValue(QString());
    query.addBindValue(user_id);
    if(!query.exec()) qDebug() << query.lastError().text();
    qDebug() << "User " << user_id << " offline...";
    db.commit();
    emit dbChanged();
    db.close();
    qDebug() << "Database connection closed...";
}

void MyThread::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(0, tr("Fortune Client"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(0, tr("Fortune Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(0, tr("Fortune Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(socket->errorString()));
    }
}
