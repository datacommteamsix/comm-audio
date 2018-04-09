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
	mServer.listen(QHostAddress::Any, 42070);
}

void VoipModule::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	QAudioOutput * output = new QAudioOutput(mFormat, this);
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections[address] = socket;
	mOutputs[address] = output;
}

void VoipModule::incomingDataHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
}

void VoipModule::newClientHandler(QHostAddress address)
{
	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &VoipModule::incomingDataHandler);
	socket->connectToHost(address, 42070);
}