#ifndef LEVEL1_H
#define LEVEL1_H

#include <QOpenGLWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QList>

#include "commander.h"
#include "pellsBawl.h"
#include "fighterAI.h"
#include "joystick.h"

struct Enemy {
    QRect rect;
    bool isDefeated;
    bool movingLeft;
    int velocityX;
    QPixmap alivePixmap;
    QPixmap defeatedPixmap;

    Enemy(int x, int y, int w, int h)
        : rect(x, y, w, h), isDefeated(false), movingLeft(true), velocityX(2) {
        // Load the enemy images
        alivePixmap.load(":/assets/testlevel/enemy_sad.png");   // Image when alive
        defeatedPixmap.load(":/assets/testlevel/enemy_happy.png"); // Image when defeated
        
    }

    void move(const QRect &platform) {
        // Move the enemy left or right within the platform bounds
        if (movingLeft) {
            rect.moveLeft(rect.left() - velocityX);
            if (rect.left() <= platform.left()) { // If at the left edge of the platform
                movingLeft = false;
            }
        } else {
            rect.moveLeft(rect.left() + velocityX);
            if (rect.right() >= platform.right()) { // If at the right edge of the platform
                movingLeft = true;
            }
        }
    }
};

struct ParallaxLayer {
    QPixmap image;
    QPointF off = {0.0, 0.0}, rate = {1.0,1.0};
    double scale = 1.0;
    int z = -2;
};

#define USE_OPENGL 1

class Level1
#ifdef USE_OPENGL
: public QOpenGLWidget
#else
: public QWidget
#endif
{
    Q_OBJECT

public:
    Level1(QWidget *parent = nullptr);
    ~Level1();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    // void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override; // Slot for handling the timer event

private:
    void loadWorld(const QString &file, const QString &path);
    void doFighterSense(double dt);
    void checkEnemyCollisions();
    void doScrolling(double dt);
    void drawAnimationLayer(QPainter &p, ParallaxLayer &l, QPointF &scrollOffset);
// private:
//     void mt2KeyPress(QKeyEvent *event);
//     void mt2TestKeyPress(QKeyEvent *event);
//     void pellsBawlKeyPress(QKeyEvent *event);
//     void mt2KeyRelease(QKeyEvent *event);
//     void mt2TestKeyRelease(QKeyEvent *event);
//     void pellsBawlKeyRelease(QKeyEvent *event);

//     static int keyControl;

// public slots:
//     void onCookieTarget(BezierThrowWidget *btw);

private:

    PellsBawl pellsBawl;
    QList<Enemy> enemies;

    QList<Shape> shapes;
    QList<Image> images;
    QRectF world = {0, 0, 1800, 1200};
    QRectF window = {0, 0, 800, 600};
    QRectF bounds = {0, 0, 800, 600};
    qint32 ground = 550;

    qreal scrollAcc = 0.01;

    QList<ParallaxLayer> animLayers;

    Fighter *fighter;
    FighterAI *fighterAI;

    IFighterCommander *enemyCommander;

    IJoystickCommander *joyCommander;
    Joystick joystick;

    QElapsedTimer m_clock;
    qint64 m_lastMs = 0;           // for dt calc
};

#endif // LEVEL1_H
