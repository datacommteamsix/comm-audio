#pragma once

#include <QDebug>

#include <QAction>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QNetworkInterface>
#include <QPushButton>
#include <QString>
#include <QTcpSocket>
#include <QTcpServer>

#include <QtWidgets/QMainWindow>
#include "ui_CommAudio.h"

class CommAudio : public QMainWindow
{
	Q_OBJECT

public:
	CommAudio(QWidget *parent = Q_NULLPTR);

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

	QDir mSharedFolder;
	QDir mDownloadFolder;

	QString mLocalIp;
	QByteArray mSessionKey;

	QString getLocalIp();
	QTcpServer mServer;
	QMap<QString, QTcpSocket *> mConnectins;

private slots:
	// UI
	void hostSessionHandler();
	void joinSessionHandler();
	void leaveSessionHandler();

	void changeSongFolderHandler();
	void changeDownloadFolderHandler();

	void playSongButtonHandler();
	void prevSongButtonHandler();
	void nextSongButtonHandler();

	// Networking
	void newConnectionHandler();
	void incomingDataHandler();
};
