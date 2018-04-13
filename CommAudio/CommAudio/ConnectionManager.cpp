#include "ConnectionManager.h"

ConnectionManager::ConnectionManager(QString * name, QWidget * parent)
	: QWidget(parent)
	, mName(name)
	, mIsHost(false)
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

void ConnectionManager::Init(QMap<QString, QTcpSocket *> * connectedClients, QMap<quint32, QString> * ipToHostname)
{
	mConnectedClients = connectedClients;
	mIptoHost = ipToHostname;
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

void ConnectionManager::AddPendingConnection(const quint32 address, QTcpSocket * socket)
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
	QByteArray packet = QByteArray(1, (char)Headers::RespondToJoin);

	// Add the session key to the packet
	packet.append(mKey);

	// Add the number of clients to the packet
	quint32 size = mConnectedClients->size() - 1;
	packet.append(size);

	// Assert the packet header is the correct length
	assert(packet.size() == 1 + 32 + 1);

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
			qDebug() << "Sending" << connection->peerAddress().toString() << "would be redudntant so skipping";
		}
	}

	socket->write(packet);
}

void ConnectionManager::sendName(QTcpSocket * socket)
{
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::RespondWithName);
	packet.append(*mName);
	packet.resize(1 + 33);

	// Send
	socket->write(packet);
}

void ConnectionManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	connect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);

	//Can either handle it as an error or as a disconnect signal, will test both

	//connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
		//[=](QAbstractSocket::SocketError socketError) { qDebug() << "host has disconnected"; });

	connect(socket, &QTcpSocket::disconnected, this, &ConnectionManager::remoteDisconnectHandler);

	mPendingConnections[address] = socket;
}

//This is for the disconnect between a client requesting to leave session and the server
void ConnectionManager::remoteDisconnectHandler()
{
	//Get the socket that sent the signal
	QTcpSocket * sender = (QTcpSocket *)QObject::sender();

	//Delete from connection map
	QHostAddress address = sender->peerAddress();
	//check values if it contains sender
	qDebug() << address;
	//Delete from user list
}

void ConnectionManager::incomingDataHandler()
{
	quint32 sender = ((QTcpSocket *)QObject::sender())->peerAddress().toIPv4Address();
	QTcpSocket * socket = mPendingConnections.take(sender);
	assert(socket != nullptr);

	QByteArray data = socket->readAll();

	// Check if incoming data is a valid request to join packet
	if (data.size() != 1 + 33)
	{
		qDebug() << "Expecting request to join packet of length" << 1 + 33 << "but packet of size" << data.size() << "was received";
		return;
	}

	switch (data[0])
	{
	case (char)Headers::RequestToJoin:
		parseJoinRequest(data, socket);
		break;
	default:
		qDebug() << "Invalid header received";
		break;
	}
	
}

void ConnectionManager::errorHandler()
{
	qDebug() << "It should come here";
}

void ConnectionManager::parseJoinRequest(const QByteArray data, QTcpSocket * socket)
{
	// Grab the name of the client
	bool isAlreadyConnected = false;
	QString clientName = QString(data.mid(1));
	quint32 pendingAddress = socket->peerAddress().toIPv4Address();

	mIptoHost->insert(pendingAddress, clientName);

	for (QTcpSocket * s : *mConnectedClients)
	{
		if (s->peerAddress().toIPv4Address() == pendingAddress)
		{
			qDebug() << clientName << "-" << "is already connected";
			socket->close();
			socket->deleteLater();
			isAlreadyConnected = true;
			break;
		}
	}

	if (!isAlreadyConnected)
	{
		emit connectionAccepted(clientName, socket);

		if (mIsHost)
		{
			// If this is a new client
			sendListOfClients(socket);
		}
		else
		{
			sendName(socket);
		}
	}

	// Remove it from list of pending connection and stop listening for its request to connect packet
	mPendingConnections.remove(pendingAddress);
	disconnect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
}