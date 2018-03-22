#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CommAudio.h"

#include "ConnectionManager.h"

class CommAudio : public QMainWindow
{
	Q_OBJECT

public:
	CommAudio(QWidget *parent = Q_NULLPTR);

private:
	Ui::CommAudioClass ui;
};
