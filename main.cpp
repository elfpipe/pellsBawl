#include <QApplication>
// #include <QSoundEffect>

#include "level1.h"
#include "title.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // QSoundEffect fx;
    // fx.setSource(QUrl("qrc:/assets/pellsBawl_intro_jingle.wav"));
    // fx.setLoopCount(1);         // or QSoundEffect::Infinite
    // fx.setVolume(0.35f);

    QScreen *screen = QApplication::screens().size() > 1 ? QApplication::screens().at(1) : QApplication::primaryScreen();

    Title title(screen, QPixmap(":/assets/intro/splash.png"));
    title.show();
    title.activateWindow();

    Level1 w;
    w.setWindowTitle("Simple Platformer");
    w.setScreen(screen);

    title.waitForMouseClick();
    title.finish(&w);

    w.showFullScreen();

    return a.exec();
}
