#include "ConnectionManager.h"

ConnectionManager::ConnectionManager(QWidget * parent)
	: mIsHost(false)
	, mServer(this)
{
}

ConnectionManager::~ConnectionManager()
{
	for (QTcpSocket * socket : mPendingConnections)
	{
		socket->close();
		delete socket;
	}
}

void ConnectionManager::Init(QMap<QString, QTcpSocket *> * connectedClients)
{
	mConnectedClients = connectedClients;
	startServerListen();
}

void ConnectionManager::BecomeHost(QByteArray key)
{
	mIsHost = true;
	mKey = key;
}

void ConnectionManager::BecomeClient()
{
	mIsHost = false;
	mKey = QByteArray();
}

void ConnectionManager::AddPendingConnection(const QString address, QTcpSocket * socket)
{
	assert(socket != nullptr);
	mPendingConnections[address] = socket;
	connect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
}

void ConnectionManager::startServerListen()
{
	connect(&mServer, &QTcpServer::newConnection, this, &ConnectionManager::newConnectionHandler);
	mServer.listen(QHostAddress::Any, 42069);
}

void ConnectionManager::sendListOfClients(QTcpSocket * socket)
{
	QByteArray packet = QByteArray(1 + 32 + 1, 0);
	packet[0] = (char)Headers::RespondToJoin;

	// Add the session key to the packet
	packet.replace(1, 32, mKey);

	// Add the number of clients to the packet
	int size = mConnectedClients->size() - 1;
	packet.replace(1 + 32, 1, QByteArray::number(size));

	// Assert the packet header is the correct length
	assert(packet.size() == 1 + 32 + 1);

	// TODO: Figure out how to put ip address into qbytearray

	// Send list of currently connected clients to the new client
	for (QTcpSocket * connection : *mConnectedClients)
	{
		// Do not send the new client it's own ip
		if (connection->peerAddress() != socket->peerAddress())
		{
			packet << connection->peerAddress().toIPv4Address();
		}
		else
		{
			qDebug() << "Sending" << connection->peerAddress().toString() << "would be redudntant ... skipping";
		}
	}

	socket->write(packet);
}

void ConnectionManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	QString address = socket->peerAddress().toString();

	connect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
	mPendingConnections[address] = socket;
}

void ConnectionManager::incomingDataHandler()
{
	QString sender = ((QTcpSocket *)QObject::sender())->peerAddress().toString();
	QTcpSocket * socket = mPendingConnections.take(sender);
	QByteArray data = socket->readAll();

	// Check if incoming data is a valid request to join packet
	if (data.size() != 1 + 33)
	{
		qDebug() << "Expecting request to join packet of length" << 1 + 33 << "but packet of size" << data.size() << "was received";
		return;
	}

	if (data[0] != (char)Headers::RequestToJoin)
	{
		qDebug() << "Header" << (char)data[0] << "does not match" << (char)Headers::RequestToJoin;
	}

	// Grab the name of the client
	QString clientName = QString(data.mid(1));

	// Reject it if the client is already connected
	if (mConnectedClients->contains(clientName))
	{
		qDebug() << clientName << "-" << "is already connected";
		socket->close();
		socket->deleteLater();
	}
	else
	{
		emit connectionAccepted(clientName, socket);

		if (mIsHost)
		{
			// If this is a new client
			sendListOfClients(socket);
		}

		// Remove it from list of pending connection and stop listening for its request to connect packet
		mPendingConnections.remove(socket->peerAddress().toString());
		disconnect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
	}
}