#pragma once

#include <assert.h>
#include <QDebug>

#include <QAction>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHostAddress>
#include <QHostInfo>
#include <QInputDialog>
#include <QList>
#include <QMap>
#include <QMediaPlayer>
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

#include "Headers.h"
#include "ConnectionManager.h"

#define SUPPORTED_FORMATS { "*.wav", "*.mp3" }

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

	QMediaPlayer * mPlayer;

	//QTcpServer mServer;
	ConnectionManager mConnectionManager;

	QMap<QString, QTcpSocket *> mConnections;

	// Functions
	void populateLocalSongsList();
	void loadSong(const QString songname);

	void parsePacketHost(const QTcpSocket * sender, const QByteArray data);
	void parsePacketClient(const QTcpSocket * sender, const QByteArray data);

	void connectToAllOtherClients(const QByteArray data);

private slots:
	// UI
	void hostSessionHandler();
	void joinSessionHandler();
	void leaveSessionHandler();

	void changeNameHandler();
	void changeSongFolderHandler();
	void changeDownloadFolderHandler();

	void playSongButtonHandler();
	void prevSongButtonHandler();
	void nextSongButtonHandler();

	void seekPositionHandler(int position);

	void songStateChangeHandler(QMediaPlayer::State state);
	void songProgressHandler(qint64 ms);
	void songDurationHandler(qint64 ms);

	void localSongClickedHandler(QTreeWidgetItem * item, int column);

	// Networking
	void newConnectionHandler(QString name, QTcpSocket * socket);
	void incomingDataHandler();
};
