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
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPoint>
#include <QPushButton>
#include <QRegExp>
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
#include "VoipModule.h"
#include "DownloadManager.h"
#include "StreamManager.h"

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
	QTreeWidgetItem *nd;

	QDir mSongFolder;
	QDir mDownloadFolder;
	QList<QTreeWidgetItem *> items;

	QMap<QString, QTcpSocket *> mConnections;
	QMap<quint32, QString> mIpToName;
	QMap<QString, QList<QTreeWidgetItem*>*> mOwnerToSong;

	// Components
	ConnectionManager mConnectionManager;
	VoipModule mVoip;
	MediaPlayer * mMediaPlayer;
	DownloadManager mDownloadManager;
	StreamManager mStreamManager;

	// Functions
	QString getAddressFromUser();

	void populateLocalSongsList();

	void parsePacketHost(QTcpSocket * sender, const QByteArray data);
	void parsePacketClient(QTcpSocket * sender, const QByteArray data);

	void connectToAllOtherClients(const QByteArray data);
	void displayClientName(const QByteArray data, QTcpSocket * sender);
	void displaySongName(const QByteArray data, QTcpSocket * sender);

	void requestForSongs(QTcpSocket * host);
	void sendSongList(QTcpSocket * sender);
	void returnSongList(QTcpSocket * sender);

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
	void remoteSongClickedHandler(QTreeWidgetItem * item, int column);
	void remoteMenuHandler(const QPoint & pos);
	void downloadSong();

	// Networking
	void newConnectionHandler(QString name, QTcpSocket * socket);
	void incomingDataHandler();
	void remoteDisconnectHandler();

signals:
	void connectVoip(QHostAddress address);

};
