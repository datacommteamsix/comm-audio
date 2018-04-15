#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWidget>

#include "globals.h"

class DownloadManager : public QWidget
{
	Q_OBJECT

public:
	DownloadManager(QDir * source, QDir * downloads, QWidget * parent = nullptr);
	~DownloadManager() = default;

private:
	QDir * mSource;
	QDir * mDownloads;

	QMap<quint32, QTcpSocket *> mConnections;
	QMap<quint32, QFile> mFiles;

	QTcpServer mServer;

	void uploadSong(QByteArray data, QTcpSocket * socket);
	void writeToFile(QByteArray data, quint32 address);

private slots:
	void newConnectionHandler();
	void incomingDataHandler();
	void disconnectHandler();

public slots:
	void DownloadFile(QString songName, quint32 address);

};