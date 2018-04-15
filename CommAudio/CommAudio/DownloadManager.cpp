#include <DownloadManager.h>

DownloadManager::DownloadManager(const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent)
	: QWidget(parent)
	, mKey(key)
	, mSource(source)
	, mDownloads(downloads)
	, mServer(this)
	, i(0)
{
	connect(&mServer, &QTcpServer::newConnection, this, &DownloadManager::newConnectionHandler);
	mServer.listen(QHostAddress::AnyIPv4, 42071);
}

void DownloadManager::DownloadFile(QString songName, quint32 address)
{
	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &DownloadManager::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &DownloadManager::disconnectHandler);

	socket->connectToHost(QHostAddress(address), 42071);
	mConnections[address] = socket;

	mFiles[address] = new QFile(mDownloads->absoluteFilePath(songName));
	mFiles[address]->open(QFile::WriteOnly);

	QByteArray request = QByteArray(1, (char)Headers::RequestDownload);
	request.append(*mKey);
	request.append(songName);
	request.resize(1 + 32 + 255);

	socket->write(request);
	i = 0;

	// Start the timer
	SocketTimer * timer = new SocketTimer(this);
	connect(timer, &QTimer::timeout, this, &DownloadManager::timeoutHandler);

	timer->address = address;
	timer->start(5 * 1000);

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
		QByteArray data = socket->read(1 + 32 + 255);
		if (data[0] == (char)Headers::RequestDownload)
		{
			uploadSong(data.mid(1), socket);
		}
	}
}

void DownloadManager::uploadSong(QByteArray data, QTcpSocket * socket)
{
	i = 0;
	quint32 address = socket->peerAddress().toIPv4Address();

	QFile file(mSource->absoluteFilePath(data.mid(32)));
	file.open(QFile::ReadOnly);

	qDebug() << "Starting to write file";
	while (!file.atEnd())
	{
		QByteArray packet = QByteArray(file.read(8192));
		socket->write(packet);
		qDebug() << ++i;
	}
	qDebug() << "Finished writing file";

	file.close();
}

void DownloadManager::writeToFile(QByteArray data, quint32 address)
{
	// Extend timer
	mTimers[address]->stop();
	mTimers[address]->start(5 * 1000);

	mFiles[address]->write(data);
	qDebug() << ++i;
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

	// Clean up timer
	mTimers.take(address)->deleteLater();

	// Close connection
	mConnections.take(address)->close();
}