/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:		globals.h - A file that contains global definitions of variables and functions used throughout
--								the program.
--
-- PROGRAM:			CommAudio
--
--
-- FUNCTIONS:
--					inline QByteArray &operator<<(QByteArray &l, quint8 r)
--					inline QByteArray &operator<<(QByteArray &l, quint16 r)
--					inline QByteArray &operator<<(QByteArray &l, quint32 r)
--
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
--					Angus Lam
--					Roger Zhang
--
-- NOTES:
--					This is a central location for all the glboals that are used in the program.
----------------------------------------------------------------------------------------------------------------------*/
#pragma once

#define TITLE_DEFAULT	"CommAudio - No Session"
#define TITLE_HOST		"CommAudio - Hosting Session"
#define TITLE_CLIENT	"CommAudio - Connected To Host"

#define DEFAULT_PORT	42069
#define VOIP_PORT		42070
#define DOWNLOAD_PORT	42071
#define STREAM_PORT		42072

#define CONNECT_TIMEOUT 5 * 1000

#define USER_NAME_SIZE 33
#define KEY_SIZE 32
#define SONGNAME_SIZE 255

#define DOWNLOAD_CHUNCK_SIZE 8192
#define DOWNLOAD_TIMEOUT 5 * 1000

#define SUPPORTED_FORMATS { "*.wav" }

#include <QByteArray>

// Wave file header format
struct WavHeader
{
    char   id[4];            // should always contain "RIFF"
    int    totalLength;      // total file length minus 8
    char   wavFormat[8];     // should be "WAVEfmt "
    int    format;           // 16 for PCM format
    short  pcm;              // 1 for PCM format
    short  channels;         // channels
    int    sampleRate;       // sampling frequency
    int    bytesPerSecond;
    short  bytesByCapture;
    short  bitsPerSample;
    char   data[4];          // should always contain "data"
    int    bytesInData;
};

// Packet headers
enum Headers
{
	RequestToJoin,
	RespondToJoin,
	RespondWithName,
	AcceptJoin,
	RequestForSongs,
	RespondWithSongs,
	ReturnWithSongs,
	RequestAudioStream,
	RespondAudioStream,
	RequestDownload,
	RespondDownload,
	NotifyQuit
};

// Following functions from - https://stackoverflow.com/questions/30660127/append-quint16-unsigned-short-to-qbytearray-quickly

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		operator<<
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Matteo Italia
--
-- PROGRAMMER:		Matteo Italia
--
-- INTERFACE:		operator<< (QByteArray& l, quint8 r)
--						QByteArray& l: The original QByteArray.
--						quint8 r: The number that will be appended.
--
-- RETURNS:			The new QByteArray reference.
--
-- NOTES:
--					Appends a quint8 as a byte to the QByteArray and returns the new QByteArray.
----------------------------------------------------------------------------------------------------------------------*/
inline QByteArray& operator<<(QByteArray& l, quint8 r)
{
	l.append(r);
	return l;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		operator<<
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Matteo Italia
--
-- PROGRAMMER:		Matteo Italia
--
-- INTERFACE:		operator<< (QByteArray& l, quint16 r)
--						QByteArray& l: The original QByteArray.
--						quint16 r: The number that will be appended.
--
-- RETURNS:			The new QByteArray reference.
--
-- NOTES:
--					Appends a quint16 as two bytes to the QByteArray and returns the new QByteArray.
----------------------------------------------------------------------------------------------------------------------*/
inline QByteArray& operator<<(QByteArray& l, quint16 r)
{
	return l << quint8(r >> 8) << quint8(r);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:		operator<<
--
-- DATE:			March 26, 2018
--
-- REVISIONS:		N/A	
--
-- DESIGNER:		Matteo Italia
--
-- PROGRAMMER:		Matteo Italia
--
-- INTERFACE:		operator<< (QByteArray& l, quint32 r)
--						QByteArray& l: The original QByteArray.
--						quint32 r: The number that will be appended.
--
-- RETURNS:			The new QByteArray reference.
--
-- NOTES:
--					Appends a quint32 as four bytes to the QByteArray and returns the new QByteArray.
----------------------------------------------------------------------------------------------------------------------*/
inline QByteArray& operator<<(QByteArray& l, quint32 r)
{
	return l << quint16(r >> 16) << quint16(r);
}