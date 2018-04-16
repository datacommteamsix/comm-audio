/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		VoipModule.cpp - A module that allows for two way voice communication over TCP/IP.
--
-- PROGRAM:			CommAudio
--
-- FUNCTIONS:
--					VoipModule(QWidget * parent = nullptr)
--					~VoipModule()
--					void newConnectionHandler()
--					void incomingDataHandler()
--					void clientDisconnectHandler()
--					void newClientHandler(QHostAddress address)
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
#include <VoipModule.h>

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		VoipModule
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
-- INTERFACE:		VoipModule (QWidget * parent)
--						QWidget * parent: The parent widget.
--
-- RETURNS:			N/A
--
-- NOTES:
--					Creates the voip module. This is where the audio format of the voice stream is established and
--					nothing else.
----------------------------------------------------------------------------------------------------------------------*/
VoipModule::VoipModule(QWidget * parent)
	: QWidget(parent)
	, mServer(this)
{
	// Set the voip format
	mFormat.setSampleRate(44100);
	mFormat.setSampleSize(16);
	mFormat.setChannelCount(2);
	mFormat.setCodec("audio/pcm");
	mFormat.setByteOrder(QAudioFormat::LittleEndian);
	mFormat.setSampleType(QAudioFormat::SignedInt);

	// Create the server to listen for new connections
	connect(&mServer, &QTcpServer::newConnection, this, &VoipModule::newConnectionHandler);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		~VoipModule
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
-- INTERFACE:		~VoipModule ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					Stops the voip moduele.
----------------------------------------------------------------------------------------------------------------------*/
VoipModule::~VoipModule()
{
	Stop();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		Start	
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
-- INTERFACE:		Start ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					Starts the voip module by listening for TCP connections.
----------------------------------------------------------------------------------------------------------------------*/
void VoipModule::Start()
{
	mServer.listen(QHostAddress::Any, VOIP_PORT);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		Stop	
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
-- INTERFACE:		Stop ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					Stops the voip module by no longer listening for new connections and closing all existing connections.
----------------------------------------------------------------------------------------------------------------------*/
void VoipModule::Stop()
{
	mServer.close();

	// This is done instead of iteration through the map of connections because sockets
	// are removed from the list when they are closed, thus the map would be resized as
	// we were iteration through it. This way we iterate through a temporary list so there
	// is no problem.
	QList<quint32> addresses = mConnections.keys();
	for (int i = 0; i < addresses.size(); i++)
	{
		mConnections[addresses[i]]->close();
	}
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
-- INTERFACE:		newConnectionHandler ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					This is the Qt slot that is triggered when a new TCP conneciton has been accepted. This function
--					will create a QAudioInput with that socket. Then both the socket and input are saved to the maps.
----------------------------------------------------------------------------------------------------------------------*/
void VoipModule::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (mConnections.contains(address))
	{
		socket->close();
		socket->deleteLater();
		return;
	}

	connect(socket, &QTcpSocket::readyRead, this, &VoipModule::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &VoipModule::clientDisconnectHandler);

	mConnections[address] = socket;
	mInputs[address] = new QAudioInput(mFormat, this);
	mInputs[address]->start(socket);
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
-- INTERFACE:		incomingDataHandler ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					This is the Qt slot that is triggered when there is data to read on a socket. This function will
--					create a QAudioOuput with the socket that triggered this function. It will then start the audio
--					output and save it to the map.
----------------------------------------------------------------------------------------------------------------------*/
void VoipModule::incomingDataHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (!mOutputs.contains(address))
	{
		mOutputs[address] = new QAudioOutput(mFormat, this);
		mOutputs[address]->start(mConnections[address]);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		newClientHandler	
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
-- INTERFACE:		newClientHandler (QHostAddress address)
--
-- RETURNS:			N/A
--
-- NOTES:
--					This is the Qt slot that is triggered when a new client address is emited. A new socket and
--					QAudioInput is made and stored in the maps. The socket then makes a request to connect.
----------------------------------------------------------------------------------------------------------------------*/
void VoipModule::newClientHandler(QHostAddress address)
{
	if (mConnections.contains(address.toIPv4Address()))
	{
		return;
	}

	QTcpSocket * socket = new QTcpSocket(this);

	socket->connectToHost(address, VOIP_PORT);

	connect(socket, &QTcpSocket::readyRead, this, &VoipModule::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &VoipModule::clientDisconnectHandler);

	mConnections[address.toIPv4Address()] = socket;
	mInputs[address.toIPv4Address()] = new QAudioInput(mFormat, this);
	mInputs[address.toIPv4Address()]->start(socket);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		clientDisconnectHandler
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
-- INTERFACE:		clientDisconnectHandler ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					This is a Qt slot that is triggered when a connection is disconnected. The socket and associated
--					QAudioInput and QAudioOuputs are closed and removed from their maps.
----------------------------------------------------------------------------------------------------------------------*/
void VoipModule::clientDisconnectHandler()
{
	QTcpSocket * sender = (QTcpSocket *)QObject::sender();
	quint32 address = sender->peerAddress().toIPv4Address();

	mConnections.take(address)->deleteLater();
	
	QAudioOutput * output = mOutputs.take(address);
	QAudioInput * input = mInputs.take(address);

	if (output)
	{
		output->stop();
		output->deleteLater();
	}

	if (input)
	{
		input->stop();
		input->deleteLater();
	}
}