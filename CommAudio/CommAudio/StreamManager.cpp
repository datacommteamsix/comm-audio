#include <StreamManager.h>

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

void StreamManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections[address] = socket;
	connect(socket, &QTcpSocket::readyRead, this, &StreamManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &StreamManager::disconnectHandler);
}

void StreamManager::disconnectHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (address == mSongSource)
	{
		mMediaPlayer->Stop();
	}

	mConnections.take(address)->deleteLater();
}

void StreamManager::StreamSong(QString songName, quint32 address)
{
	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &StreamManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &StreamManager::disconnectHandler);

	socket->connectToHost(QHostAddress(address), STREAM_PORT);
	mConnections[address] = socket;
	mSongSource = address;

	QByteArray request = QByteArray(1, (char)Headers::RequestAudioStream);
	request.append(*mKey);
	request.append(songName);
	request.resize(1 + KEY_SIZE + SONGNAME_SIZE);

	socket->write(request);

	qDebug() << "Requseting stream";
}

void StreamManager::incomingDataHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (address == mSongSource)
	{
		if (mMediaPlayer->State() == MediaPlayer::StoppedState)
		{
			qDebug() << "Starting stream";
			mMediaPlayer->StartStream(socket);
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