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

	// Song Lists
	connect(ui.treeLocalSongs, &QTreeWidget::itemClicked, this, &CommAudio::localSongClickedHandler);

	// Configure the media player
	mPlayer->setVolume(100);
	// Set volume
	connect(ui.sliderVolume, &QSlider::sliderMoved, mPlayer, &QMediaPlayer::setVolume);
	// Song state changed
	connect(mPlayer, &QMediaPlayer::stateChanged, this, &CommAudio::songStateChangeHandler);
	// Song progress
	connect(mPlayer, &QMediaPlayer::positionChanged, this, &CommAudio::songProgressHandler);
	// Song length
	connect(mPlayer, &QMediaPlayer::durationChanged, this, &CommAudio::songDurationHandler);

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

void CommAudio::setCurrentlyPlayingSong(const QString songname)
{
	mPlayer->setMedia(QUrl::fromLocalFile(mSongFolder.absoluteFilePath(songname)));
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
	}

	if (mPlayer->state() == QMediaPlayer::StoppedState)
	{
		mPlayer->play();
	}

	if (mPlayer->state() == QMediaPlayer::PausedState)
	{
		mPlayer->play();
	}
}

void CommAudio::prevSongButtonHandler()
{

}

void CommAudio::nextSongButtonHandler()
{

}

void CommAudio::songStateChangeHandler(QMediaPlayer::State state)
{
	switch (state)
	{
	case QMediaPlayer::PlayingState:
		ui.btnPlaySong->setText("Pause");
		break;
	case QMediaPlayer::StoppedState:
	case QMediaPlayer::PausedState:
	default:
		ui.btnPlaySong->setText("Play");
		break;
	}
}

void CommAudio::songProgressHandler(qint64 ms)
{
	if (mPlayer->isAudioAvailable())
	{
		int progress = ((float)ms / mPlayer->duration()) * 100;
		ui.sliderProgress->setValue(progress);

		// set label text
		qint64 milliseconds = ms % 1000;
		qint64 seconds = (ms - milliseconds) / 1000;
		qint64 minutes = (seconds - (seconds % 60)) / 60;

		ui.labelCurrentTime->setText(QString::number(minutes) + ":" + QString::number(seconds));
	}
}

void CommAudio::songDurationHandler(qint64 ms)
{
	if (mPlayer->isAudioAvailable())
	{
		qint64 milliseconds = ms % 1000;
		qint64 seconds = (ms - milliseconds) / 1000;
		qint64 minutes = (seconds - (seconds % 60)) / 60;

		ui.labelTotalTime->setText(QString::number(minutes) + ":" + QString::number(seconds));
	}
}

void CommAudio::localSongClickedHandler(QTreeWidgetItem * item, int column)
{
	QString song = item->text(0);
	
	setCurrentlyPlayingSong(song);
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