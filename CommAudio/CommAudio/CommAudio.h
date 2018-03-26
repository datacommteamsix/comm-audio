#pragma once

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

#include <QtWidgets/QMainWindow>
#include "ui_CommAudio.h"

class CommAudio : public QMainWindow
{
	Q_OBJECT

public:
	CommAudio(QWidget * parent = Q_NULLPTR);
	~CommAudio();

private:

	enum Headers
	{
		RequestToJoin,
		AcceptJoin,
		RequestForSongs,
		RespondWithSongs,
		RequestAudioStream,
		RequestDownload,
		RequestUpload,
		NotifyQuit
	};

	Ui::CommAudioClass ui;

	bool mIsHost;
	QString mName;
	QByteArray mSessionKey;

	QDir mSongFolder;
	QDir mDownloadFolder;

	QMediaPlayer * mPlayer;

	QTcpServer mServer;

	QMap<QString, QTcpSocket *> mPendingConnections;
	QMap<QString, QTcpSocket *> mConnections;

	void populateLocalSongsList();

	void parsePacketHost(const QTcpSocket * sender, const QByteArray data);
	void parsePacketClient(const QTcpSocket * sender, const QByteArray data);

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

	void songProgressHandler(qint64 ms);

	// Networking
	void newConnectionHandler();
	void incomingDataHandler();
};
