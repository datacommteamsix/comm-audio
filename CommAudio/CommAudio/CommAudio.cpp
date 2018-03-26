#include "CommAudio.h"

CommAudio::CommAudio(QWidget * parent)
	: QMainWindow(parent)
	, mIsHost(false)
	, mName(QHostInfo::localHostName())
	, mSessionKey()
	, mPlayer(new QMediaPlayer())
	, mServer(this)
	, mConnections()
	, mPendingConnections()
{
	ui.setupUi(this);
	
	// Setting default folder to home/comm-audio
	QDir tmp = QDir(QDir::homePath() + "/comm-audio");
	mSongFolder = tmp;
	mDownloadFolder = tmp;

	// Configure the media player
	mPlayer->setVolume(100);
	// Set volume
	connect(ui.sliderVolume, &QSlider::sliderMoved, mPlayer, &QMediaPlayer::setVolume);
	// Song progress
	connect(mPlayer, &QMediaPlayer::positionChanged, this, &CommAudio::songProgressHandler);

	// Closing the application
	connect(ui.actionExit, &QAction::triggered, this, &QWidget::close);

	// Changing name
	connect(ui.actionSetName, &QAction::triggered, this, &CommAudio::changeNameHandler);

	// Host, Join, Leave session buttons
	connect(ui.actionHostSession, &QAction::triggered, this, &CommAudio::hostSessionHandler);
	connect(ui.actionJoinSession, &QAction::triggered, this, &CommAudio::joinSessionHandler);
	connect(ui.actionLeaveSession, &QAction::triggered, this, &CommAudio::leaveSessionHandler);

	// Changing targeted folders
	connect(ui.actionPublicSongFolder, &QAction::triggered, this, &CommAudio::changeSongFolderHandler);
	connect(ui.actionDownloadFolder, &QAction::triggered, this, &CommAudio::changeDownloadFolderHandler);

	// Populate local song list
	populateLocalSongsList();

	// Audio Control Buttons
	connect(ui.btnPlaySong, &QPushButton::pressed, this, &CommAudio::playSongButtonHandler);
	connect(ui.btnPrevSong, &QPushButton::pressed, this, &CommAudio::prevSongButtonHandler);
	connect(ui.btnNextSong, &QPushButton::pressed, this, &CommAudio::nextSongButtonHandler);

	// Networking set up
	connect(&mServer, &QTcpServer::newConnection, this, &CommAudio::newConnectionHandler);
	mServer.listen(QHostAddress::Any, 42069);
}

CommAudio::~CommAudio()
{
	delete mPlayer;
}

void CommAudio::populateLocalSongsList()
{
	QStringList songs = mSongFolder.entryList({ "*.wav", "*.mp3" , "*.m4a" }, 
		QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

	// Create a list of widgets
	QList<QTreeWidgetItem *> items;
	for (QString song : songs)
	{
		items.append(new QTreeWidgetItem(ui.treeLocalSongs, QStringList(song)));
	}

	// Add the list of widgets to tree
	ui.treeLocalSongs->insertTopLevelItems(0, items);
}

void CommAudio::hostSessionHandler()
{
	// Generate session key
	QCryptographicHash hasher(QCryptographicHash::Sha3_256);

	for (int i = 0; i < qrand() % 10; i++)
	{
		switch (qrand() % 4)
		{
		case 0:
			hasher.addData(QHostInfo::localHostName().toUtf8());
			break;
		case 1:
			hasher.addData(QString::number(pos().x()).toUtf8());
			break;
		case 2:
			hasher.addData(QString::number(pos().y()).toUtf8());
			break;
		case 3:
		default:
			break;
		}
	}
	mSessionKey = hasher.result();

	// Set host mode to true
	mIsHost = true;
}

void CommAudio::joinSessionHandler()
{
	mIsHost = false;
	mSessionKey = QByteArray();

	// Send Request to join session here
}

void CommAudio::leaveSessionHandler()
{
	mIsHost = false;
	mSessionKey = QByteArray();

	// Send notice of leave to all connected members

	// Disconnect from all memebers
}

void CommAudio::changeNameHandler()
{
	mName = QInputDialog::getText(this, tr("Enter new name"), "Name: ", QLineEdit::Normal);
}

void CommAudio::changeSongFolderHandler()
{
	QString dir = QFileDialog::getExistingDirectory(this, 
		tr("Select Song Folder"), QDir::homePath(), 
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	mSongFolder = QDir(dir);

	populateLocalSongsList();
}

void CommAudio::changeDownloadFolderHandler()
{
	QString dir = QFileDialog::getExistingDirectory(this, 
		tr("Select Download Folder"), QDir::homePath(), 
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	mDownloadFolder = QDir(dir);
}

void CommAudio::playSongButtonHandler()
{
	if (mPlayer->state() == QMediaPlayer::PlayingState)
	{
		mPlayer->pause();
		
		if (mPlayer->state() == QMediaPlayer::PausedState)
		{
			ui.btnPlaySong->setText("Play");
		}
	}

	if (mPlayer->state() == QMediaPlayer::StoppedState)
	{
		mPlayer->play();

		if (mPlayer->state() == QMediaPlayer::PlayingState)
		{
			ui.btnPlaySong->setText("Pause");
		}
	}

	if (mPlayer->state() == QMediaPlayer::PausedState)
	{
		mPlayer->play();

		if (mPlayer->state() == QMediaPlayer::PlayingState)
		{
			ui.btnPlaySong->setText("Pause");
		}
	}
}

void CommAudio::prevSongButtonHandler()
{

}

void CommAudio::nextSongButtonHandler()
{

}

void CommAudio::songProgressHandler(qint64 ms)
{
	int progress = (ms / mPlayer->duration()) * 100;
	QString milliseconds = QString::number(ms % 1000);
	QString seconds = QString::number((ms - (ms % 1000)) / 1000);
	// set label text
	ui.labelCurrentTime->setText(seconds + ":" + milliseconds);
	ui.sliderProgress->setValue(progress);
}

void CommAudio::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	QHostAddress address = socket->peerAddress();

	// Connect socket and add connection to map of pending connections
	connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);
	mPendingConnections[address.toString()] = socket;
}

void CommAudio::incomingDataHandler()
{
	QTcpSocket * sender = (QTcpSocket *)QObject::sender();
	QByteArray data = sender->readAll();

	if (mIsHost)
	{
		parsePacketHost(sender, data);
	}
	else
	{
		parsePacketClient(sender, data);
	}
}

void CommAudio::parsePacketHost(const QTcpSocket * sender, const QByteArray data)
{
	QHostAddress address = sender->peerAddress();

}

void CommAudio::parsePacketClient(const QTcpSocket * sender, const QByteArray data)
{
	QHostAddress address = sender->peerAddress();
}