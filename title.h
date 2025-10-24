#ifndef TITLE_H
#define TITLE_H

#include "qevent.h"
#include <QEventLoop>
#include <QSplashScreen>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPainter>
#include "Game.h"

class Title : public QObject {
    Q_OBJECT
public:
    Title(Game *parent = nullptr);
    void playJingle(QString jingle = QString());

signals:

private:
    QMediaPlayer* player;
    QAudioOutput* audio;
};

#endif // TITLE_H
