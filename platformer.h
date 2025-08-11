#ifndef PLATFORMER_H
#define PLATFORMER_H

#include <QOpenGLWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QList>

#include "fluffyWalk.h"

#include <proto/exec.h>

struct Platform {
    QRect rect;
    bool isGround;

    Platform(int x, int y, int w, int h, bool ground = false)
        : rect(x, y, w, h), isGround(ground) {}
};

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
        IExec->DebugPrintF("move\n");
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
    void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

public slots:
    void doTimerEvent(); // Slot for handling the timer event

private:
    void updatePlayerPosition();
    void checkCollisions();
    void loadPlatforms(const QString &filePath);
void checkEnemyCollisions();

    // QPixmap playerPixmap;
    WalkerAnim playerAnim;
    QRect playerRect;
    QList<Platform> platforms;
    QList<Enemy> enemies;



    bool isJumping;
    bool isMovingLeft;
    bool isMovingRight;
    bool isFalling;

    int jumpVelocity;
    int gravity;
    int velocityX;
    int velocityY;

    QTimer *gameTimer;
};

#endif // PLATFORMER_H
