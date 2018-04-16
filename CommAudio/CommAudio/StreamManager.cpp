#include <StreamManager.h>

StreamManager::StreamManager(MediaPlayer * mediaplayer, const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent)
	: QWidget(parent)
	, mKey(key)
	, mSource(source)
	, mDownloads(downloads)
	, mServer(this)
	, mMediaPlayer(mediaplayer)
{
	// Set up the format, eg.
	mFormat.setSampleRate(44100);
	mFormat.setSampleSize(16);
	mFormat.setChannelCount(2);
	mFormat.setCodec("audio/pcm");
	mFormat.setByteOrder(QAudioFormat::LittleEndian);
	mFormat.setSampleType(QAudioFormat::SignedInt);

	connect(&mServer, &QTcpServer::newConnection, this, &StreamManager::newConnectionHandler);
	mServer.listen(QHostAddress::AnyIPv4, STREAM_PORT);
}

void StreamManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections[address] = socket;
	mOutputs[address] = new QAudioOutput(mFormat, this);
	connect(socket, &QTcpSocket::readyRead, this, &StreamManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &StreamManager::disconnectHandler);
}

void StreamManager::disconnectHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections.take(address)->deleteLater();

	if (mOutputs.contains(address))
	{
		mOutputs.clear();
	}
}

void StreamManager::StreamSong(QString songName, quint32 address)
{
	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &StreamManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &StreamManager::disconnectHandler);

	socket->connectToHost(QHostAddress(address), STREAM_PORT);
	mConnections[address] = socket;

	QByteArray request = QByteArray(1, (char)Headers::RequestAudioStream);
	request.append(*mKey);
	request.append(songName);
	request.resize(1 + KEY_SIZE + SONGNAME_SIZE);

	socket->write(request);

	SocketTimer * timer = new SocketTimer(this);
	connect(timer, &QTimer::timeout, this, &StreamManager::timeoutHandler);

	timer->address = address;
	timer->start(DOWNLOAD_TIMEOUT);

	mTimers[address] = timer;
}

void StreamManager::incomingDataHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (mOutputs.contains(address))
	{
		mOutputs[address]->start(mConnections[address]);
	}
	else
	{
		QByteArray data = socket->read(1 + KEY_SIZE + SONGNAME_SIZE);
		if (data[0] == (char)Headers::RequestAudioStream)
		{
			uploadSong(data.mid(1), socket);
		}
	}
}

void StreamManager::uploadSong(QByteArray data, QTcpSocket * socket)
{
	quint32 address = socket->peerAddress().toIPv4Address();

	QFile file(mSource->absoluteFilePath(data.mid(KEY_SIZE)));
	file.open(QFile::ReadOnly);

	while (!file.atEnd())
	{
		QByteArray packet = QByteArray(file.read(DOWNLOAD_CHUNCK_SIZE));
		socket->write(packet);
	}

	file.close();
}

void StreamManager::timeoutHandler()
{
	SocketTimer * expiredTimer = (SocketTimer *)QObject::sender();
	quint32 address = expiredTimer->address;

	mTimers.take(address)->deleteLater();

	mConnections.take(address)->close();
}

