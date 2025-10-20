#ifndef PLATFORMER_H
#define PLATFORMER_H

#include <QOpenGLWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QList>

#include "commander.h"
#include "pellsBawl.h"
#include "fighterAI.h"
#include "combo.h"

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
        alivePixmap.load(":/assets/enemy_sad.png");   // Image when alive
        defeatedPixmap.load(":/assets/enemy_happy.png"); // Image when defeated
        
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

#define USE_OPENGL 1

class Platformer
#ifdef USE_OPENGL
: public QOpenGLWidget
#else
: public QWidget
#endif
{
    Q_OBJECT

public:
    Platformer(QWidget *parent = nullptr);
    ~Platformer();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    // void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override; // Slot for handling the timer event

private:
    void updatePlayerPosition();
    void checkCollisions();
    void loadPlatforms(const QString &filePath);
    void checkEnemyCollisions();

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

    // QPixmap playerPixmap;
    PellsBawl pellsBawl;
    QList<Enemy> enemies;

    QList<Platform> platforms;
    QRect world;
    QRect bounds;
    qint32 ground;

    Fighter *fighter;
    FighterAI *fighterAI;

    IJoystickCommander *joyCommander;
    IFighterCommander *enemyCommander;

    Combo combo;

    QElapsedTimer m_clock;
    qint64 m_lastMs = 0;           // for dt calc
};

#endif // PLATFORMER_H
