/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		StreamManager.cpp - A manager for all the audio streaming of the application.
--
-- PROGRAM:			CommAudio
--
-- FUNCTIONS:
--					StreamManager(const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent = nullptr)
--					~StreamManager()
--					void uploadSong(QByteArray data, QTcpSocket * socket)
--					void newConnectionHandler()
--					void incomingDataHandler()
--					void disconnectHandler()
--					void StreamSong(QString songName, quint32 address)
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
--					This is a class that encapsulates all the audio streaming for the application.
----------------------------------------------------------------------------------------------------------------------*/
#include <StreamManager.h>

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		StreamManager
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--					Benny Wang
--
-- INTERFACE:		StreamManager (const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent)
--						const QByteArray * key: A reference to the session key.
--						QDir * source: A reference to the source directory.
--						QDir * downloads: A reference to the downloads directory.
--						QWdiget * parent: A reference to the QWidget parent.
--
-- RETURNS:			N/A
--
-- NOTES:
--					The contstructor for the StreamManager. This is where the TCP listen call is made for the reserverd
--					streaming port.
----------------------------------------------------------------------------------------------------------------------*/
StreamManager::StreamManager(const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent)
	: QWidget(parent)
	, mKey(key)
	, mSource(source)
	, mDownloads(downloads)
	, mServer(this)
	, mSongSource(0)
{
	connect(&mServer, &QTcpServer::newConnection, this, &StreamManager::newConnectionHandler);
	mServer.listen(QHostAddress::AnyIPv4, STREAM_PORT);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		~StreamManager
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--					Benny Wang
--
-- INTERFACE:		~StreamManager ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					The deconstructor of the StreamManger. This is where all the remain connections are closed.
----------------------------------------------------------------------------------------------------------------------*/
StreamManager::~StreamManager()
{
	QList<quint32> keys = mConnections.keys();

	for (int i = 0; i < keys.size(); i++)
	{
		mConnections[keys[i]]->close();
	}
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
-- PROGRAMMER:		Roger Zhang
--					Benny Wang
--
-- INTERFACE:		newConnectionHandler ()
--
-- RETURNS:			void.
--
-- NOTES:
--					This is a Qt slot that is triggered when a new connection has been accepted. The connection is 
--					stored in a map of connections. In addition the socket is connected with the related qt slots of this
--					class for handling new data and disconnections.
----------------------------------------------------------------------------------------------------------------------*/
void StreamManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections[address] = socket;
	connect(socket, &QTcpSocket::readyRead, this, &StreamManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &StreamManager::disconnectHandler);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		disconnectHandler
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--					Benny Wang
--
-- INTERFACE:		disconnectHandler ()
--
-- RETURNS:			void.
--
-- NOTES:
--					This is a Qt slot that is triggered when a socket has disconnected. The socket is removed from the
--					map of connections along with its associated buffer.
----------------------------------------------------------------------------------------------------------------------*/
void StreamManager::disconnectHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (mSongSource == address)
	{
		mSongSource = 0;
	}

	mConnections.take(address)->deleteLater();
	mBuffers.remove(address);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		StreamSong
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--					Benny Wang
--
-- INTERFACE:		StreamSong (QString songName, quint32 address)
--						QString songName: The name of the song to stream.
--						quint32 address: The address of the person who owns the song.
--
-- RETURNS:			void.
--
-- NOTES:
--					This is a Qt slot that is triggered when the user presses a button to download a new song.
--					If there is already a request to that address in progress this request is ignored. Otherwise. A new
--					request and buffer are created.
----------------------------------------------------------------------------------------------------------------------*/
void StreamManager::StreamSong(QString songName, quint32 address)
{
	if (mConnections.contains(address))
	{
		return;
	}

	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &StreamManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &StreamManager::disconnectHandler);

	socket->connectToHost(QHostAddress(address), STREAM_PORT);
	mConnections[address] = socket;
	mSongSource = address;

	mBuffers[address] = new QBuffer(this);
	mBuffers[address]->open(QIODevice::ReadWrite);

	QByteArray request = QByteArray(1, (char)Headers::RequestAudioStream);
	request.append(*mKey);
	request.append(songName);
	request.resize(1 + KEY_SIZE + SONGNAME_SIZE);

	socket->write(request);

	qDebug() << "Requseting stream";
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
-- PROGRAMMER:		Roger Zhang
--					Benny Wang
--
-- INTERFACE:		incomingDataHandler ()
--
-- RETURNS:			void.
--
-- NOTES:
--					This is a Qt slot that is triggered when there is new data on the port. If the data is coming from
--					the address that we have requested a song to be streamed form, the data is read off the socket and
--					stored in a buffer. If the buffer is not already being played then the buffer is passed to the
--					media player to be played. Otherwise, if a valid stream request is made, a song upload is initiated.
----------------------------------------------------------------------------------------------------------------------*/
void StreamManager::incomingDataHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (address == mSongSource)
	{
		mBuffers[address]->buffer().append(socket->readAll());
		if (mMediaPlayer->State() == MediaPlayer::StoppedState)
		{
			qDebug() << "Starting stream";
			mMediaPlayer->StartStream(mBuffers[address]);
		}
	}
	else
	{
		QByteArray data = socket->readAll();
		if (data[0] == (char)Headers::RequestAudioStream)
		{
			uploadSong(data.mid(1), socket);
		}
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		uploadSOng
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--					Benny Wang
--
-- INTERFACE:		uploadSong (QByteArray data, QTcpSocket * socket)
--						QByteArray data: The data of the incoming packet.
--						QTcpSocket * socket: The socket of the sender.
--
-- RETURNS:			void.
--
-- NOTES:
--					The file name for the song is read here and the the is openned and written to the socket that
--					request the song in chunks.
----------------------------------------------------------------------------------------------------------------------*/
void StreamManager::uploadSong(QByteArray data, QTcpSocket * socket)
{
	QFile file(mSource->absoluteFilePath(data.mid(KEY_SIZE)));
	file.open(QFile::ReadOnly);

	while (!file.atEnd())
	{
		QByteArray packet = QByteArray(file.read(DOWNLOAD_CHUNCK_SIZE));
		socket->write(packet);
		socket->flush();
	}

	file.close();
}