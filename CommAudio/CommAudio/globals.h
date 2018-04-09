#pragma once

#define TEST_HOST_IP "192.168.0.20"
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
	RequestDownload,
	RequestUpload,
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