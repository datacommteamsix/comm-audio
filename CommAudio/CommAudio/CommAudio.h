#pragma once

#include <assert.h>
#include <QDebug>

#include <QAction>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHostAddress>
#include <QHostInfo>
#include <QInputDialog>
#include <QList>
#include <QMap>
#include <QNetworkInterface>
#include <QPoint>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUrl>

#include <QtWidgets/QMainWindow>
#include "ui_CommAudio.h"

#include "ConnectionManager.h"
#include "globals.h"
#include "MediaPlayer.h"

class CommAudio : public QMainWindow
{
	Q_OBJECT

public:
	CommAudio(QWidget * parent = Q_NULLPTR);
	~CommAudio();

private:
	// Variables
	Ui::CommAudioClass ui;

	bool mIsHost;
	QString mName;
	QByteArray mSessionKey;

	QDir mSongFolder;
	QDir mDownloadFolder;

	MediaPlayer * mMediaPlayer;

	//QTcpServer mServer;
	ConnectionManager mConnectionManager;

	QMap<QString, QTcpSocket *> mConnections;
	QMap<quint32, QString> mIpToHostnames;

	// Functions
	void populateLocalSongsList();

	void parsePacketHost(const QTcpSocket * sender, const QByteArray data);
	void parsePacketClient(const QTcpSocket * sender, const QByteArray data);

	void connectToAllOtherClients(const QByteArray data);
	void displayClientName(const QByteArray data);

private slots:
	// Menu Bar 
	void hostSessionHandler();
	void joinSessionHandler();
	void leaveSessionHandler();

	void changeNameHandler();
	void changeSongFolderHandler();
	void changeDownloadFolderHandler();

	// Song Lists
	void localSongClickedHandler(QTreeWidgetItem * item, int column);

	// Networking
	void newConnectionHandler(QString name, QTcpSocket * socket);
	void incomingDataHandler();
	void remoteDisconnectHandler();
};
