#include <DownloadManager.h>

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

void DownloadManager::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections[address] = socket;

	connect(socket, &QTcpSocket::readyRead, this, &DownloadManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &DownloadManager::disconnectHandler);
}

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

void DownloadManager::uploadSong(QByteArray data, QTcpSocket * socket)
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

void DownloadManager::writeToFile(QByteArray data, quint32 address)
{
	mTimers[address]->stop();
	mTimers[address]->start(DOWNLOAD_TIMEOUT);

	mFiles[address]->write(data);
}

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

void DownloadManager::timeoutHandler()
{
	SocketTimer * expiredTimer = (SocketTimer *)QObject::sender();
	quint32 address = expiredTimer->address;

	mTimers.take(address)->deleteLater();

	mConnections.take(address)->close();
}