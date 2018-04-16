/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		DownloadManager.cpp - Handles all the downloading for the client.
--
-- PROGRAM:			CommAudio
--
-- FUNCTIONS:
--					DownloadManager(const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent = nullptr);
--					~DownloadManager() = default;
--					void uploadSong(QByteArray data, QTcpSocket * socket)
--					void writeToFile(QByteArray data, quint32 address)
--					void newConnectionHandler()
--					void incomingDataHandler()
--					void disconnectHandler();
--					void timeoutHandler()
--					void DownloadFile(QString songName, quint32 address)
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
--					Angus Lam
--					Roger Zhang
--
-- NOTES:
--					This class encapsulates all the downloading logic and functionality.
----------------------------------------------------------------------------------------------------------------------*/
#include <DownloadManager.h>

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		DownloadManager
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		DownloadManager (const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent)
--						const QByteArray * key: A reference to the sesion key.
--						QDir * source: A reference to the location of local songs.
--						QDir * downloads: A reference to the download location.
--						QWidget * parent: The parent widget.
--
-- NOTES:
--					Creates a download manager and starts listening on the port reserved for donwloading files.
----------------------------------------------------------------------------------------------------------------------*/
DownloadManager::DownloadManager(const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent)
	: QWidget(parent)
	, mKey(key)
	, mSource(source)
	, mDownloads(downloads)
	, mServer(this)
{
	connect(&mServer, &QTcpServer::newConnection, this, &DownloadManager::newConnectionHandler);
	mServer.listen(QHostAddress::AnyIPv4, DOWNLOAD_PORT);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		DownloadFile
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		DownloadFile (QString songName, quint32 address)
--						QString songName: The name of the song.
--						quin32 address: The address that has the song.
--
-- NOTES:
--					Creates a connection to address and makes a request for songName to be sent over.
----------------------------------------------------------------------------------------------------------------------*/
void DownloadManager::DownloadFile(QString songName, quint32 address)
{
	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &DownloadManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &DownloadManager::disconnectHandler);

	socket->connectToHost(QHostAddress(address), DOWNLOAD_PORT);
	mConnections[address] = socket;

	mFiles[address] = new QFile(mDownloads->absoluteFilePath(songName));
	mFiles[address]->open(QFile::WriteOnly);

	QByteArray request = QByteArray(1, (char)Headers::RequestDownload);
	request.append(*mKey);
	request.append(songName);
	request.resize(1 + KEY_SIZE + SONGNAME_SIZE);

	socket->write(request);

	SocketTimer * timer = new SocketTimer(this);
	connect(timer, &QTimer::timeout, this, &DownloadManager::timeoutHandler);

	timer->address = address;
	timer->start(DOWNLOAD_TIMEOUT);

	mTimers[address] = timer;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		newConnectionHandler
--
-- DATE:			March 26, 2018
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
-- NOTES:
--					This is a Qt slot that is triggered when the QTcpServer has a new connection that is ready to be used.
--					The connect is connected to the appropriate slots and is saved in the map of connections.
----------------------------------------------------------------------------------------------------------------------*/
void DownloadManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections[address] = socket;

	connect(socket, &QTcpSocket::readyRead, this, &DownloadManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &DownloadManager::disconnectHandler);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		incomingDataHandler
--
-- DATE:			March 26, 2018
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
-- NOTES:
--					This is the Qt slot that is triggered when the socket has new data. This function will check where
--					the data is coming from. If the data is coming from an ongoing connection, the data is written to
--					the corrisponding file. Otherwise, if the data is coming from a new connection, the packet is
--					validated according to the download protocol and a function is called to upload the song.
----------------------------------------------------------------------------------------------------------------------*/
void DownloadManager::incomingDataHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (mFiles.contains(address))
	{
		writeToFile(socket->readAll(), address);
	}
	else
	{
		QByteArray data = socket->read(1 + KEY_SIZE + SONGNAME_SIZE);
		if (data[0] == (char)Headers::RequestDownload)
		{
			uploadSong(data.mid(1), socket);
		}
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		uploadSong
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		uploadSong (QByteArray data, QTcpSocket * socket)
--						QByteArray data: The request packet without the header.
--						QTcpSocket * socket: The socket that sent the request.
--
-- NOTES:
--					Extracts the song name from the requests and sends it to the socket over tcp.
----------------------------------------------------------------------------------------------------------------------*/
void DownloadManager::uploadSong(QByteArray data, QTcpSocket * socket)
{
	quint32 address = socket->peerAddress().toIPv4Address();

	// TODO: Check session key

	QFile file(mSource->absoluteFilePath(data.mid(KEY_SIZE)));
	file.open(QFile::ReadOnly);

	while (!file.atEnd())
	{
		QByteArray packet = QByteArray(file.read(DOWNLOAD_CHUNCK_SIZE));
		socket->write(packet);
	}

	file.close();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		writeToFile
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		uploadSong (QByteArray data, QTcpSocket * socket)
--						QByteArray data: The incoming song data.
--						quint32 address: The address of the sender.
--
-- NOTES:
--					Writes the data into the file that has the key address. The timer for the socket is also reset so
--					that the socket does not get closed.
----------------------------------------------------------------------------------------------------------------------*/
void DownloadManager::writeToFile(QByteArray data, quint32 address)
{
	mTimers[address]->stop();
	mTimers[address]->start(DOWNLOAD_TIMEOUT);

	mFiles[address]->write(data);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		disconnectHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		disconnectHandler ()
--
-- NOTES:
--					This is a Qt slot that is triggered when a socket is disconnected or closed. The sockt is removed 
--					from the map of sockets and the corrisponding file is closed and removed from the map of files.
----------------------------------------------------------------------------------------------------------------------*/
void DownloadManager::disconnectHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections.take(address)->deleteLater();

	if (mFiles.contains(address))
	{
		mFiles[address]->close();
		delete mFiles.take(address);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		timeoutHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		timeoutHandler ()
--
-- NOTES:
--					This is a Qt slot that is triggerd when the timer for a socket has timed out. When a sockett times out
--					the socket is closed and the corrisponding timer is removed from the map of timers.
----------------------------------------------------------------------------------------------------------------------*/
void DownloadManager::timeoutHandler()
{
	SocketTimer * expiredTimer = (SocketTimer *)QObject::sender();
	quint32 address = expiredTimer->address;

	mTimers.take(address)->deleteLater();

	mConnections[address]->close();
}