#pragma once

#define DEFAULT_PORT	42069
#define VOIP_PORT		42070
#define DOWNLOAD_PORT	42071
#define STREAM_PORT		42072

#define CONNECT_TIMEOUT 5 * 1000

#define KEY_SIZE 32
#define SONGNAME_SIZE 255

#define DOWNLOAD_CHUNCK_SIZE 8192
#define DOWNLOAD_TIMEOUT 5 * 1000

#define TEST_HOST_IP "192.168.0.16"
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
inline QByteArray &operator<<(QByteArray &l, quint8 r)
{
	l.append(r);
	return l;
}

inline QByteArray &operator<<(QByteArray &l, quint16 r)
{
	return l << quint8(r >> 8) << quint8(r);
}

inline QByteArray &operator<<(QByteArray &l, quint32 r)
{
	return l << quint16(r >> 16) << quint16(r);
}
// ---------------------------------------------------------------------------------------------------------------------------