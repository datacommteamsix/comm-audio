/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		ConnectionManager.cpp - This is a manager for new connections.
--
-- PROGRAM:			CommAudio
--
-- FUNCTIONS:
--					ConnectionManager(QByteArray * key, QString * name, QWidget * parent = nullptr)
--					~ConnectionManager()
--					void Init(QMap<QString, QTcpSocket *> * connectedClients)
--					void BecomeHost()
--					void BecomeClient()
--					void AddPendingConnection(const quint32 address, QTcpSocket * socket)
--					void startServerListen()
--					void sendListOfClients(QTcpSocket * socket)
--					void sendName(QTcpSocket * socket)
--					void parseJoinRequest(const QByteArray data, QTcpSocket * socket)
--					void newConnectionHandler()
--					void incomingDataHandler()
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- NOTES:
--					This class encapsulates all the logic for accepting and validating new connections to the session.
----------------------------------------------------------------------------------------------------------------------*/
#include "ConnectionManager.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		ConnecitonManager
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		ConnectionManager (QByteArray * key, QString * name, QWidget * parent)
--						QByteArray * key: A reference to the session key.
--						QString * name: A reference to the name of the client.
--						QWidget * parent: A reference to the QWidget parent.
--
-- RETURNS:			N/A
--
-- NOTES:
--					The contstructor for the ConnectionManager. 
----------------------------------------------------------------------------------------------------------------------*/
ConnectionManager::ConnectionManager(QByteArray * key, QString * name, QWidget * parent)
	: QWidget(parent)
	, mName(name)
	, mIsHost(false)
	, mServer(this)
	, mKey(key)
{
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		~ConnecitonManager
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		~ConnectionManager ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					The deconstructor for ConnectionManager. THis is where all the remaining connections are closed.
----------------------------------------------------------------------------------------------------------------------*/
ConnectionManager::~ConnectionManager()
{
	for (QTcpSocket * socket : mPendingConnections)
	{
		socket->close();
		delete socket;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		~ConnecitonManager
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		~ConnectionManager ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					The deconstructor for ConnectionManager. THis is where all the remaining connections are closed.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::Init(QMap<QString, QTcpSocket *> * connectedClients)
{
	mConnectedClients = connectedClients;
	startServerListen();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		BecomeHost
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		BecomeHost ()
--
-- RETURNS:			void.
--
-- NOTES:
--					Sets the ConnectionManager to host mode.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::BecomeHost()
{
	mIsHost = true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		BecomeClient
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		BecomeClient ()
--
-- RETURNS:			void.
--
-- NOTES:
--					Sets the ConnectionManager to client mode.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::BecomeClient()
{
	mIsHost = false;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		AddPendingConnection
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		AddPendingConnection (const quint32 address, QTcpSocket * socket)
--						const quint32 address: The address of the pending conneciton.
--						QTcpSocket * socket: The socket of the pending connection.
--
-- RETURNS:			void.
--
-- NOTES:
--					Saves the new pending connection to the map of pending connections.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::AddPendingConnection(const quint32 address, QTcpSocket * socket)
{
	mPendingConnections[address] = socket;
	connect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		startServerListen
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		startServerListen ()
--
-- RETURNS:			void.
--
-- NOTES:
--					Starts the server by entering the TCP listen state.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::startServerListen()
{
	connect(&mServer, &QTcpServer::newConnection, this, &ConnectionManager::newConnectionHandler);
	mServer.listen(QHostAddress::Any, 42069);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		sendListofClients
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		sendListOfClients (QTcpSocket * socket)
--						QTcpSocket * socket: The socket to send the list of clients.
--
-- RETURNS:			void.
--
-- NOTES:
--					Creates a packet containin the name of this host as well as a list of Ip addresses of all connected
--					clients in the sesison to the socket.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::sendListOfClients(QTcpSocket * socket)
{
	QByteArray packet = QByteArray(1, (char)Headers::RespondToJoin);

	// Add the session key to the packet
	packet.append(*mKey);

	// Add name of host
	QByteArray name = QByteArray(USER_NAME_SIZE, 0);
	name.replace(0, mName->size(), mName->toStdString().c_str());
	packet.append(name);

	// Add the number of clients to the packet
	quint32 size = mConnectedClients->size() - 1;
	packet.append(size);

	// Send list of currently connected clients to the new client
	for (QTcpSocket * connection : *mConnectedClients)
	{
		// Do not send the new client it's own ip
		if (connection->peerAddress() != socket->peerAddress())
		{
			packet << connection->peerAddress().toIPv4Address();
		}
	}

	socket->write(packet);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		sendName
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		sendName (QTcpSocket * socket)
--						QTcpSocket * socket: The socket to send the list of clients.
--
-- RETURNS:			void.
--
-- NOTES:
--					Sends the name of this socket to the socket.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::sendName(QTcpSocket * socket)
{
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::RespondWithName);
	packet.append(*mName);
	packet.resize(1 + 33);

	// Send
	socket->write(packet);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		newConnectionHandler
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		newConnectionHandler ()
--
-- RETURNS:			void.
--
-- NOTES:
--					This is a Qt slot that is triggered when the TCP server acecpts a new conneciton. That connection
--					is added to a map of pending connections.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	connect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
	mPendingConnections[address] = socket;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		incomingDataHandler
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		incomingDataHandler ()
--
-- RETURNS:			void.
--
-- NOTES:
--					This is a Qt slot that is triggered when there is new data on a socket. The data is read and if it
--					is a request to join the request is then parsed.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::incomingDataHandler()
{
	quint32 sender = ((QTcpSocket *)QObject::sender())->peerAddress().toIPv4Address();
	QTcpSocket * socket = mPendingConnections.take(sender);

	QByteArray data = socket->readAll();

	// Check if incoming data is a valid request to join packet
	if (data.size() != 1 + USER_NAME_SIZE)
	{
		return;
	}

	switch (data[0])
	{
	case (char)Headers::RequestToJoin:
		parseJoinRequest(data, socket);
		break;
	default:
		break;
	}
	
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		parseJoinRequest
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		parseJoinRequset (const QByteArray data, QTcpSocket * socket)
--						const QByteArray data: The data to be parsed.
--						QTcpSocket * socket: The socket that the data was read from.
--
-- RETURNS:			void.
--
-- NOTES:
--					The request to join is parsed. First if the socket is already connected the request is ignored.
--					Also, if there are already 10 people in this session (including the host) the connection will
--					be ignored. Otherwise if the ConnectionManager is in host mode, the new connection saved and a list
--					of currently connected clients is sent in response. If the ConnectionManager is in client mode, 
--					the key of the incoming request is compared to the one that is stored in memory, if the keys match
--					then the client responds with its name.
----------------------------------------------------------------------------------------------------------------------*/
void ConnectionManager::parseJoinRequest(const QByteArray data, QTcpSocket * socket)
{
	// Grab the name of the client
	bool isAlreadyConnected = false;
	QString clientName = QString(data.mid(1));
	quint32 pendingAddress = socket->peerAddress().toIPv4Address();

	if (mConnectedClients->size() > 9)
	{
		return;
	}

	for (QTcpSocket * s : *mConnectedClients)
	{
		if (s->peerAddress().toIPv4Address() == pendingAddress)
		{
			socket->close();
			socket->deleteLater();
			isAlreadyConnected = true;
			break;
		}
	}

	if (!isAlreadyConnected)
	{
		if (mIsHost)
		{
			emit connectionAccepted(clientName, socket);
			sendListOfClients(socket);
		}
		else
		{
			QByteArray incomingKey = data.mid(1, KEY_SIZE);

			if (incomingKey == *mKey)
			{
				emit connectionAccepted(clientName, socket);
				sendName(socket);
			}
		}
	}

	// Remove it from list of pending connection and stop listening for its request to connect packet
	mPendingConnections.remove(pendingAddress);
	disconnect(socket, &QTcpSocket::readyRead, this, &ConnectionManager::incomingDataHandler);
}