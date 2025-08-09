#ifndef PLATFORMER_H
#define PLATFORMER_H

#include <QWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QList>

struct Platform {
    QRect rect;
    bool isGround;

    Platform(int x, int y, int w, int h, bool ground = false)
        : rect(x, y, w, h), isGround(ground) {}
};

class Platformer : public QWidget {
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

    QPixmap playerPixmap;
    QRect playerRect;
    QList<Platform> platforms;

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
