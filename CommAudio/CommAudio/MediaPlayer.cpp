#include "MediaPlayer.h"

MediaPlayer::MediaPlayer(Ui::CommAudioClass * ui, QWidget * parent)
	: ui(ui)
	, mSongHeader(new WavHeader)
	, mSongFormat(new QAudioFormat())
	, mSong(new QFile())
	, mState(PlayerState::StoppedState)
{
	memset(mSongHeader, 0, sizeof(WavHeader));

	mSongFormat->setSampleRate(44100);
	mSongFormat->setSampleSize(16);
	mSongFormat->setChannelCount(2);
	mSongFormat->setCodec("audio/pcm");
	mSongFormat->setByteOrder(QAudioFormat::LittleEndian);
	mSongFormat->setSampleType(QAudioFormat::SignedInt);

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

MediaPlayer::~MediaPlayer()
{
	delete mSongHeader;
	delete mSongFormat;
	delete mSong;
}

// Roger
void MediaPlayer::SetDirAndSong(QDir songDir, QTreeWidgetItem *currSong)
{
	mCurrentDir = songDir;
	mCurrentSong = currSong;
}

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

void MediaPlayer::StartStream(QTcpSocket * socket)
{
	mPlayer->stop();
	mPlayer->start(socket);
	mState = PlayerState::PlayingState;
	mSourceType = SourceType::Stream;
}

void MediaPlayer::Play()
{
	if (mSong != nullptr)
	{
		mPlayer->start(mSong);
		mState = PlayerState::PlayingState;
		mSourceType = SourceType::Song;
	}
}

void MediaPlayer::Pause()
{
	mPlayer->suspend();
	mState = PlayerState::StoppedState;
}

void MediaPlayer::Stop()
{
	mPlayer->stop();
	mSong->seek(0);
	mState = PlayerState::StoppedState;
}

MediaPlayer::PlayerState MediaPlayer::State()
{
	return mState;
}

int MediaPlayer::GetDuration()
{
	if (mSourceType == SourceType::Stream)
	{
		return 1;
	}

	return mSongHeader->totalLength / mSongHeader->bytesPerSecond;
}

void MediaPlayer::changeVolumeHandler(int position)
{
	double volume = (double)position / (double)100;
	mPlayer->setVolume(volume);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		MediaPlayer::playSongButtonHandler
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
-- INTERFACE:		MediaPlayer::playSongButtonHandler ()
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user pressed the play/pause button.
--
-- The state of the media player is changed based on the current state of the media player.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::playSongButtonHandler()
{
	switch (mPlayer->state())
	{
	case QAudio::ActiveState:
		Pause();
		break;
	case QAudio::StoppedState:
		break;
	case QAudio::SuspendedState:
		Play();
		break;
	default:
		break;
	}
}

//Roger
void MediaPlayer::prevSongButtonHandler()
{
	int index = songList.indexOf(mCurrentSong);
	index -= 1;
	if (index < 0) { index = songList.size() - 1; }
	mCurrentSong = songList[index];
	SetSong(mCurrentDir.absoluteFilePath(mCurrentSong->text(0)));
	Play();
}

//Roger
void MediaPlayer::nextSongButtonHandler()
{
	int index = songList.indexOf(mCurrentSong);
	index += 1;
	if (index > songList.size() - 1) { index = 0; }
	mCurrentSong = songList[index];
	SetSong(mCurrentDir.absoluteFilePath(mCurrentSong->text(0)));
	Play();
}

//Roger
void MediaPlayer::UpdateSongList(QList<QTreeWidgetItem *> items)
{
	songList = items;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		MediaPlayer::seekPositionHandler
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
-- INTERFACE:		MediaPlayer::seekPositionHandler (int position)
--						int position: The new position in the song.
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the user moves the position slider for the song.
--
-- This function will cause the QMediaPlayer to seek to the new position in the song.
----------------------------------------------------------------------------------------------------------------------*/
void MediaPlayer::seekPositionHandler(int position)
{
	if (mSourceType == SourceType::Song)
	{
		mSong->seek(position * mSongHeader->bytesPerSecond);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		MediaPlayer::songStateChangedHandler
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
-- INTERFACE:		MediaPlayer::songStateChangedHandler (QMediaPLayer::State state)
--						QMediaPlayer::State state: The state of the media player.
--
-- RETURNS:			void.		
--
-- NOTES:
-- This is a Qt slot that is triggered when the state of the media player changes.
--
-- Elements of the GUI that are tied to the state of the media player will be updated to the new state.
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