#include <DownloadManager.h>

DownloadManager::DownloadManager(QDir * source, QDir * downloads, QWidget * parent)
	: QWidget(parent)
	, mSource(source)
	, mDownloads(downloads)
{

}

void DownloadManager::DownloadFile(QString songName, quint32 address)
{
	
}