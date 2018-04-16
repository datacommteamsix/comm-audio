#pragma once

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QHostAddress>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWidget>

class VoipModule : public QWidget
{
	Q_OBJECT

public:
	VoipModule(QWidget * parent = nullptr);
	~VoipModule();

	void Start();
	void Stop();

private:
	QAudioFormat mFormat;

	QTcpServer mServer;
	QMap<quint32, QTcpSocket *> mConnections;
	QMap<quint32, QAudioInput *> mInputs;
	QMap<quint32, QAudioOutput *> mOutputs;

private slots:
	void newConnectionHandler();
	void incomingDataHandler();
	void clientDisconnectHandler();

public slots:
	void newClientHandler(QHostAddress address);
};
