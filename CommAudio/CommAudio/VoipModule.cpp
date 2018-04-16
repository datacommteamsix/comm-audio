#include <VoipModule.h>

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

VoipModule::~VoipModule()
{
	Stop();
}

void VoipModule::Start()
{
	mServer.listen(QHostAddress::Any, 42070);
}

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

void VoipModule::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	connect(socket, &QTcpSocket::readyRead, this, &VoipModule::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &VoipModule::clientDisconnectHandler);

	mConnections[address] = socket;
	mInputs[address] = new QAudioInput(mFormat, this);
	mInputs[address]->start(socket);
}

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

void VoipModule::newClientHandler(QHostAddress address)
{
	QTcpSocket * socket = new QTcpSocket(this);

	connect(socket, &QTcpSocket::readyRead, this, &VoipModule::incomingDataHandler);
	connect(socket, &QTcpSocket::disconnected, this, &VoipModule::clientDisconnectHandler);

	socket->connectToHost(address, 42070);

	mConnections[address.toIPv4Address()] = socket;
	mInputs[address.toIPv4Address()] = new QAudioInput(mFormat, this);
	mInputs[address.toIPv4Address()]->start(socket);
}

void VoipModule::clientDisconnectHandler()
{
	QTcpSocket * sender = (QTcpSocket *)QObject::sender();
	quint32 address = sender->peerAddress().toIPv4Address();

	mOutputs[address]->stop();
	mOutputs.take(address)->deleteLater();

	mInputs[address]->stop();
	mOutputs.take(address)->deleteLater();

	mConnections[address]->close();
	mConnections.take(address)->deleteLater();
}