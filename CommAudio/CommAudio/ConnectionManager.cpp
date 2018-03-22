#include "ConnectionManager.h"

ConnectionManager::~ConnectionManager()
{
	for (auto it : mConnections.toStdMap())
	{
		it.second->close();
	}
}

bool ConnectionManager::AddConnection(QString hostname, short port)
{
	QTcpSocket * socket = new QTcpSocket();
	socket->connectToHost(hostname, port, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);

	if (!socket->isOpen())
	{
		return false;
	}

	mConnections[hostname] = socket;
	return true;
}

bool ConnectionManager::RemoveConnection(QString hostname)
{
	mConnections[hostname]->close();
	return !mConnections[hostname]->isOpen();
}

QTcpSocket * ConnectionManager::GetSocket(QString hostname)
{
	return mConnections[hostname];
}

QTcpSocket ** ConnectionManager::GetAllSockets()
{
	QTcpSocket ** tmp = new QTcpSocket*[mConnections.size()];
	int i = 0;

	for (auto it : mConnections.toStdMap())
	{
		tmp[i] = it.second;
		i++;
	}

	return tmp;
}
