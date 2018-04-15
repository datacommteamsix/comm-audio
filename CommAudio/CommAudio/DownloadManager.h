#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWidget>

#include "globals.h"
#include "SocketTimer.h"


class DownloadManager : public QWidget
{
	Q_OBJECT

public:
	DownloadManager(const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent = nullptr);
	~DownloadManager() = default;

private:
	const QByteArray * mKey;

	QDir * mSource;
	QDir * mDownloads;

	QMap<quint32, QTcpSocket *> mConnections;
	QMap<quint32, SocketTimer *> mTimers;
	QMap<quint32, QFile *> mFiles;

	QTcpServer mServer;

	int i;

	void uploadSong(QByteArray data, QTcpSocket * socket);
	void writeToFile(QByteArray data, quint32 address);

private slots:
	void newConnectionHandler();
	void incomingDataHandler();
	void disconnectHandler();

	void timeoutHandler();

public slots:
	void DownloadFile(QString songName, quint32 address);

};

