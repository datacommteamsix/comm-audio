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

VoipModule::~VoipModule()
{
	for (QAudioOutput * output : mOutputs)
	{
		output->stop();
		output->deleteLater();
	}

	for (QAudioInput * input : mInputs)
	{
		input->stop();
		input->deleteLater();
	}

	for (QTcpSocket * socket : mConnections)
	{
		socket->close();
		socket->deleteLater();
	}
}

void VoipModule::newConnectionHandler()
{
	QTcpSocket * socket = mServer.nextPendingConnection();
	quint32 address = socket->peerAddress().toIPv4Address();

	mConnections[address] = socket;
	mInputs[address] = new QAudioInput(mFormat, this);
	mInputs[address]->start(socket);
	qDebug() << "inputs" << mInputs.size() << "outputs" << mOutputs.size() << "connections" << mConnections.size();
}

void VoipModule::incomingDataHandler()
{
	QTcpSocket * socket = (QTcpSocket *)QObject::sender();
	quint32 address = socket->peerAddress().toIPv4Address();

	if (!mOutputs.contains(address))
	{
		mOutputs[address] = new QAudioOutput(mFormat, this);
		mOutputs[address]->start(mConnections[address]);
		qDebug() << "inputs" << mInputs.size() << "outputs" << mOutputs.size() << "connections" << mConnections.size();
	}
}

void VoipModule::newClientHandler(QHostAddress address)
{
	QTcpSocket * socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &VoipModule::incomingDataHandler);
	socket->connectToHost(address, 42070);

	mConnections[address.toIPv4Address()] = socket;
	mInputs[address.toIPv4Address()] = new QAudioInput(mFormat, this);
	mInputs[address.toIPv4Address()]->start(socket);
	qDebug() << "inputs" << mInputs.size() << "outputs" << mOutputs.size() << "connections" << mConnections.size();
}