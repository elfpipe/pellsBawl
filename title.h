#ifndef TITLE_H
#define TITLE_H

#include "qevent.h"
#include <QEventLoop>
#include <QSplashScreen>
#include <QMediaPlayer>
#include <QAudioOutput>

class Title : public QSplashScreen {
    Q_OBJECT
public:
    Title(QScreen *screen = nullptr, QPixmap pixmap = QPixmap());
    void stopJingle() { player->stop(); }

public:
    void waitForMouseClick() {
        QEventLoop loop;
        connect(this, &Title::userClick, &loop, &QEventLoop::quit);
        loop.exec();  // Blocks until quit() is called
    }

signals:
    void userClick();

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit userClick();
        }
    }
protected:
    void keyPressEvent(QKeyEvent */*event*/) override {
            emit userClick();
    }
private:
    QMediaPlayer* player;
    QAudioOutput* audio;
};

#endif // TITLE_H
