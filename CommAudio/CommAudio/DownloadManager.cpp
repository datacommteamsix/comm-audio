#include <DownloadManager.h>

DownloadManager::DownloadManager(QDir * source, QDir * downloads, QWidget * parent)
	: QWidget(parent)
	, mSource(source)
	, mDownloads(downloads)
	, mServer(this)
{
	connect(&mServer, QTcpServer::newConnection, this, &DownloadManager::newConnectionHandler);
	mServer.listen(QHostAddress::AnyIPv4, 42071);
}

void DownloadManager::DownloadFile(QString songName, quint32 address)
{
	// Send download request here
	// open qfile here for downloaded contents
	// save it in map with address as key
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
	QByteArray data = socket->readAll();

	switch (data[0])
	{
	case (char)Headers::RequestDownload:
		uploadSong(data.mid(1), socket);
		break;
	case (char)Headers::RespondDownload:
		writeToFile(data.mid(1), address);
		break;
	}
}

void DownloadManager::uploadSong(QByteArray data, QTcpSocket * socket)
{
	// Open the file

	// in a while loop
	// send chunks for the file

	// close the file
	// close the socket
}

void DownloadManager::writeToFile(QByteArray data, quint32 address)
{
	// write to the file that has the address as key	
}

void DownloadManager::disconnectHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	// remove socket from the map
	mConnections.take(address)->deleteLater();

	// Remove file from the map and close it
	mFiles.take(address).close();
}