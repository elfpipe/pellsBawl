#include "level1.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "pellsBawl.h"

#include <QDebug>


Level1::Level1(QWidget *parent)
#ifdef USE_OPENGL
: QOpenGLWidget(parent)
#else
: QWidget(parent)
#endif
{
    // Load platforms from a JSON file
    loadPlatforms(":/assets/platforms.json");

    fighter = new Fighter(this);
    fighter->loadFromJson(":/assets/mt2/mt2.json");

    joyCommander = new PellsBawlCommander(this, &pellsBawl);
    enemyCommander = new FighterCommander(this, fighter);

    fighterAI = new FighterAI(this);
    fighterAI->setCommander(enemyCommander);

    joystick.setCommander(joyCommander);

    // Create enemies and assign them to platforms
    enemies.append(Enemy(110, 412, 40, 40));  // Example enemy 1 on platform
    enemies.append(Enemy(310, 312, 40, 40));  // Example enemy 2 on platform

    // gameTimer = new QTimer(this);
    // connect(gameTimer, &QTimer::timeout, this, &Platformer::doTimerEvent); // Connect the timer to the doTimerEvent slot
    // gameTimer->start(16); // Approximately 60 FPS

    startTimer(1000/60); // ~60 FPS
    m_clock.start();

    m_lastMs = m_clock.elapsed();
}

Level1::~Level1() {}

// void Platformer::mt2KeyPress(QKeyEvent *event) {
//     // MT2
//     if (event->key() == Qt::Key_Left) {
//         combo.key(Combo::LEFT, m_lastMs);
//     }
//     if (event->key() == Qt::Key_Right) {
//         combo.key(Combo::RIGHT, m_lastMs);
//     }
//     if (event->key() == Qt::Key_Down) {
//         combo.key(Combo::DOWN, m_lastMs);
//     }
//     if (event->key() == Qt::Key_Up) {
//         combo.key(Combo::UP, m_lastMs);
//     }
//     if (event->key() == Qt::Key_Comma) {
//         combo.key(Combo::FIRE1, m_lastMs);
//     }
//     if (event->key() == Qt::Key_Period) {
//         combo.key(Combo::FIRE2, m_lastMs);
//     }

//     if(commander->applyCombo(combo.getCurrent())) combo.resetCurrent();
// }


// void Platformer::pellsBawlKeyPress(QKeyEvent *event) {
// }

// // void Platformer::onCookieTarget(BezierThrowWidget *btw) {
// //     shots.removeAll(btw);
// // }

// void Platformer::mt2KeyRelease(QKeyEvent *event) {
//     // MT2
//     if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right || event->key() == Qt::Key_Down) {
//         fighter->stop();
//     }
// }

// void Platformer::mt2TestKeyRelease(QKeyEvent */*event*/) {
// }

// void Platformer::pellsBawlKeyRelease(QKeyEvent *event) {
// }

// int Level1::keyControl = 1;

void Level1::keyPressEvent(QKeyEvent *event) {
//     if (event->isAutoRepeat()) {
//         // Ignore auto-repeated key press events
//         return;
//     }

//     if (event->key() == Qt::Key_1) keyControl = 1;
//     if (event->key() == Qt::Key_2) keyControl = 2;
//     if (event->key() == Qt::Key_3) keyControl = 3;

//     switch(keyControl) {
//     case 1: mt2KeyPress(event); break;
//     // case 2: mt2TestKeyPress(event); break;
//     case 3: pellsBawlKeyPress(event); break;
//     }

//     // Generic
//     if (event->key() == Qt::Key_G) {
//         qDebug() << "onGround : " << fighter->onGround();
//     }
    if (event->key() == Qt::Key_Escape) close();
}

// void Platformer::keyReleaseEvent(QKeyEvent *event) {
//     if (event->isAutoRepeat()) {
//         // Ignore auto-repeated key press events
//         return;
//     }

//     switch(keyControl) {
//     case 1: mt2KeyRelease(event); break;
//     case 2: mt2TestKeyRelease(event); break;
//     case 3: pellsBawlKeyRelease(event); break;
//     }
// }

void Level1::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    // Set the world rectangle (logical coordinates)
    painter.setWindow(world);

    // Optional: Set the viewport (physical coordinates)
    painter.setViewport(0, 0, width(), height()); //for now, otherwise 0, 0, width(), height()

    // Save state
    painter.save();

#ifdef USE_OPENGL
    //cls
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

    // This includes shots and shadow
    pellsBawl.paintWalker(painter, ground); //, playerRect, turningLeft, m_animTime); // Draw player character

    // Fighter (MT2)
    fighter->paint(&painter);

    // debug
    // painter.setPen(QColor(255,255,255,180));
    // painter.setFont(QFont("DejaVu", 10));
    // painter.drawText(10, height()-10,
    // QString("rate: %1x  %2").arg(m_playbackRate,0,'f',2).arg(m_paused? "paused":""));

    painter.restore();
}

void Level1::checkEnemyCollisions() {
    QRectF rect = pellsBawl.playerRectangle();
    for (Enemy &enemy : enemies) {
        if (rect.intersects(enemy.rect)) {
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

// struct ProjectileInfo {
//     QPointF pos {0,0};
//     QPointF vel {0,0};
//     qreal radius = 6.0; // for simple collision prediction
// };

// struct OpponentSnapshot {
//     QPointF pos {0,0};
//     QPointF vel {0,0};
//     bool onGround = true;
//     bool isShooting = false; // transient when a shot is fired this frame
//     Dir facing = Dir::Right;
//     QList<ProjectileInfo> activeProjectiles; // cloud shots
// };

// struct SelfSnapshot {
//     QPointF pos {0,0};
//     QPointF vel {0,0};
//     bool onGround = true;
//     Dir facing = Dir::Right;
//     qreal health = 100.0;
//     qreal stamina = 100.0; // basic gating for specials
// };

// struct WorldSnapshot {
//     qreal gravity = 1800.0;   // px/s^2; used for air-time heuristics
//     QRectF walkableBounds;    // to avoid walking off-screen
//     qreal timeSeconds = 0.0;  // world time for seeded randomness
// };

void Level1::timerEvent(QTimerEvent *) {
    qint64 now = m_clock.elapsed();
    double dt = (now - m_lastMs) / 1000.0;
    m_lastMs = now;

    joystick.updateTime(now);

    // pellsBawl
    pellsBawl.tick(dt);

    // opponent AI
    struct OpponentSnapshot os;
    os.pos = pellsBawl.playerRectangle().center();
    os.vel = QPointF(pellsBawl.velX(), pellsBawl.velY());
    os.onGround = !pellsBawl.jumping();
    os.facing = pellsBawl.faceLeft() ? Dir::Left : Dir::Right;

    struct SelfSnapshot ss;
    ss.pos = fighter->pos();
    ss.vel = fighter->vel();
    ss.onGround = fighter->onGround();
    ss.facing = fighter->facing();

    struct WorldSnapshot ws;
    ws.timeSeconds = dt;
    ws.walkableBounds = bounds;

    // fighterAI->sense(ss, os, ws);
    // fighterAI->update();

    // Opponent movement
    fighter->update(dt, platforms, bounds);

    // Collisions
    pellsBawl.checkCollisions(platforms, bounds);
    checkEnemyCollisions();

    update(); // Repaint the widget
}

void Level1::loadPlatforms(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open platforms file.");
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonObject o = doc.object();
    if(!o.empty()) {
        QJsonArray a = o["world"].toArray();
        world.setRect(a.at(0).toInt(), a.at(1).toInt(), a.at(2).toInt(), a.at(3).toInt());

        a = o["bounds"].toArray();
        bounds.setRect(a.at(0).toInt(), a.at(1).toInt(), a.at(2).toInt(), a.at(3).toInt());

        ground = o["ground"].toInt(550);

        QJsonArray platformArray = o["platforms"].toArray();

        // Ground platform
        // platforms.append(Platform(0, height() - 50, width(), 50, true));

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
}
