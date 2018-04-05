#pragma once

#include <assert.h>

#include <QDebug>

#include <QByteArray>
#include <QHostAddress>
#include <QMap>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWidget>

#include "Headers.h"

class ConnectionManager : public QWidget
{
	Q_OBJECT

public:
	ConnectionManager(QWidget * parent = nullptr);
	~ConnectionManager();

	void Init(QMap<QString, QTcpSocket *> * connectedClients);
	void BecomeHost(QByteArray key);
	void BecomeClient();

	void AddPendingConnection(const QString address, QTcpSocket * socket);

private:
	bool mIsHost;
	QByteArray mKey;

	QTcpServer mServer;
	QMap<QString, QTcpSocket *> * mConnectedClients;
	QMap<QString, QTcpSocket *> mPendingConnections;

	void startServerListen();
	void sendListOfClients(QTcpSocket * socket);

private slots:
	void newConnectionHandler();
	void incomingDataHandler();

signals:
	void connectionAccepted(QString, QTcpSocket *);
};