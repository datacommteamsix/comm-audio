#pragma once

#define SUPPORTED_FORMATS { "*.wav" }

#include <QByteArray>

enum Headers
{
	RequestToJoin,
	RespondToJoin,
	AcceptJoin,
	RequestForSongs,
	RespondWithSongs,
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