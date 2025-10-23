#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>

#include "title.h"

Title::Title(QScreen *screen, QPixmap pixmap) : QSplashScreen(screen, pixmap) {
    audio = new QAudioOutput(this);
    player = new QMediaPlayer(this);
    player->setAudioOutput(audio);

    // Local file or qrc (see below)
    player->setSource(QUrl("qrc:/assets/intro/pellsBawl_intro_jingle.wav"));
    audio->setVolume(0.8);                 // 0.0 .. 1.0
    player->setLoops(QMediaPlayer::Once);  // or QMediaPlayer::Infinite for bg music
    player->play();
}
