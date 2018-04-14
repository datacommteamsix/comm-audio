#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWidget>

class DownloadManager : public QWidget
{
	Q_OBJECT
public:
	DownloadManager(QDir * source, QDir * downloads, QWidget * parent = nullptr);
	~DownloadManager() = default;

private:
	QDir * mSource;
	QDir * mDownloads;

public slots:
	void DownloadFile(QString songName, quint32 address);
};