/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		CommAudio.cpp - An audio player that has stream and voip capabilities.
--
-- PROGRAM:			CommAudio
--
-- FUNCTIONS:
--
--
-- DATE:			March 26, 2018
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
-- TODO: Fill out
----------------------------------------------------------------------------------------------------------------------*/
#include "CommAudio.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio (QWidget * parent)
--						QWidget * parent: The parent widget.
--
-- RETURNS:			N/A
--
-- NOTES:
-- The constructor for the main window of the program. This constructor also acts as the main entry point of the program
-- in place of main(int argc, char * argv[]).
--
-- All slots of objects present during the start of the program are connected here.
--
-- Lastly, this is where the program also starts listening for tcp connections.
----------------------------------------------------------------------------------------------------------------------*/
CommAudio::CommAudio(QWidget * parent)
	: QMainWindow(parent)
	, mIsHost(false)
	, mName(QHostInfo::localHostName())
	, mSessionKey()
	, mPlayer(new QMediaPlayer())
	, mConnections()
	, mConnectionManager(&mConnections, this)
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

	// Seeking
	connect(ui.sliderProgress, &QSlider::sliderMoved, this, &CommAudio::seekPositionHandler);

	// Networking set up
	connect(&mConnectionManager, &ConnectionManager::connectionAccepted, this, &CommAudio::newConnectionHandler);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		~CommAudio
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		~CommAudio ()
--
-- RETURNS:			N/A
--
-- NOTES:
-- Deconstructor for the main window of the program. This is where fill clean up of all resources used by the program
-- takes place.
----------------------------------------------------------------------------------------------------------------------*/
CommAudio::~CommAudio()
{
	for (QTcpSocket * socket : mConnections)
	{
		socket->close();
		delete socket;
	}

	delete mPlayer;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::populateLocalSongsList
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::populateLocalSongsList ()
--
-- RETURNS:			void.		
--
-- NOTES:
-- This function grabs all the songs in the local songs folder that are encoded in one of the supported formats and
-- displays them in the local song list tree view.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::populateLocalSongsList()
{
	ui.treeLocalSongs->clear();

	QStringList songs = mSongFolder.entryList(SUPPORTED_FORMATS,
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::loadSong
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::loadSong (const QString songname)
--						const QString songname: The name of the song.
--
-- RETURNS:			void.		
--
-- NOTES:
-- Loads a song into the media player and prepares it to be played.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::loadSong(const QString songname)
{
	// Stop any current song
	mPlayer->stop();
	mPlayer->setPosition(0);

	// Set the song
	mPlayer->setMedia(QUrl::fromLocalFile(mSongFolder.absoluteFilePath(songname)));
	ui.labelCurrentSong->setText("Currently Playing: " + songname);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::hostSessionHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::hostSessionHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user selects the menu item to become a host.
--
-- A SHA3 256 byte array is generated and saved to be used as the current sessio key and the application is switched
-- into host mode by setting mIsHost to true.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::hostSessionHandler()
{
	// Generate session key
	QCryptographicHash hasher(QCryptographicHash::Sha3_256);

	for (int i = 0; i < rand() % 1000; i++)
	{
		switch (rand() % 4)
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
			// Do Nothing
		default:
			break;
		}
	}
	mSessionKey = hasher.result();

	// Set host mode to true
	mIsHost = true;

	// Set the connection manager to host mode;
	mConnectionManager.BecomeHost(mSessionKey);
}

void CommAudio::joinSessionHandler()
{
	mIsHost = false;
	mConnectionManager.BecomeClient();
	mSessionKey = QByteArray();

	// Send Request to join session
	QByteArray joinRequest = QByteArray(1 + 33, (char)0);
	joinRequest[0] = (char)Headers::RequestToJoin;
	joinRequest.replace(1, mName.size(), mName.toStdString().c_str());
	joinRequest.resize(1 + 33);

	assert(joinRequest.size() == 1 + 33);

	// Send request to host
	QTcpSocket * socket = new QTcpSocket(this);
	socket->connectToHost("70.79.180.224", 42069);
	mConnections["Hard Coded Host Name"] = socket;
	socket->write(joinRequest);

	QStringList host;
	host << "Hard Coded Host Name" << "Host";
	ui.treeUsers->insertTopLevelItem(ui.treeUsers->topLevelItemCount(), new QTreeWidgetItem(ui.treeUsers, host));
}

void CommAudio::leaveSessionHandler()
{
	mIsHost = false;
	mConnectionManager.BecomeClient();
	mSessionKey = QByteArray();

	// Send notice of leave to all connected members

	// Disconnect from all memebers
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::changeNameHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::changeNameHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user selects the menu item to change their name.
--
-- A message box is shown to the user to enter a new name. That name is then saved and used for communication with
-- other clients.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::changeNameHandler()
{
	mName = QInputDialog::getText(this, tr("Enter new name"), "Name: ", QLineEdit::Normal);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::changeSongFolderHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::changeSongFolderHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user selects the menu item to change the local song folder.
--
-- A file browser is shown to the user to select a new directory. Once a new directory is selected, the directory is
-- saved and the local songs tree view is populated with all the songs in that folder that are in a supported format.
-- This folder can be the same as the download folder.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::changeSongFolderHandler()
{
	QString dir = QFileDialog::getExistingDirectory(this, 
		tr("Select Song Folder"), QDir::homePath(), 
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	mSongFolder = QDir(dir);

	populateLocalSongsList();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::changeDownloadFolderHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::changeSongFolderHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user selects the menu item to change the downlaod destination folder.
--
-- A file browser is shown to the user to select a new direcotry. That directory is then used as the new download
-- location. This folder can be the same as the local song folder.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::changeDownloadFolderHandler()
{
	QString dir = QFileDialog::getExistingDirectory(this, 
		tr("Select Download Folder"), QDir::homePath(), 
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	mDownloadFolder = QDir(dir);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::playSongButtonHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::playSongButtonHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user pressed the play/pause button.
--
-- The state of the media player is changed based on the current state of the media player.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::playSongButtonHandler()
{
	switch (mPlayer->state())
	{
	case QMediaPlayer::PlayingState:
		mPlayer->pause();
		break;
	case QMediaPlayer::StoppedState:
	case QMediaPlayer::PausedState:
		mPlayer->play();
		break;
	default:
		break;
	}
}

void CommAudio::prevSongButtonHandler()
{

}

void CommAudio::nextSongButtonHandler()
{

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::seekPositionHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::seekPositionHandler (int position)
--						int position: The new position in the song.
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user moves the position slider for the song.
--
-- This function will cause the QMediaPlayer to seek to the new position in the song.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::seekPositionHandler(int position)
{
	mPlayer->setPosition(position);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::songStateChangedHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::songStateChangedHandler (QMediaPLayer::State state)
--						QMediaPlayer::State state: The state of the media player.
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the state of the media player changes.
--
-- Elements of the GUI that are tied to the state of the media player will be updated to the new state.
----------------------------------------------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::songProgressHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::songProgressHandler (qint64 ms)
--						qint64 ms: The progress of the song in milliseconds.
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the position of the song changes.
--
-- This function will update the GUI elements that are tied to how far along the song is like the slider and time label.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::songProgressHandler(qint64 ms)
{
	if (mPlayer->isAudioAvailable())
	{
		// Set label text
		qint64 milliseconds = ms % 1000;
		qint64 seconds = (ms - milliseconds) / 1000;
		qint64 minutes = (seconds - (seconds % 60)) / 60;

		QString labelText = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
		ui.labelCurrentTime->setText(labelText);

		// Update slider
		ui.sliderProgress->setValue(ms);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::songDurationHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::songDurationHandler (qint64 ms)
--						qint64 ms: The total length of the song in milliseconds.
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the total duration of the song changes.
--
-- This function will update the GUI elements that are tied to the total duration of the song like the time label.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::songDurationHandler(qint64 ms)
{
	if (mPlayer->isAudioAvailable())
	{
		// Change the label
		qint64 milliseconds = ms % 1000;
		qint64 seconds = ((ms - milliseconds) / 1000);
		qint64 minutes = (seconds - (seconds % 60)) / 60;

		QString labelText = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
		ui.labelTotalTime->setText(labelText);

		// Change slider max
		ui.sliderProgress->setMaximum(ms);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		CommAudio::localSongClickedHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		CommAudio::localSongClickedHandler (QTreeWidgetItem * item, int column)
--						QTreeWidgetItem * item: The QTreeWidgetItem that was clicked on.
--						int column: The colomn that was clicked on.
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user clicks on a song in the local songs list.
--
-- This function will grab the name of the song that was clicked on and set it as the current song in the QMediaPlayer.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::localSongClickedHandler(QTreeWidgetItem * item, int column)
{
	loadSong(item->text(0));
}

void CommAudio::newConnectionHandler(QString name, QTcpSocket * socket)
{
	assert(socket != nullptr);
	assert(!mConnections.contains(name));

	// Add the new client to the map of clients
	mConnections[name] = socket;
	connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);

	// Add the client to the tree view
	QStringList client;
	client << name << "Client";
	ui.treeUsers->insertTopLevelItem(ui.treeUsers->topLevelItemCount(), new QTreeWidgetItem(ui.treeUsers, client));

	qDebug() << name << "-" << socket << "was accepted successfully";
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

	// Host packet logic goes here
}

void CommAudio::parsePacketClient(const QTcpSocket * sender, const QByteArray data)
{
	QHostAddress address = sender->peerAddress();
	
	switch (data[0])
	{
	case Headers::RespondToJoin:
		connectToAllOtherClients(data);
		break;
	default:
		break;
	}

	// Client packet logic goes here
}

void CommAudio::connectToAllOtherClients(const QByteArray data)
{
	// Grab session key
	mSessionKey = data.mid(1, 32);

	// Grab the length
	int length = -1;
	QByteArray len = data.mid(32, sizeof(int));
	memcpy(&length, len.data(), sizeof(int));

	// Craft connect request

	// Send connect request to all other clients in the session
	int offset = 1 + 32 + 4;

	QByteArray joinRequest = QByteArray(1 + 33, (char)0);
	joinRequest[0] = (char)Headers::RequestToJoin;
	joinRequest.replace(1, mName.size(), mName.toStdString().c_str());
	joinRequest.resize(1 + 33);

	assert(joinRequest.size() == 1 + 33);

	for (int i = 0; i < length; i++)
	{
		QString address = QString(data.mid(offset, 32));
		QTcpSocket * socket = new QTcpSocket(this);
		socket->connectToHost(address, 42069);
		socket->write(joinRequest);
	}

}