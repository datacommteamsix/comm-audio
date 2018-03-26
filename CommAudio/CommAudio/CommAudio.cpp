#include "CommAudio.h"

CommAudio::CommAudio(QWidget *parent)
	: QMainWindow(parent)
	, mLocalIp(getLocalIp())
	, mSessionKey()
	, mServer(this)
	, mConnectins()
{
	ui.setupUi(this);
	
	// Setting default folder to home/comm-audio
	QDir tmp = QDir(QDir::homePath() + "/comm-audio");
	mSharedFolder = tmp;
	mDownloadFolder = tmp;

	// Closing the application
	connect(ui.actionExit, &QAction::triggered, this, &QWidget::close);

	// Host, Join, Leave session buttons
	connect(ui.actionHostSession, &QAction::triggered, this, &CommAudio::hostSessionHandler);
	connect(ui.actionJoinSession, &QAction::triggered, this, &CommAudio::joinSessionHandler);
	connect(ui.actionLeaveSession, &QAction::triggered, this, &CommAudio::leaveSessionHandler);

	// Changing targeted folders
	connect(ui.actionPublicSongFolder, &QAction::triggered, this, &CommAudio::changeSongFolderHandler);
	connect(ui.actionDownloadFolder, &QAction::triggered, this, &CommAudio::changeDownloadFolderHandler);

	// Audio Control Buttons
	connect(ui.btnPlaySong, &QPushButton::pressed, this, &CommAudio::playSongButtonHandler);
	connect(ui.btnPrevSong, &QPushButton::pressed, this, &CommAudio::prevSongButtonHandler);
	connect(ui.btnNextSong, &QPushButton::pressed, this, &CommAudio::nextSongButtonHandler);

	// Networking set up
	connect(&mServer, &QTcpServer::newConnection, this, &CommAudio::newConnectionHandler);
	mServer.listen(QHostAddress::Any, 42069);
}

QString CommAudio::getLocalIp()
{
	QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

	for (auto address : addresses)
	{
		if (address.protocol() == QTcpSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
		{
			return address.toString();
		}
	}

	return QString();
}

void CommAudio::hostSessionHandler()
{

}

void CommAudio::joinSessionHandler()
{

}

void CommAudio::leaveSessionHandler()
{

}

void CommAudio::changeSongFolderHandler()
{

}

void CommAudio::changeDownloadFolderHandler()
{

}

void CommAudio::playSongButtonHandler()
{

}

void CommAudio::prevSongButtonHandler()
{

}

void CommAudio::nextSongButtonHandler()
{

}

void CommAudio::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	QString hostname = socket->peerName();

	if (mConnectins.contains(hostname))
	{
		qDebug() << "Connection to this host already exists";
		delete socket;
		return;
	}

	connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);
	mConnectins[hostname] = socket;
	qDebug() << hostname << "was added to connections";
}

void CommAudio::incomingDataHandler()
{
	QTcpSocket * sender = (QTcpSocket *)QObject::sender();
	QByteArray data = sender->readAll();
	qDebug() << data;
	mConnectins[sender->peerName()]->write(data);
}