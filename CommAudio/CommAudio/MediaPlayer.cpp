/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		MediaPlayer.cpp - An wrapper for QAudioOutput that adds common audio player functionality.
--
-- PROGRAM:			CommAudio
--
-- FUNCTIONS:
--					MediaPlayer(Ui::CommAudioClass * ui, QWidget * parent = nullptr)
--					~MediaPlayer()
--					void SetSong(QString absoluteFileName)
--					void StartStream(QIODevice * stream)
--					void SetDirAndSong(QDir songDir, QTreeWidgetItem *currSong)
--					void UpdateSongList(QList<QTreeWidgetItem *> songList)
--					void Play()
--					void Pause()
--					void Stop()
--					PlayerState State()
--					int GetDuration()
--					void playSongButtonHandler()
--					void prevSongButtonHandler()
--					void nextSongButtonHandler()
--					void changeVolumeHandler(int volume)
--					void seekPositionHandler(int position)
--					void songStateChangeHandler(QAudio::State state)
--					void songProgressHandler()
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- NOTES:
--					This is a class that encapsulates the QAudioInput with other media player functions.
----------------------------------------------------------------------------------------------------------------------*/
#include "MediaPlayer.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		MediaPlayer
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--					Angus Lam
--					Benny Wang
--
-- INTERFACE:		MediaPlayer (Ui::CommAudioClass * ui, QWidget * parent)
--						Ui::CommAudioClass * ui: A reference Qt ui object.
--						QWidget * parent: A reference to the QWidget parent.
--
-- RETURNS:			N/A
--
-- NOTES:
--					This is the cosntructor for the MediaPlayer. The default audio format and UI buttons are set here.
----------------------------------------------------------------------------------------------------------------------*/
MediaPlayer::MediaPlayer(Ui::CommAudioClass * ui, QWidget * parent)
	: ui(ui)
	, mSongHeader(new WavHeader)
	, mSongFormat(new QAudioFormat())
	, mSong(new QFile())
	, mState(PlayerState::StoppedState)
	, mStream(nullptr)
{
	memset(mSongHeader, 0, sizeof(WavHeader));

	mSongFormat->setSampleRate(44100);
	mSongFormat->setSampleSize(16);
	mSongFormat->setChannelCount(2);
	mSongFormat->setCodec("audio/pcm");
	mSongFormat->setByteOrder(QAudioFormat::LittleEndian);
	mSongFormat->setSampleType(QAudioFormat::SignedInt);

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(*mSongFormat)) {
		qWarning() << "Raw audio format not supported by backend, cannot play audio.";
		return;
	}

	mPlayer = new QAudioOutput(*mSongFormat, this);
	mPlayer->setNotifyInterval(1000);
	mPlayer->setVolume(1);

	// Configure the media player
	// Set volume
	connect(ui->sliderVolume, &QSlider::sliderMoved, this, &MediaPlayer::changeVolumeHandler);
	// Song state changed
	connect(mPlayer, &QAudioOutput::stateChanged, this, &MediaPlayer::songStateChangeHandler);
	// Song progress
	connect(mPlayer, &QAudioOutput::notify, this, &MediaPlayer::songProgressHandler);

	// Audio Control Buttons
	connect(ui->btnPlaySong, &QPushButton::pressed, this, &MediaPlayer::playSongButtonHandler);
	connect(ui->btnPrevSong, &QPushButton::pressed, this, &MediaPlayer::prevSongButtonHandler);
	connect(ui->btnNextSong, &QPushButton::pressed, this, &MediaPlayer::nextSongButtonHandler);

	// Seeking
	connect(ui->sliderProgress, &QSlider::sliderMoved, this, &MediaPlayer::seekPositionHandler);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		SetDirAndSong
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		SetDirAndSong (QDir songDir, QTreeWidgetItem *currSong)
--						QDir songDir: The directory where the local songs are.
--						QTreeWidgetItem *currSong: The current song that is selected.
--
-- RETURNS:			N/A
--
-- NOTES:
--					A setter method that sets both the current directory and current song.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::SetDirAndSong(QDir songDir, QTreeWidgetItem *currSong)
{
	mCurrentDir = songDir;
	mCurrentSong = currSong;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		SetSong
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		SetSong (QString absoluteFilename)
--						QString absoluteFilename: The absolte file name for the song.
--
-- RETURNS:			N/A
--
-- NOTES:
--					Loads the song for the MediaPlayer to play when the play button is pressed.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::SetSong(QString absoluteFilename)
{
	Stop();

	mSong->close();
	delete mSong;

	mSong = new QFile(absoluteFilename);
	mSong->open(QFile::ReadOnly);

	// Grab song header
	mSong->read((char *)mSongHeader, sizeof(WavHeader));
	mSong->seek(0);

	// Grab song format
	mSongFormat->setSampleRate(mSongHeader->sampleRate);
	mSongFormat->setChannelCount(mSongHeader->channels);
	mSongFormat->setSampleSize(mSongHeader->bitsPerSample);
	if (mSongHeader->pcm)
	{
		mSongFormat->setCodec("audio/pcm");
	}
	mSongFormat->setByteOrder(QAudioFormat::LittleEndian);
	mSongFormat->setSampleType(QAudioFormat::UnSignedInt);

	// Change the label
	qint64 totalSeconds = GetDuration();
	qint64 seconds = totalSeconds % 60;;
	qint64 minutes = (totalSeconds - (totalSeconds % 60)) / 60;
	QString totalTimeText = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));

	ui->labelCurrentSong->setText("Currently Playing: " + mSong->fileName());
	ui->labelCurrentTime->setText("00:00");
	ui->labelTotalTime->setText(totalTimeText);

	// Change slider max
	ui->sliderProgress->setMaximum(totalSeconds);
	ui->sliderProgress->setSliderPosition(0);

	mSourceType = SourceType::Song;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		StartStream
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		StartStream (QIODevice * stream)
--						QIODevice * stream: The stream of the song.
--
-- RETURNS:			N/A
--
-- NOTES:
--					Starts playing the stream of the song that was requested by the user.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::StartStream(QIODevice * stream)
{
	mStream = stream;
	mPlayer->stop();
	mPlayer->start(mStream);
	mState = PlayerState::PlayingState;
	mSourceType = SourceType::Stream;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		Play
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		Play ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					Starts playing the song that is loading by SetSong().
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::Play()
{
	if (mSong->isOpen())
	{
		mPlayer->start(mSong);
		mState = PlayerState::PlayingState;
		mSourceType = SourceType::Song;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		Pause
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		Pause ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					If a song is being played, the song is pause in its current position. If a stream is being played,
--					the stream is stopped.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::Pause()
{
	if (mSourceType == SourceType::Song)
	{
		mPlayer->suspend();
		mState = PlayerState::StoppedState;
	}
	else 
	{
		mPlayer->stop();
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		Stop
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		Stop ()
--
-- RETURNS:			N/A
--
-- NOTES:
--					If a song is being played, the song is stopped and the position is set back at 0. If a stream is
--					being played, the stream is closed and its memory is cleaned up.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::Stop()
{
	mPlayer->stop();
	mState = PlayerState::StoppedState;

	if (mSourceType == SourceType::Song)
	{
		mSong->seek(0);
	}
	else
	{
		mStream->close();
		delete mStream;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		State
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		State ()
--
-- RETURNS:			The state of the MediaPlayer as a Media::PlayerState enum.
--
-- NOTES:
--					Returns the current state of the MediaPlayer.
----------------------------------------------------------------------------------------------------------------------*/
MediaPlayer::PlayerState MediaPlayer::State()
{
	return mState;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		GetDuration
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		GetDuration ()
--
-- RETURNS:			The duration of the current song.
--
-- NOTES:
--					If a song is being played, gets the duration of the current song, otherwise 1.
----------------------------------------------------------------------------------------------------------------------*/
int MediaPlayer::GetDuration()
{
	if (mSourceType == SourceType::Stream)
	{
		return 1;
	}

	return mSongHeader->totalLength / mSongHeader->bytesPerSecond;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		changeVolumeHandler
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		changeVolumeHandler ()
--
-- RETURN:			void
--
-- NOTES:
--					This is a Qt slot that is triggered when the user drags the volume slider. The volume is set to
--					the position of the volume slider.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::changeVolumeHandler(int position)
{
	double volume = (double)position / (double)100;
	mPlayer->setVolume(volume);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		playSongButtonHandler
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		playSongButtonHandler ()
--
-- RETURN:			void
--
-- NOTES:
--					This is a Qt slot that is triggerd when the user pressed the play button. If the player is stopped
--					the player is then started. Likewise, if the player is playing then the player is stopped.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::playSongButtonHandler()
{
	switch (mPlayer->state())
	{
	case QAudio::ActiveState:
		Pause();
		break;
	case QAudio::StoppedState:
	case QAudio::SuspendedState:
		Play();
		break;
	default:
		break;
	}
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		prevSongButtonHandler
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		prevSongButtonHandler ()
--
-- RETURN:			void
--
-- NOTES:
--					This is a Qt slot that is triggered when the user pressed the previous song button. If there is a
--					previous song to the currently loaded source it would be set as the current source, otherwise this
--					function does nothing.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::prevSongButtonHandler()
{
	if (songList.size() == 0)
	{
		return;
	}

	int index = songList.indexOf(mCurrentSong);
	index -= 1;
	if (index < 0) { index = songList.size() - 1; }
	mCurrentSong = songList[index];
	SetSong(mCurrentDir.absoluteFilePath(mCurrentSong->text(0)));
	Play();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		nextSongButtonHandler
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		nextSongButtonHandler ()
--
-- RETURN:			void
--
-- NOTES:
--					This is a Qt slot that is trigged when the user pressed the nextSongButtonHandler. If there is a next
--					song to the currently loaded source it would be st as the current source, otherwise this function
--					does nothing.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::nextSongButtonHandler()
{
	if (songList.size() == 0)
	{
		return;
	}
	
	int index = songList.indexOf(mCurrentSong);
	index += 1;
	if (index > songList.size() - 1) { index = 0; }
	mCurrentSong = songList[index];
	SetSong(mCurrentDir.absoluteFilePath(mCurrentSong->text(0)));
	Play();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		UodateSongList
--
-- DATE:			April 14, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Roger Zhang
--
-- INTERFACE:		UpdateSongList (QList<QTreeWidgetItem *> items)
--						QList<QTreeWidgetItem *> items: The list of songs.
--
-- RETURN:			void
--
-- NOTES:
--					Updates the list of songs the user sees.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::UpdateSongList(QList<QTreeWidgetItem *> items)
{
	songList = items;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		seekPositionHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		seekPositionHandler (int position)
--						int position: The new position in the song.
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the user moves the position slider for the song. This 
--					function will cause the QMediaPlayer to seek to the new position in the song.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::seekPositionHandler(int position)
{
	if (mSourceType == SourceType::Song)
	{
		mSong->seek(position * mSongHeader->bytesPerSecond);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		songStateChangedHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		songStateChangedHandler (QMediaPLayer::State state)
--						QMediaPlayer::State state: The state of the media player.
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggered when the state of the media player changes. Elements of the
--					GUI that are tied to the state of the media player will be updated to the new state.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::songStateChangeHandler(QAudio::State state)
{
	switch (state)
	{
	case QAudio::ActiveState:
		ui->btnPlaySong->setText("Pause");
		break;
	case QAudio::IdleState:
		SetSong(mSong->fileName());
		Play();
		break;
	default:
		ui->btnPlaySong->setText("Play");
		break;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		:songStateChangedHandler
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Benny Wang
--					Angus Lam
--					Roger Zhang
--
-- PROGRAMMER:		Benny Wang
--
-- INTERFACE:		songProgressHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
--					This is a Qt slot that is triggerd when the song's progress changes. The slider that displays the
--					song's progress is updated as well as the text beside it that shows the current timestamp of the
--					song.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::songProgressHandler()
{
	if (mSourceType == SourceType::Stream)
	{
		ui->sliderProgress->setValue(0);
		ui->labelCurrentTime->setText("");
		return;
	}

	int progress = ui->sliderProgress->value() + 1;
	int maxDuration = GetDuration();

	if (progress > maxDuration)
	{
		progress = maxDuration;
	}

	// Set label text
	qint64 seconds = progress % 60;
	qint64 minutes = (progress - (progress % 60)) / 60;

	QString labelText = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
	ui->labelCurrentTime->setText(labelText);

	// Update slider
	ui->sliderProgress->setValue(progress);
}