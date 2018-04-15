#pragma once

#include <QTimer>
#include <QWidget>

class SocketTimer : public QTimer
{
	Q_OBJECT
public:
	inline SocketTimer(QWidget * parent = nullptr)
		: QTimer(parent)
	{
	}

	virtual ~SocketTimer() = default;

	quint32 address;
};
