#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWidget>
#include <Qbuffer.h>

#include "globals.h"
#include "SocketTimer.h"
#include "MediaPlayer.h"

class StreamManager : public QWidget
{
	Q_OBJECT

public:
	StreamManager(const QByteArray * key, QDir * source, QDir * downloads, QWidget * parent = nullptr);
	~StreamManager() = default;

	MediaPlayer * mMediaPlayer;

private:
	const QByteArray * mKey;

	QDir * mSource;
	QDir * mDownloads;
	QAudioFormat mFormat;

	quint32 mSongSource;
	QMap<quint32, QTcpSocket *> mConnections;

	QTcpServer mServer;

	void uploadSong(QByteArray data, QTcpSocket * socket);

private slots:
	void newConnectionHandler();
	void incomingDataHandler();
	void disconnectHandler();

public slots:
	void StreamSong(QString songName, quint32 address);

};

