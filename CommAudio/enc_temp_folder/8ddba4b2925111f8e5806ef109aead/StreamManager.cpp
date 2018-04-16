#include <StreamManager.h>

StreamManager::StreamManager(MediaPlayer * mediaplayer, const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent)
	: QWidget(parent)
	, mKey(key)
	, mSource(source)
	, mDownloads(downloads)
	, mServer(this)
	, mMediaPlayer(mediaplayer)
{
	QAudioFormat format;
	// Set up the format, eg.
	format.setSampleRate(44100);
	format.setChannelCount(16);
	format.setSampleSize(2);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::UnSignedInt);
	mplayer = new QAudioOutput(format, this);

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

	mConnections.take(address)->deleteLater();

	if (mFiles.contains(address))
	{
		mFiles[address]->close();
		delete mFiles.take(address);
	}
}

void StreamManager::StreamSong(QString songName, quint32 address)
{
	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &StreamManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &StreamManager::disconnectHandler);

	socket->connectToHost(QHostAddress(address), STREAM_PORT);
	mConnections[address] = socket;

	mFiles[address] = new QFile(mDownloads->absoluteFilePath(songName));
	mFiles[address]->open(QFile::WriteOnly);

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

	if (mFiles.contains(address))
	{
		playSong(socket->readAll(), address);
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

void StreamManager::playSong(QByteArray data, quint32 address)
{
	mTimers[address]->stop();
	mTimers[address]->start(DOWNLOAD_TIMEOUT);

	QBuffer *buffer = new QBuffer(&data);
	buffer->open(QIODevice::ReadOnly);
	qDebug() << data.size();
	mplayer->start(buffer);
}

void StreamManager::timeoutHandler()
{
	SocketTimer * expiredTimer = (SocketTimer *)QObject::sender();
	quint32 address = expiredTimer->address;

	mTimers.take(address)->deleteLater();

	mConnections.take(address)->close();
}

