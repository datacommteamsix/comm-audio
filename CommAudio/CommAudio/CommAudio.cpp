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
	, mConnections()
	, mIpToName()
	, mOwnerToSong()
	, mConnectionManager(&mName, this)
	//, mVoip(this)
{
	ui.setupUi(this);

	// Create the Media Player
	mMediaPlayer = new MediaPlayer(&ui, this);

	// Setting default folder to home/comm-audio
	QDir tmp = QDir(QDir::homePath() + "/comm-audio");
	mSongFolder = tmp;
	mDownloadFolder = tmp;

	// Song Lists
	connect(ui.treeLocalSongs, &QTreeWidget::itemClicked, this, &CommAudio::localSongClickedHandler);

	connect(ui.treeRemoteSongs, &QTreeWidget::itemClicked, this, &CommAudio::remoteSongClickedHandler);
	connect(ui.treeRemoteSongs, &QTreeWidget::itemDoubleClicked, this, &CommAudio::remoteDoubleClickedHandler);

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

	// Networking set up
	connect(&mConnectionManager, &ConnectionManager::connectionAccepted, this, &CommAudio::newConnectionHandler);

	// Connect signal for VoIP module
	//connect(this, &CommAudio::connectVoip, &mVoip, &VoipModule::newClientHandler);

	mConnectionManager.Init(&mConnections);
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
	}

	delete mMediaPlayer;
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
	for (QString song : songs)
	{
		items.append(new QTreeWidgetItem(ui.treeLocalSongs, QStringList(song)));
	}

	// Add the list of widgets to tree
	ui.treeLocalSongs->insertTopLevelItems(0, items);
	mMediaPlayer->updateSongList(items);
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

	// Create connection
	QTcpSocket * socket = new QTcpSocket(this);
	socket->connectToHost(TEST_HOST_IP, 42069);
	mConnections["Hard Coded Host Name"] = socket;
	mIpToName[QHostAddress(TEST_HOST_IP).toIPv4Address()] = "Hard Coded Host Name";
	connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &CommAudio::remoteDisconnectHandler);

	// Send data
	socket->write(joinRequest);

	QStringList host;
	host << "Hard Coded Host Name" << "Host";
	ui.treeUsers->insertTopLevelItem(ui.treeUsers->topLevelItemCount(), new QTreeWidgetItem(ui.treeUsers, host));

	//emit connectVoip(QHostAddress(TEST_HOST_IP));
}

void CommAudio::leaveSessionHandler()
{
	//If you are a host
	mConnectionManager.BecomeClient();

	// Send notice of leave to all connected members

	// Disconnect from all memebers
	for (QTcpSocket * socket : mConnections)
	{
		disconnect(socket, &QTcpSocket::disconnected, this, &CommAudio::remoteDisconnectHandler);
		socket->close();
		socket->deleteLater();
	}

	mConnections.clear();
	mIpToName.clear();

	//clear the treeUsers
	ui.treeUsers->clear();
	ui.treeRemoteSongs->clear();
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
	mMediaPlayer->SetDirAndSong(mSongFolder, item);
	mMediaPlayer->SetSong(mSongFolder.absoluteFilePath(item->text(0)));
}

void CommAudio::remoteSongClickedHandler(QTreeWidgetItem * item, int column)
{
	QTcpSocket * socket = mConnections[item->text(1)];
	QString songName = item->text(0);
	qDebug() << "requesting stream for song" << songName << "from" << socket->peerAddress().toString();
}

void CommAudio::remoteSongDoubleClickedHandler(QTreeWidgetItem * item, int column)
{
	QTcpSocket * socket = mConnections[item->text(1)];
	QString songName = item->text(0);
	qDebug() << "requesting download for song" << songName << "from" << socket->peerAddress().toString();
	requestToDownload(socket, songName);
}

void CommAudio::requestToDownload(QTcpSocket * sender, QString songname)
{
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::RequestDownload);
	packet.append(mSessionKey);
	packet.append(songname);
	packet.resize(1 + 32 + 255);

	// Send
	sender->write(packet);
	qDebug() << "Sent request for download";
}

void CommAudio::uploadSong(QTcpSocket * sender, const QByteArray data)
{
	QString songname = QString(data.mid(1 + 32));
	qDebug() << "Requested song is: " << songname;

	QByteArray packet = QByteArray(1, (char)Headers::RespondDownload);

	qDebug() << "Uploading song";
	/*QFile * mSongFile;
	mSongFile = new QFile(mSongFolder.absoluteFilePath(songname));
	mSongFile->open(QFile::ReadOnly);
	packet.append(mSessionKey);
	packet.append(mSongFile->readAll());
	sender->write(packet);
	mSongFile->close();*/
}

void CommAudio::newConnectionHandler(QString name, QTcpSocket * socket)
{
	assert(socket != nullptr);
	assert(!mConnections.contains(name));

	//Insert name and ip to map
	mIpToName.insert(socket->peerAddress().toIPv4Address(), name);

	// Add the new client to the map of clients
	mConnections[name] = socket;
	connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &CommAudio::remoteDisconnectHandler);

	// Add the client to the tree view
	QStringList client;
	client << name << "Client";
	ui.treeUsers->insertTopLevelItem(ui.treeUsers->topLevelItemCount(), new QTreeWidgetItem(ui.treeUsers, client));
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

void CommAudio::parsePacketHost(QTcpSocket * sender, const QByteArray data)
{
	QHostAddress address = sender->peerAddress();
	switch (data[0])
	{
	case Headers::RequestForSongs:
		sendSongList(sender);
		break;
	case Headers::ReturnWithSongs:
		displaySongName(data, sender);
		break;
	case Headers::RequestDownload:
		uploadSong(sender, data);
		break;
	case Headers::RespondDownload:
		downloadSong(data);
		break;
	case Headers::RequestAudioStream:
		startStreamingSong(sender, data);
		break;
	}
}

void CommAudio::parsePacketClient(QTcpSocket * sender, const QByteArray data)
{
	switch (data[0])
	{
	case Headers::RespondToJoin:
		requestForSongs(sender);
		connectToAllOtherClients(data);
		break;
	case Headers::RespondWithName:
		displayClientName(data, sender);
		requestForSongs(sender);
		break;
	case Headers::RequestForSongs:
		sendSongList(sender);
		break;
	case Headers::RespondWithSongs:
		displaySongName(data, sender);
		returnSongList(sender);
		break;
	case Headers::ReturnWithSongs:
		displaySongName(data, sender);
		break;
	case Headers::RequestDownload:
		uploadSong(sender, data);
		break;
	case Headers::RespondDownload:
		downloadSong(data);
		break;
	case Headers::RespondAudioStream:
		playStreamSong(data);
		break;
	default:
		break;
	}
}

void CommAudio::startStreamingSong(QTcpSocket * sender, const QByteArray data)
{
	QByteArray packet = QByteArray(1, (char)Headers::RespondAudioStream);
	//packet.append(mSessionKey);
	//packet.resize(1 + 32);
}

void CommAudio::playStreamSong(const QByteArray Data)
{
}

void CommAudio::downloadSong(QByteArray packet)
{
	QByteArray data = packet.mid(33);
	// read ...
	QFile file("D:\\Downloads\\test.wav"); // change path
	file.open(QIODevice::WriteOnly);
	file.write(data);
	file.close();
}

void CommAudio::requestForSongs(QTcpSocket * socket)
{
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::RequestForSongs);
	packet.append(mSessionKey);
	packet.resize(1 + 33);

	// Send
	socket->write(packet);
}

void CommAudio::returnSongList(QTcpSocket * socket)
{
	int initSize = 1 + 32 + 4;
	int songNameSize = 255;
	quint32 songSize = items.size();
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::ReturnWithSongs);
	packet.append(mSessionKey);
	packet << songSize;

	for (QTreeWidgetItem * item : items)
	{
		packet.append((item->text(0)).toUtf8());
		initSize += songNameSize;
		packet.resize(initSize);
	}

	// Send
	socket->write(packet);
}

void CommAudio::sendSongList(QTcpSocket * socket)
{
	int initSize = 1 + 32 + 4;
	int songNameSize = 255;
	quint32 songSize = items.size();
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::RespondWithSongs);
	packet.append(mSessionKey);
	packet << songSize;

	for (QTreeWidgetItem * item : items)
	{
		packet.append((item->text(0)).toUtf8());
		initSize += songNameSize;
		packet.resize(initSize);
	}

	// Send
	socket->write(packet);
}

void CommAudio::connectToAllOtherClients(const QByteArray data)
{
	// Grab session key
	mSessionKey = data.mid(1, 32);

	// Grab the length
	int length = (int)data[33];

	// Craft connect request
	QByteArray joinRequest = QByteArray(1 + 33, (char)0);
	joinRequest[0] = (char)Headers::RequestToJoin;
	joinRequest.replace(1, mName.size(), mName.toStdString().c_str());
	joinRequest.resize(1 + 33);

	assert(joinRequest.size() == 1 + 33);

	// Send connect request to all other clients in the session
	int offset = 1 + 32 + 1;

	for (int i = 0; i < length; i++)
	{
		// TODO: Make sure this is reading address correctly
		quint32 addressInt = -1;
		QDataStream(data.mid(offset, 4)) >> addressInt;
		QHostAddress qHostAddress = QHostAddress(addressInt);
		QString address = qHostAddress.toString();

		//emit connectVoip(qHostAddress);

		QTcpSocket * socket = new QTcpSocket(this);
		connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);
		connect(socket, &QTcpSocket::disconnected, this, &CommAudio::remoteDisconnectHandler);

		socket->connectToHost(address, 42069);
		socket->write(joinRequest);
		offset += 4;
	}
}

//This is to handle a disconnect from a client requesting the leave session and to this client
void CommAudio::remoteDisconnectHandler()
{
	//Get the socket that sent the signal
	QTcpSocket * sender = (QTcpSocket *)QObject::sender();

	//Get the peer address as quint32
	quint32 address = sender->peerAddress().toIPv4Address();
	//Get the name of the client using the dictionary converter
	QString clientName = mIpToName[address];

	for (int i = 0; i < mConnections.size(); i++)
	{
		if (ui.treeUsers->topLevelItem(i)->text(0) == clientName)
		{
			delete ui.treeUsers->takeTopLevelItem(i);
			break;
		}
	}

	//Delete client from connections
	mConnections.remove(clientName);
	mIpToName.remove(address);

	//Delete the client songs
	QList<QTreeWidgetItem*>* items = mOwnerToSong.value(clientName, NULL);

	for (int i = 0; i < items->length(); i++)
	{
		delete items->value(i);
	}

	//delete the socket
	sender->deleteLater();
}

void CommAudio::displayClientName(const QByteArray data, QTcpSocket * socket)
{
	quint32 address = socket->peerAddress().toIPv4Address();
	mIpToName[address] = QString(data.mid(1));
	QStringList otherClient;
	otherClient << mIpToName[address] << "Client";
	ui.treeUsers->insertTopLevelItem(ui.treeUsers->topLevelItemCount(), new QTreeWidgetItem(ui.treeUsers, otherClient));
}

void CommAudio::displaySongName(const QByteArray data, QTcpSocket * sender)
{
	quint32 length = -1;
	QDataStream(data.mid(1 + 32, 4)) >> length;

	int offset = 37;
	QStringList songList;

	QString clientName = mIpToName[sender->peerAddress().toIPv4Address()];

	//If the peer doesn't have any previous entries in the mOwnerToSong
	//then create a new QList for the owner
	if (!mOwnerToSong.contains(clientName))
	{
		mOwnerToSong.insert(clientName, new QList<QTreeWidgetItem*>());
	}

	for (quint32 i = 0; i < length; i++)
	{
		songList << QString(data.mid(offset, 255)) << clientName;
		offset += 255;

		// Append song and the widget item to the owner to song map
		mOwnerToSong.value(clientName, NULL)->append(new QTreeWidgetItem(ui.treeRemoteSongs, songList));
		songList.clear();
	}
}