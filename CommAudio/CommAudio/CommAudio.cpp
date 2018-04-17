/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		CommAudio.cpp - An audio player that has stream and voip capabilities.
--
-- PROGRAM:			CommAudio
--
-- FUNCTIONS:
--					QString getAddressFromUser()
--					void populateLocalSongsList()
--					void parsePacketHost(QTcpSocket * sender, const QByteArray data)
--					void parsePacketClient(QTcpSocket * sender, const QByteArray data)
--					void connectToAllOtherClients(const QByteArray data)
--					void displayClientName(const QByteArray data, QTcpSocket * sender)
--					void displaySongName(const QByteArray data, QTcpSocket * sender)
--					void requestForSongs(QTcpSocket * host)
--					void sendSongList(QTcpSocket * sender)
--					void returnSongList(QTcpSocket * sender)
--					void hostSessionHandler()
--					void joinSessionHandler()
--					void leaveSessionHandler()
--					void changeNameHandler()
--					void changeSongFolderHandler()
--					void changeDownloadFolderHandler()
--					void localSongClickedHandler(QTreeWidgetItem * item, int column)
--					void remoteSongClickedHandler(QTreeWidgetItem * item, int column)
--					void remoteMenuHandler(const QPoint & pos)
--					void downloadSong()
--					void newConnectionHandler(QString name, QTcpSocket * socket)
--					void incomingDataHandler()
--					void remoteDisconnectHandler()
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
--					This is the parent window class for the application.
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
--					The constructor for the main window of the program. This constructor also acts as the main entry 
--					point of the program in place of main(int argc, char * argv[]). All slots of objects present during 
--					the start of the program are connected here. Lastly, this is where the program also starts listening
--					for tcp connections.
----------------------------------------------------------------------------------------------------------------------*/
CommAudio::CommAudio(QWidget * parent)
	: QMainWindow(parent)
	, mIsHost(false)
	, mName(QHostInfo::localHostName())
	, mSessionKey()
	, mConnections()
	, mIpToName()
	, mOwnerToSong()
	, mConnectionManager(&mSessionKey, &mName, this)
	, mVoip(this)
	, mDownloadManager(&mSessionKey, &mSongFolder, &mDownloadFolder, this)
	, mStreamManager(&mSessionKey, &mSongFolder, &mDownloadFolder, this)
{
	ui.setupUi(this);
	setWindowTitle(TITLE_DEFAULT);

	// Create the Media Player
	mMediaPlayer = new MediaPlayer(&ui, this);
	mStreamManager.mMediaPlayer = mMediaPlayer;

	// Setting default folder to home/comm-audio
	QDir tmp = QDir(QDir::homePath() + "/comm-audio");
	mSongFolder = tmp;
	mDownloadFolder = tmp;

	ui.treeRemoteSongs->setContextMenuPolicy(Qt::CustomContextMenu);

	// Song Lists
	connect(ui.treeLocalSongs, &QTreeWidget::itemClicked, this, &CommAudio::localSongClickedHandler);
	connect(ui.treeRemoteSongs, &QTreeWidget::itemClicked, this, &CommAudio::remoteSongClickedHandler);
	connect(ui.treeRemoteSongs, &QTreeWidget::customContextMenuRequested, this, &CommAudio::remoteMenuHandler);

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
	connect(this, &CommAudio::connectVoip, &mVoip, &VoipModule::newClientHandler);

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
--					Deconstructor for the main window of the program. This is where fill clean up of all resources used 
--					by the program takes place.
----------------------------------------------------------------------------------------------------------------------*/
CommAudio::~CommAudio()
{
	QList<QString> keys = mConnections.keys();

	for (int i = 0; i < keys.size(); i++)
	{
		mConnections[keys[i]]->close();
	}

	delete mMediaPlayer;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		populateLocalSongsList
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
-- INTERFACE:		populateLocalSongsList ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This function grabs all the songs in the local songs folder that are encoded in one of the supported 
--					formats and displays them in the local song list tree view.
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
	mMediaPlayer->UpdateSongList(items);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		getAddressFromUser
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
-- INTERFACE:		getAddressFromUser ()
--
-- RETURNS:			The string the user gives if it is a valid dotted decimal address, otherwise an empty string is returned.
--
-- NOTES:
--					Creates a pop up box that asks the user for an ip address then checks the give address against a
--					regex that checks for dotted decimal format. Values of the numbers are not checked.
----------------------------------------------------------------------------------------------------------------------*/
QString CommAudio::getAddressFromUser()
{
	bool ok;
	QRegExp rx("(?:[0-9]{1,3}\.){3}[0-9]{1,3}");
    QString text = QInputDialog::getText(this, tr("Enter Host Address"), "Host Address:", 
											QLineEdit::Normal, "", &ok);

	if (ok && rx.exactMatch(text))
	{
		return text;
	}
	else
	{
		return "";
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		hostSessionHandler
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
-- INTERFACE:		hostSessionHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user selects the menu item to become a host. A SHA3 
--					256 byte array is generated and saved to be used as the current sessio key and the application is 
--					switched into host mode by setting mIsHost to true.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::hostSessionHandler()
{
	leaveSessionHandler();
	ui.actionHostSession->setDisabled(true);
	ui.actionJoinSession->setDisabled(true);

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
	mConnectionManager.BecomeHost();

	mVoip.Start();

	setWindowTitle(TITLE_HOST);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		joinSessionHandler
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
-- INTERFACE:		joinSessionHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user selects the menu item to become a host. A SHA3 
--					256 byte array is generated and saved to be used as the current sessio key and the application is 
--					switched into host mode by setting mIsHost to true. Lastly, a connection the the given host address
--					is made if the input was valid.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::joinSessionHandler()
{
	leaveSessionHandler();

	mVoip.Start();

	// Send Request to join session
	QByteArray joinRequest = QByteArray(1 + 33, (char)0);
	joinRequest[0] = (char)Headers::RequestToJoin;
	joinRequest.replace(1, mName.size(), mName.toStdString().c_str());
	joinRequest.resize(1 + 33);

	// Create connection
	QString address = getAddressFromUser();

	if (address == "")
	{
		QMessageBox::warning(this, "Invalid Address", "The address you entered was invalid");
		return;
	}
	
	QHostAddress hostAddress = QHostAddress(address);
	QTcpSocket * socket = new QTcpSocket(this);
	socket->connectToHost(hostAddress, DEFAULT_PORT);

	if (!socket->waitForConnected(CONNECT_TIMEOUT))
	{
		socket->deleteLater();
		QMessageBox::warning(this, "Connetion Error", "Connection Timed Out");
		return;
	}

	mConnections[address] = socket;
	mIpToName[hostAddress.toIPv4Address()] = address;
	connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &CommAudio::remoteDisconnectHandler);

	// Send data
	socket->write(joinRequest);

	emit connectVoip(hostAddress);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		leaveSessionHandler
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
-- INTERFACE:		leaveSessionHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					Leaves the current session and sets the state of all components in the application the the default
--					state.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::leaveSessionHandler()
{
	ui.actionHostSession->setDisabled(false);
	ui.actionJoinSession->setDisabled(false);
	setWindowTitle(TITLE_DEFAULT);

	mIsHost = false;
	mConnectionManager.BecomeClient();
	mVoip.Stop();

	mSessionKey = QByteArray();

	// Disconnect from all memebers
	for (QTcpSocket * socket : mConnections)
	{
		disconnect(socket, &QTcpSocket::disconnected, this, &CommAudio::remoteDisconnectHandler);
		socket->close();
	}

	mConnections.clear();
	mIpToName.clear();

	//clear the treeUsers
	ui.treeUsers->clear();
	ui.treeRemoteSongs->clear();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		changeNameHandler
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
-- INTERFACE:		changeNameHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user selects the menu item to change their name. A 
--					message box is shown to the user to enter a new name. That name is then saved and used for 
--					communication with other clients.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::changeNameHandler()
{
	mName = QInputDialog::getText(this, tr("Enter new name"), "Name: ", QLineEdit::Normal);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		changeSongFolderHandler
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
-- INTERFACE:		changeSongFolderHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user selects the menu item to change the local song folder.
--					A file browser is shown to the user to select a new directory. Once a new directory is selected, the
--					directory is saved and the local songs tree view is populated with all the songs in that folder that 
--					are in a supported format.  This folder can be the same as the download folder.
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
-- FUNCTION:		changeDownloadFolderHandler
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
-- INTERFACE:		changeSongFolderHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user selects the menu item to change the downlaod 
--					destination folder. A file browser is shown to the user to select a new direcotry. That directory 
--					is then used as the new download location. This folder can be the same as the local song folder.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::changeDownloadFolderHandler()
{
	QString dir = QFileDialog::getExistingDirectory(this, 
		tr("Select Download Folder"), QDir::homePath(), 
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	mDownloadFolder = QDir(dir);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		localSongClickedHandler
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
-- INTERFACE:		localSongClickedHandler (QTreeWidgetItem * item, int column)
--						QTreeWidgetItem * item: The QTreeWidgetItem that was clicked on.
--						int column: The colomn that was clicked on.
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user clicks on a song in the local songs list. This 
--					function will grab the name of the song that was clicked on and set it as the current song in the
--					MediaPlayer.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::localSongClickedHandler(QTreeWidgetItem * item, int column)
{
	mMediaPlayer->SetDirAndSong(mSongFolder, item);
	mMediaPlayer->SetSong(mSongFolder.absoluteFilePath(item->text(0)));
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		remoteSongClickedHandler
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
-- INTERFACE:		remoteSongCLickedHandler (QTreeWidgetItem * item, int column)
--						QTreeWidgetItem * item: The QTreeWidgetItem that was clicked on.
--						int column: The colomn that was clicked on.
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user clicks on a song in the remote songs list. A request
--					to stream that song is made to the owner of the song.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::remoteSongClickedHandler(QTreeWidgetItem * item, int column)
{
	QTcpSocket * socket = mConnections[item->text(1)];
	QString songName = item->text(0);
	mStreamManager.StreamSong(songName, socket->peerAddress().toIPv4Address());
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		remoteMenuHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		remoteMenuHandler (const QPoint & pos)
--						const QPoint & pos: The point that was clicked on.
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user right clicks on a song in the remote songs list. 
--					A menu pops up where the only option is to download the song, if the user clicks on the download
--					menu option, then a download request is triggered.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::remoteMenuHandler(const QPoint & pos)
{
	nd = ui.treeRemoteSongs->itemAt(pos);

	QAction *newAct = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Download"), this);
	newAct->setStatusTip(tr("Download Song"));
	connect(newAct, &QAction::triggered, this, &CommAudio::downloadSong);

	QMenu menu(this);
	menu.addAction(newAct);

	QPoint pt(pos);
	menu.exec(ui.treeRemoteSongs->mapToGlobal(pos));
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		downloadSong
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		downloadSong ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					Makes a download request for the song the user clicked on.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::downloadSong()
{
	QTcpSocket * socket = mConnections[nd->text(1)];
	mDownloadManager.DownloadFile(nd->text(0), socket->peerAddress().toIPv4Address());
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		newConnectionHandler
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
-- INTERFACE:		newConnectionHandler (QString name, QTcpSocket * socket)
--						QString name: The name of the client.
--						QTcpSocket * socket: The socket of the client.
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the ConnectionManager releases a new valid connection.
--					That connection is then added the the list of valid connections and they are displayed on the GUI
--					for the user.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::newConnectionHandler(QString name, QTcpSocket * socket)
{
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		incomingDataHandler
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
-- INTERFACE:		incomginDataHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when there is data on one of the ports. The data is handled 
--					depending on whether or not the application is currently in client mode or host mode.
----------------------------------------------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		parsePacketHost
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
-- INTERFACE:		parsePacketHost ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					Handles the incoming packet as a host based on the protocol.
----------------------------------------------------------------------------------------------------------------------*/
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
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		parsePacketClient
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
-- INTERFACE:		parsePacketClient ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					Handles the incoming packet as a host based on the protocol.
----------------------------------------------------------------------------------------------------------------------*/
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
	default:
		break;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		requestForSongs
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
-- INTERFACE:		requestForSongs (QTcpSocket * socket)
--						QTcpSocket * socket: The socket to request from.
--
-- RETURNS:			void.		
--
-- NOTES:
--					Makes a request for songs to the socket.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::requestForSongs(QTcpSocket * socket)
{
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::RequestForSongs);
	packet.append(mSessionKey);
	packet.resize(1 + KEY_SIZE);

	// Send
	socket->write(packet);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		returnSongList
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		returnSongList (QTcpSocket * socket)
--						QTcpSocket * socket: The socket to write to.
--
-- RETURNS:			void.		
--
-- NOTES:
--					Returns the list of currently selected songs to the socket as a response to a request.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::returnSongList(QTcpSocket * socket)
{
	int initSize = 1 + KEY_SIZE + 4;
	quint32 songSize = items.size();
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::ReturnWithSongs);
	packet.append(mSessionKey);
	packet << songSize;

	for (QTreeWidgetItem * item : items)
	{
		packet.append((item->text(0)).toUtf8());
		initSize += SONGNAME_SIZE;
		packet.resize(initSize);
	}

	// Send
	socket->write(packet);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		returnSongList
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		returnSongList (QTcpSocket * socket)
--						QTcpSocket * socket: The socket to write to.
--
-- RETURNS:			void.		
--
-- NOTES:
--					Sends a list of currently selected songs to the socket.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::sendSongList(QTcpSocket * socket)
{
	int initSize = 1 + KEY_SIZE + 4;
	quint32 songSize = items.size();
	// Create packet
	QByteArray packet = QByteArray(1, (char)Headers::RespondWithSongs);
	packet.append(mSessionKey);
	packet << songSize;

	for (QTreeWidgetItem * item : items)
	{
		packet.append((item->text(0)).toUtf8());
		initSize += SONGNAME_SIZE;
		packet.resize(initSize);
	}

	// Send
	socket->write(packet);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		connectToAllOtherClients
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
--					Roger Zhang
--
-- INTERFACE:		connectToAllOtherClients (const QByteArray data)
--						const QByteArray data: The incoming data.
--
-- RETURNS:			void.		
--
-- NOTES:
--					Sends a connect request to all other clients that were sent to over in the data.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::connectToAllOtherClients(const QByteArray data)
{
	setWindowTitle(TITLE_CLIENT);
	ui.actionHostSession->setDisabled(true);
	ui.actionJoinSession->setDisabled(true);

	// Grab session key
	mSessionKey = data.mid(1, KEY_SIZE);

	QStringList host;
	host << data.mid(1 + KEY_SIZE, USER_NAME_SIZE) << "Host";
	ui.treeUsers->insertTopLevelItem(ui.treeUsers->topLevelItemCount(), new QTreeWidgetItem(ui.treeUsers, host));

	// Grab the length
	int length = (int)data[1 + KEY_SIZE + USER_NAME_SIZE];

	// Craft connect request
	QByteArray joinRequest = QByteArray(1 + KEY_SIZE + USER_NAME_SIZE, (char)0);
	joinRequest[0] = (char)Headers::RequestToJoin;
	joinRequest.replace(1, mSessionKey.size(), mSessionKey);
	joinRequest.replace(1 + 32, mName.size(), mName.toStdString().c_str());

	// Send connect request to all other clients in the session
	int offset = 1 + KEY_SIZE + 1;

	for (int i = 0; i < length; i++)
	{
		quint32 addressInt = -1;
		QDataStream(data.mid(offset, 4)) >> addressInt;
		QHostAddress qHostAddress = QHostAddress(addressInt);
		QString address = qHostAddress.toString();

		emit connectVoip(qHostAddress);

		QTcpSocket * socket = new QTcpSocket(this);
		connect(socket, &QTcpSocket::readyRead, this, &CommAudio::incomingDataHandler);
		connect(socket, &QTcpSocket::disconnected, this, &CommAudio::remoteDisconnectHandler);

		socket->connectToHost(address, DEFAULT_PORT);
		socket->write(joinRequest);
		offset += 4;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		remoteDisconnectHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Angus Lam
--
-- INTERFACE:		remoteDisconnectHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					Cleans up after a socket has disconnected.
----------------------------------------------------------------------------------------------------------------------*/
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
	QList<QTreeWidgetItem*>* items = mOwnerToSong.take(clientName);

	for (int i = 0; i < items->size(); i++)
	{
		delete items->at(i);
	}

	delete items;

	//delete the socket
	sender->deleteLater();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		displayClientName
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
-- INTERFACE:		displayClientName (const QByteArray data, QTcpSocket * socket)
--						cosnt QByteArray data: The data containing the name.
--						QTcpSocket * socket: The socket that sent the data.
--
-- RETURNS:			void.		
--
-- NOTES:
--					Displays the client's name on the GUI for the user.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::displayClientName(const QByteArray data, QTcpSocket * socket)
{
	quint32 address = socket->peerAddress().toIPv4Address();
	mIpToName[address] = QString(data.mid(1));
	QStringList otherClient;
	otherClient << mIpToName[address] << "Client";
	ui.treeUsers->insertTopLevelItem(ui.treeUsers->topLevelItemCount(), new QTreeWidgetItem(ui.treeUsers, otherClient));
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		displayClientName
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		displaySongName (const QByteArray data, QTcpSocket * socket)
--						cosnt QByteArray data: The data containing the list of songs.
--						QTcpSocket * socket: The socket that sent the data.
--
-- RETURNS:			void.		
--
-- NOTES:
--					Displays the list of incoming songs on the GUI for the user.
----------------------------------------------------------------------------------------------------------------------*/
void CommAudio::displaySongName(const QByteArray data, QTcpSocket * sender)
{
	quint32 length = -1;
	QDataStream(data.mid(1 + KEY_SIZE, 4)) >> length;

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
		songList << QString(data.mid(offset, SONGNAME_SIZE)) << clientName;
		offset += SONGNAME_SIZE;

		// Append song and the widget item to the owner to song map
		mOwnerToSong.value(clientName, NULL)->append(new QTreeWidgetItem(ui.treeRemoteSongs, songList));
		songList.clear();
	}
}