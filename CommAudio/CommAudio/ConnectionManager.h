#pragma once

#include <QIODevice>
#include <QMap>
#include <QString>
#include <QTcpSocket>

using namespace std;

class ConnectionManager
{
public:
	ConnectionManager() = default;
	virtual ~ConnectionManager();

	bool AddConnection(QString hostname, short port);
	bool RemoveConnection(QString hostname);

	QTcpSocket * GetSocket(QString hostname);
	QTcpSocket ** GetAllSockets();

private:
	QMap<QString, QTcpSocket *> mConnections;
};
