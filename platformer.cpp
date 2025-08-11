#include "platformer.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "fluffyWalk.h"

Platformer::Platformer(QWidget *parent)
#ifdef USE_OPENGL
: QOpenGLWidget(parent)
#else
: QWidget(parent)
#endif
, isJumping(false), isMovingLeft(false), isMovingRight(false), isFalling(true), jumpVelocity(15), gravity(1), velocityX(0), velocityY(0) {
    setFixedSize(800, 600);
    // playerPixmap.load(":/assets/character.png"); // Load the character image
    playerRect = QRect(100, 500, 50, 50); // Initial position of the player

    // Load platforms from a JSON file
    loadPlatforms(":/assets/platforms.json");

    // Create enemies and assign them to platforms
    enemies.append(Enemy(110, 412, 40, 40));  // Example enemy 1 on platform
    enemies.append(Enemy(310, 312, 40, 40));  // Example enemy 2 on platform

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &Platformer::doTimerEvent); // Connect the timer to the doTimerEvent slot
    gameTimer->start(16); // Approximately 60 FPS
}

Platformer::~Platformer() {}

void Platformer::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Left) {
        isMovingLeft = true;
        turningLeft = true;
    }
    if (event->key() == Qt::Key_Right) {
        isMovingRight = true;
        turningLeft = false;
    }
    if (event->key() == Qt::Key_Space && !isJumping) {
        isJumping = true;
        velocityY = -jumpVelocity;
    }
    if (event->key() == Qt::Key_Escape) close();
}

void Platformer::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Left) {
        isMovingLeft = false;
    }
    if (event->key() == Qt::Key_Right) {
        isMovingRight = false;
    }
}

void Platformer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
#ifdef USE_OPENGL
    //cls - this shouldn't be necessary, is probably an amiga thing
    painter.setBrush(Qt::white);
    painter.drawRect(rect());
#endif

    painter.setBrush(Qt::blue);

    // Draw platforms
    for (const Platform &platform : platforms) {
        painter.drawRect(platform.rect); // Draw each platform
    }

    // Draw enemies with the correct animation based on their state
    for (const Enemy &enemy : enemies) {
        if (!enemy.isDefeated) {
            painter.drawPixmap(enemy.rect, enemy.alivePixmap); // Draw enemy when alive
        } else {
            painter.drawPixmap(enemy.rect, enemy.defeatedPixmap); // Draw enemy when defeated
        }
    }

    // Draw player
    // painter.drawPixmap(playerRect, playerPixmap); // Draw player character
    playerAnim.paintWalker(painter, playerRect, turningLeft, qAbs(velocityX) * 0.5); // Draw player character
    playerAnim.drawShadow(painter, QPointF(playerRect.left() + playerRect.width() / 2, 562), QSizeF(120, 17), 0.35);
}

void Platformer::checkEnemyCollisions() {
    for (Enemy &enemy : enemies) {
        if (playerRect.intersects(enemy.rect)) {
            if (!enemy.isDefeated) {
                // If the player collides with an enemy, mark it as defeated
                enemy.isDefeated = true;
            }
        }

        // Move the enemy based on its platform
        for (const Platform &platform : platforms) {
            if (platform.rect.intersects(enemy.rect)) {
                enemy.move(platform.rect);  // Move the enemy within its platform
            }
        }
    }
}

void Platformer::doTimerEvent() {
    updatePlayerPosition();
    // move enemies
    checkCollisions();
    checkEnemyCollisions();
    update(); // Repaint the widget
}

void Platformer::updatePlayerPosition() {
    // Handle horizontal movement
    if (isMovingLeft) {
        velocityX -= 0.1; // velocityX == 0 ? -.5 : (velocityX < -4.0 ? velocityX : (qAbs(velocityX) > 2.0 ? velocityX - 0.5 : velocityX - .2));
    } else if (isMovingRight) {
        velocityX += 0.1; //velocityX == 0 ? .5 : (velocityX > 4.0 ? velocityX : (qAbs(velocityX) > 2.0 ? velocityX + 0.5 : velocityX + .2));
    } else {
        velocityX *= 0.9;
    }

    // friction - this could be done better
    // velocityX *= 0.98;

    // Apply gravity and jump velocity
    if (isJumping || isFalling) {
        velocityY += gravity;
    }
    if(velocityY > 0) isFalling = true; else isFalling = false;

    // Update player position
    playerRect.moveLeft(playerRect.left() + velocityX);
    playerRect.moveTop(playerRect.top() + velocityY);
}

void Platformer::checkCollisions() {
    bool onGround = false;
    for (const Platform &platform : platforms) {
        if (playerRect.intersects(platform.rect)) {
            // Collision with ground or other platform
            // if (playerRect.bottom() <= platform.rect.top() && playerRect.bottom() >= platform.rect.top() - velocityY) {
            //     playerRect.moveBottom(platform.rect.top());
            if (isFalling) {
                isJumping = false;
                isFalling = false;
                velocityY = 0;
                onGround = true;
                playerRect.moveBottom(platform.rect.top());
            }
        }
    }

    if (!onGround) {
        isFalling = true;
    }

    QRectF sceneRect = rect();
    if (!sceneRect.contains(playerRect)) {
        playerRect.moveTop(qMax(sceneRect.top(), playerRect.top()));
        playerRect.moveBottom(qMin(sceneRect.bottom(), playerRect.bottom()));
        playerRect.moveLeft(qMax(sceneRect.left(), playerRect.left()));
        playerRect.moveRight(qMin(sceneRect.right(), playerRect.right()));
    }
}

void Platformer::loadPlatforms(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open platforms file.");
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray platformArray = doc.object()["platforms"].toArray();

    // Ground platform
    platforms.append(Platform(0, height() - 50, width(), 50, true));

    // Read platforms from the JSON file
    for (const QJsonValue &value : platformArray) {
        QJsonObject obj = value.toObject();
        int x = obj["x"].toInt();
        int y = obj["y"].toInt();
        int w = obj["width"].toInt();
        int h = obj["height"].toInt();
        platforms.append(Platform(x, y, w, h));
    }
}
