#pragma once

#include <QTcpSocket>
#include <QWidget>

class VoipModule : public QWidget
{
	Q_OBJECT

public:
	VoipModule(QWidget * parent = nullptr);
	~VoipModule() = default;

public slots:
	void newClientHandler(QTcpSocket * socket);
};
