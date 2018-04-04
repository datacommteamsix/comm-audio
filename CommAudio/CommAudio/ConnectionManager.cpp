#include "ConnectionManager.h"

ConnectionManager::ConnectionManager(QWidget * parent)
	: mIsHost(false)
	, mServer(this)
{
	startServerListen();
}

ConnectionManager::~ConnectionManager()
{
	for (QTcpSocket * socket : mPendingConnections)
	{
		socket->close();
		delete socket;
	}
}

void ConnectionManager::BecomeHost(QMap<QString, QTcpSocket *> * connectedClients, QByteArray key)
{
	mIsHost = true;
	mConnectedClients = connectedClients;
	mKey = key;
}

void ConnectionManager::BecomeClient()
{
	mIsHost = false;
	mConnectedClients = nullptr;
	mKey = QByteArray();
}

void ConnectionManager::startServerListen()
{
	connect(&mServer, &QTcpServer::newConnection, this, &ConnectionManager::newConnectionHandler);
	mServer.listen(QHostAddress::Any, 42069);
}

void ConnectionManager::sendListOfClients(QTcpSocket * socket)
{
	QByteArray packet = QByteArray(1, (char)Headers::RespondToJoin);

	// Add the session key to the packet
	packet += mKey;

	// Add the number of clients to the packet
	int size = mConnectedClients->size();
	char bytes[sizeof(size)];
	memcpy(bytes, &size, sizeof(size));
	packet += bytes;

	// Assert the packet header is the correct length
	assert(packet.size() == 1 + 32 + 4);

	// Send list of currently connected clients to the new client
	for (QTcpSocket * socket : *mConnectedClients)
	{
		packet += socket->peerAddress().toString();
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
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	QByteArray data = socket->readAll();

	// Check if incoming data is a valid request to join packet
	if (data.size() != 33)
	{
		qDebug() << "Expecting request to join packet of length" << 33 << "but packet of size" << data.size() << "was received";
		return;
	}

	if (data[0] != (char)Headers::RequestToJoin)
	{
		qDebug() << "Header" << (char)data[0] << "does not match" << (char)Headers::RequestToJoin;
	}

	// Accept client
	QString clientName = QString(data.mid(1));
	emit connectionAccepted(clientName, socket);

	if (mIsHost)
	{
		// Respond according to protocol
		sendListOfClients(socket);
	}

	// Remove it from list of pending connection and stop listening for its request to connect packet
	mPendingConnections.remove(socket->peerAddress().toString());
	disconnect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
}