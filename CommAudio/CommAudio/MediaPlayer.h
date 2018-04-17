#pragma once

#include <QAudioFormat>
#include <QAudioOutput>
#include <QFile>
#include <QDir>
#include <QTcpSocket>
#include <QWidget>

#include "globals.h"
#include "ui_CommAudio.h"

class MediaPlayer : public QWidget
{
	Q_OBJECT

public:
	enum PlayerState
	{
		PlayingState,
		StoppedState
	};

	MediaPlayer(Ui::CommAudioClass * ui, QWidget * parent = nullptr);
	~MediaPlayer() = default;

	void SetSong(QString absoluteFileName);
	void StartStream(QIODevice * stream);
	void SetDirAndSong(QDir songDir, QTreeWidgetItem *currSong);
	void UpdateSongList(QList<QTreeWidgetItem *> songList);

	void Play();
	void Pause();
	void Stop();
	PlayerState State();

	int GetDuration();

private:
	enum SourceType
	{
		Song,
		Stream
	};

	Ui::CommAudioClass * ui;

	PlayerState mState;
	SourceType mSourceType;

	// Song variables
	WavHeader * mSongHeader;
	QAudioFormat * mSongFormat;
	QFile * mSong;
	QIODevice * mStream;
	QTreeWidgetItem * mCurrentSong = NULL;

	QAudioOutput * mPlayer;
	QList<QTreeWidgetItem *> songList;
	QDir mCurrentDir = NULL;

private slots:
	void playSongButtonHandler();
	void prevSongButtonHandler();
	void nextSongButtonHandler();

	void changeVolumeHandler(int volume);

	void seekPositionHandler(int position);

	void songStateChangeHandler(QAudio::State state);
	void songProgressHandler();
};
