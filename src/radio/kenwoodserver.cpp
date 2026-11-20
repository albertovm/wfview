// Copyright 2017-2025 Elliott H. Liggett W6EL and Phil E. Taylor M0VSE

#include "kenwoodserver.h"
#include "logcategories.h"
#include "icomudpbase.h"
#include <QUuid>

kenwoodServer::kenwoodServer(SERVERCONFIG* config, rigServer* parent) :
    rigServer(config,parent)
{
    qInfo(logRigServer()) << "Creating instance of Kenwood server";
}

void kenwoodServer::init()
{
    connect(queue,SIGNAL(rigCapsUpdated(rigCapabilities*)),this, SLOT(receiveRigCaps(rigCapabilities*)));

    if (config->httpEnabled) {
        httpServer = new QTcpServer(this);
        if (httpServer->listen(QHostAddress::Any, config->httpPort)) {
            connect(httpServer, &QTcpServer::newConnection, this, [this]() {
                while (httpServer->hasPendingConnections()) {
                    QTcpSocket *socket = httpServer->nextPendingConnection();
                    connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                        const QByteArray request = socket->readAll();
                        const QList<QByteArray> lines = request.split('\n');
                        if (lines.isEmpty()) { socket->disconnectFromHost(); return; }
                        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
                        const QByteArray path = requestLine.size() > 1 ? requestLine[1] : QByteArray("/");
                        QByteArray body;
                        QByteArray status("200 OK");
                        QByteArray type("text/html; charset=utf-8");
                        const QByteArray cookie = request.toLower().contains("cookie: wfview_session=")
                            ? request.mid(request.toLower().indexOf("cookie: wfview_session=") + 23).split('\n').first().trimmed()
                            : QByteArray();
                        bool authenticated = httpSessions.contains(QString::fromLatin1(cookie));
                        if (path.startsWith("/login") && requestLine.size() > 0 && requestLine[0] == "POST") {
                            const int separator = request.indexOf("\r\n\r\n");
                            const QByteArray form = separator >= 0 ? request.mid(separator + 4) : QByteArray();
                            auto value = [&form](const QByteArray &key) {
                                const QByteArray marker = key + '=';
                                int at = form.indexOf(marker);
                                if (at < 0) return QByteArray();
                                QByteArray v = form.mid(at + marker.size()).split('&').first().replace('+', ' ');
                                return QByteArray::fromPercentEncoding(v);
                            };
                            const QString username = QString::fromUtf8(value("username"));
                            QByteArray pass;
                            passcode(QString::fromUtf8(value("password")), pass);
                            bool valid = false;
                            bool txAllowed = false;
                            for (const SERVERUSER &user : config->users)
                                if (user.username == username && user.password == pass) {
                                    valid = true;
                                    txAllowed = user.userType != 2;
                                    break;
                                }
                            if (valid) {
                                const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
                                httpSessions.insert(token);
                                httpSessionTx.insert(token, txAllowed);
                                socket->write("HTTP/1.1 303 See Other\r\nLocation: /\r\nSet-Cookie: wfview_session=" + token.toLatin1() + "; HttpOnly; SameSite=Strict; Path=/\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
                                socket->disconnectFromHost();
                                return;
                            }
                            status = "401 Unauthorized";
                        }
                        if (path == "/logout") {
                            httpSessions.remove(QString::fromLatin1(cookie));
                            httpSessionTx.remove(QString::fromLatin1(cookie));
                            socket->write("HTTP/1.1 303 See Other\r\nLocation: /\r\nSet-Cookie: wfview_session=; Max-Age=0; HttpOnly; Path=/\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
                            socket->disconnectFromHost();
                            return;
                        }
                        if (!authenticated) {
                            body = "<!doctype html><meta name='viewport' content='width=device-width'><title>wfview login</title><style>body{font:16px system-ui;background:#111827;color:#eee;display:grid;place-items:center;height:90vh}form{padding:2rem;background:#1f2937;border-radius:12px}input,button{display:block;margin:12px 0;padding:12px;width:240px}</style><form method='post' action='/login'><h1>wfview</h1><input name='username' placeholder='Username' required><input name='password' type='password' placeholder='Password' required><button>Sign in</button>" + (status.startsWith("401") ? QByteArray("<p>Invalid username or password</p>") : QByteArray()) + "</form>";
                        } else {
                            const bool txAllowed = httpSessionTx.value(QString::fromLatin1(cookie), false);
                            body = "<!doctype html><meta name='viewport' content='width=device-width'><title>wfview remote</title><style>body{font:16px system-ui;background:#111827;color:#eee;margin:2rem}main{max-width:800px;margin:auto;background:#1f2937;padding:2rem;border-radius:14px}a{color:#67e8f9}</style><main><h1>wfview Remote</h1><p>Authenticated with Radio Server Users.</p><p>" + (txAllowed ? QByteArray("Transmit permission enabled.") : QByteArray("Receive only account.")) + "</p><a href='/logout'>Sign out</a></main>";
                        }
                        const QByteArray response = "HTTP/1.1 " + status + "\r\nContent-Type: " + type + "\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
                        socket->write(response);
                        socket->disconnectFromHost();
                    });
                }
            });
            qInfo(logRigServer()) << "HTTP remote access listening on port" << config->httpPort;
        } else {
            qWarning(logRigServer()) << "Could not start HTTP remote access on port" << config->httpPort;
        }
    }

    server = new QTcpServer(this);

    if (config->httpAccessOnly)
        return;

    if (!server->listen(QHostAddress::Any, config->controlPort)) {
        qInfo(logRigServer()) << "could not start on port " << config->controlPort;
        return;
    }
    else
    {
        qInfo(logRigServer()) << "started on port " << config->controlPort;
    }
    connect(server, &QTcpServer::newConnection, this, &kenwoodServer::newConnection);
}

kenwoodServer::~kenwoodServer()
{
    // We should probably step through all connected clients (just in case)

    qInfo(logRigServer()) << "Stopping audio streaming";
    foreach (RIGCONFIG* radio, config->rigs)
    {
        if (radio->rtpThread != Q_NULLPTR) {
            radio->rtpThread->quit();
            radio->rtpThread->wait();
            radio->rtp = Q_NULLPTR;
            radio->rtpThread = Q_NULLPTR;
        }
    }

    /*
    for (auto it = clients.begin(); it != clients.end(); it++)
    {
        it.key()->close();
        delete it.key();
        delete it.value();
    }

    if (server != Q_NULLPTR)
    {
        server->close();
    }

    */

}

void kenwoodServer::dataForServer(QByteArray d)
{
    kenwoodCommander* sender = qobject_cast<kenwoodCommander*>(QObject::sender());

    if (sender == Q_NULLPTR)
    {
        qInfo(logRigServer()) << "Object type didn't match!";
        //return;
    }

    //qInfo(logRigServer()) << "Radio reply:" << d;

    for (auto it = clients.begin(); it != clients.end(); it++)
    {
        QTcpSocket* socket = it.key();
        CLIENT* client = it.value();
        if (socket != Q_NULLPTR && client != Q_NULLPTR && socket->isOpen() && client->connected && client->authenticated)
        {
            socket->write(d);
        }
    }

    return;
}


void kenwoodServer::receiveAudioData(const audioPacket& d)
{
    rigCommander* sender = qobject_cast<rigCommander*>(QObject::sender());
    quint8 guid[GUIDLEN];
    if (sender != Q_NULLPTR)
    {
        memcpy(guid, sender->getGUID(), GUIDLEN);
    }
    else {
        memcpy(guid, d.guid, GUIDLEN);
    }


    return;
}

void kenwoodServer::newConnection()
{
    // A new client has connected!
    CLIENT* current = new CLIENT();
    QTcpSocket* socket = server->nextPendingConnection();
    connect(socket, &QTcpSocket::disconnected, socket, [this,socket] { disconnected(socket);});
    connect(socket, &QTcpSocket::readyRead, socket, [this,socket] { readyRead(socket);});

    current->ipAddress = socket->peerAddress();

    clients.insert(socket,current);
    qInfo(logRigServer()) << "Client connected!" << socket->peerAddress() << ":" << socket->peerPort();
}

void kenwoodServer::disconnected(QTcpSocket* socket)
{
    // Client has disconnected!
    qInfo(logRigServer()) << "Client disconnected!" << socket->peerAddress() << ":" << socket->peerPort();
    if (socket->isOpen())
        socket->close();
    clients.remove(socket);
    foreach (RIGCONFIG* radio, config->rigs)
    {
        if (radio->rtpThread != Q_NULLPTR) {
            radio->rtpThread->quit();
            radio->rtpThread->wait();
            radio->rtp = Q_NULLPTR;
            radio->rtpThread = Q_NULLPTR;
        }
    }
}

void kenwoodServer::readyRead(QTcpSocket* socket)
{
    // Data from client
    auto it = clients.find(socket);
    if (it == clients.end())
    {
        qWarning(logRigServer()) << "Client not found, this shouldn't happen!";
        disconnected(socket);
        return;
    }

    CLIENT *c = it.value();
    c->buffer.append(socket->readAll());

    c->lastHeard = QDateTime::currentDateTime();

    if (c->buffer.endsWith(";") || c->buffer.endsWith((";\n")))
    {
        QList<QByteArray> commands = c->buffer.split(';');
        for (auto &d: commands) {
            if (d.isEmpty() || d == QString("\n").toLatin1())
                continue;
            // We have at least one full command, so process it
            if (!c->connected)
            {
                if (d == ("##CN")) {
                    socket->write(QString("##CN1;").toLatin1());
                    c->connected = true;
                    continue;
                }
                else
                {
                    disconnected(socket);
                    return;
                }
            } else if (!c->authenticated) {
                if (d.startsWith("##ID")) {
                    // This is an authentication request
                    uchar type = d.mid(4,1).toShort();
                    uchar userlen = d.mid(5,2).toUShort();
                    uchar passlen = d.mid(7,2).toUShort();
                    QString username = d.mid(9,userlen);
                    QString password = d.mid(9+userlen,passlen);
                    QByteArray passcomp;
                    passcode(password, passcomp);
                    qInfo(logRigServer()) << c->ipAddress.toString() << ": Received 'login'";
                    foreach(SERVERUSER user, config->users)
                    {
                        if (user.username == username && user.password == passcomp)
                        {
                            c->authenticated = true;
                            c->user = username;
                            // Usertypes 0=admin, 1=user, 2=rx only.
                            if (user.userType == 0 && type == 0)
                            {
                                c->admin = true;
                                c->tx = true;
                            } else if (user.userType == 1 && type == 1)
                            {
                                c->admin=false;
                                c->tx=true;
                            }
                            else if (user.userType == 2 && type == 1)
                            {
                                c->admin=false;
                                c->tx=false;
                            } else
                            {
                                qInfo(logRigServer()) << "User type did not match stored type";
                                c->authenticated = false;
                                disconnected(socket);
                                return;
                            }
                            qInfo(logRigServer()) << "User" << username << "logged-in successfully";
                            socket->write(QString("##ID1;##UE1;##TI%0;").arg(uchar(c->tx)).toLatin1());
                            break;
                        }
                    }
                    if (c->authenticated != true)
                    {
                        qInfo(logRigServer()) << "User" << username << "invalid password";
                        socket->write(QString("##ID0").toLatin1());
                        disconnected(socket);
                        return;
                    } else {
                        continue;
                    }
                } else {
                    qInfo(logRigServer()) << "Invalid command received" << d;
                    disconnected(socket);
                    return;
                }
            }
            // If we got here then user is both connected and authenticated.
            if (d.startsWith("##VP")) {
                //User is requesting voip (audio) control
                uchar vp = uchar(d.mid(4,1).toUShort());
                foreach (RIGCONFIG* radio, config->rigs)
                {
                    if ((!memcmp(radio->guid, c->guid, GUIDLEN) || config->rigs.size()==1) && radio->rtp == Q_NULLPTR && !config->lan)
                    {
                        if (!c->isStreaming && vp)
                        {
                            if (vp==1) {
                                qInfo(logRigServer()) << "Starting high quality audio streaming";
                                radio->txAudioSetup.codec = 4; //c->txCodec;
                                radio->txAudioSetup.sampleRate= 16000; //c->txSampleRate;
                                radio->rxAudioSetup.codec = 4; //c->rxCodec;
                                radio->rxAudioSetup.sampleRate = 16000; //c->rxSampleRate;
                            } else if (vp==2){
                                qInfo(logRigServer()) << "Starting low quality audio streaming";
                                radio->txAudioSetup.codec = 128; //c->txCodec;
                                radio->txAudioSetup.sampleRate= 16000; //c->txSampleRate;
                                radio->rxAudioSetup.codec = 128; //c->rxCodec;
                                radio->rxAudioSetup.sampleRate = 16000; //c->rxSampleRate;
                            } else if (vp==3){
                                qInfo(logRigServer()) << "Starting Opus audio streaming";
                                radio->txAudioSetup.codec = 64; //c->txCodec;
                                radio->txAudioSetup.sampleRate= 16000; //c->txSampleRate;
                                radio->rxAudioSetup.codec = 64; //c->rxCodec;
                                radio->rxAudioSetup.sampleRate = 16000; //c->rxSampleRate;
                            }
                            // This is the first client, so stop running the queue.
                            radio->queueInterval = queue->interval();
                            if (config->disableUI)
                            {
                                queue->interval(-1);
                            }

                            radio->txAudioSetup.isinput = false;
                            radio->txAudioSetup.latency = 150; //c->txBufferLen;

                            radio->rxAudioSetup.latency = 150; //c->txBufferLen;
                            radio->rxAudioSetup.isinput = true;
                            memcpy(radio->rxAudioSetup.guid, radio->guid, GUIDLEN);

                            outAudio.isinput = false;
                            radio->rtp = new rtpAudio(c->ipAddress.toString(),config->audioPort,radio->txAudioSetup, radio->rxAudioSetup);
                            radio->rtpThread = new QThread(this);
                            radio->rtpThread->setObjectName("RTP()");
                            radio->rtp->moveToThread(radio->rtpThread);
                            connect(this, SIGNAL(initRtpAudio()), radio->rtp, SLOT(init()));
                            connect(radio->rtpThread, SIGNAL(finished()), radio->rtp, SLOT(deleteLater()));
                            //connect(this, SIGNAL(haveChangeLatency(quint16)), radio->rtp, SLOT(changeLatency(quint16)));
                            //connect(this, SIGNAL(haveSetVolume(quint8)), radio->rtp, SLOT(setVolume(quint8)));
                            // Audio from UDP
                            //connect(radio->rtp, SIGNAL(haveAudioData(audioPacket)), this, SLOT(receiveAudioData(audioPacket)));
                            //QObject::connect(radio->rtp, SIGNAL(haveOutLevels(quint16, quint16, quint16, quint16, bool, bool)), this, SLOT(getOutLevels(quint16, quint16, quint16, quint16, bool, bool)));
                            //QObject::connect(radio->rtp, SIGNAL(haveInLevels(quint16, quint16, quint16, quint16, bool, bool)), this, SLOT(getInLevels(quint16, quint16, quint16, quint16, bool, bool)));
                            radio->rtpThread->start(QThread::TimeCriticalPriority);
                            emit initRtpAudio();
                            c->isStreaming = true;
                        }
                        else
                        {
                            qInfo(logRigServer()) << "Stopping audio streaming";
                            if (radio->rtpThread != Q_NULLPTR) {
                                radio->rtpThread->quit();
                                radio->rtpThread->wait();
                                radio->rtp = Q_NULLPTR;
                                radio->rtpThread = Q_NULLPTR;
                            }
                            c->isStreaming=false;
                        }
                        socket->write(QString("##VP%0;").arg(uchar(vp)).toLatin1());

                    }
                }
            } else {
                // I think all other commands need to be forwarded to the radio?
                // We have removed the semicolon, so add it back.
                d.append(QString(";").toLatin1());
                //qInfo(logRigServer()) << "Sending:" << d;
                emit haveDataFromServer(d);
            }
        }
        // We have processed this buffer, so lets get rid of it.
        c->buffer.clear();
    }


}
