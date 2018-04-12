#pragma once

#include <assert.h>

#include <QDebug>

#include <QByteArray>
#include <QDataStream>
#include <QHostAddress>
#include <QMap>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWidget>

#include "globals.h"

class ConnectionManager : public QWidget
{
	Q_OBJECT

public:
	ConnectionManager(QString * name, QWidget * parent = nullptr);
	~ConnectionManager();

	void Init(QMap<QString, QTcpSocket *> * connectedClients, QMap<quint32, QString> * ipToHostname);
	void BecomeHost(QByteArray key);
	void BecomeClient();

	void AddPendingConnection(const quint32 address, QTcpSocket * socket);

private:
	bool mIsHost;
	QString * mName;
	QByteArray mKey;

	QTcpServer mServer;
	QMap<QString, QTcpSocket *> * mConnectedClients;
	QMap<quint32, QTcpSocket *> mPendingConnections;
	QMap<quint32, QString> * mIptoHost;

	void startServerListen();

	void sendListOfClients(QTcpSocket * socket);
	void sendName(QTcpSocket * socket);
	void parseJoinRequest(const QByteArray data, QTcpSocket * socket);

private slots:
	void remoteDisconnectHandler();
	void newConnectionHandler();
	void incomingDataHandler();
	void errorHandler();

signals:
	void connectionAccepted(QString, QTcpSocket *);
};