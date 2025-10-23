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
    loadWorld(":/assets/test.json");

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

void Level1::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) close();
}

QPolygonF triLocal(const QRectF& R, bool left) {
    if (left) return QPolygonF{ R.topRight(), R.bottomLeft(), R.bottomRight() };
    return QPolygonF{ R.topLeft(), R.bottomRight(), R.bottomLeft() };
}

void drawShape(QPainter& p, const Shape& it){
    QPen pen(QColor(0,255,0)); /*pen.setWidthF();*/ p.setPen(pen);
    QColor fill = it.isWall? QColor(140,210,140) : QColor(0,0,200);
    fill.setAlphaF(1.0);
    p.save();
    if (it.shape==Shape::Rect) {
        p.setBrush(fill); p.drawRect(it.rect);
    } else {
        QPainterPath path; QPolygonF tri = triLocal(it.rect, it.shape == Shape::TriLeft ? true : false);
        p.setBrush(fill); p.drawPolygon(tri);
    }
    p.restore();
}

void applyTransform(QPainter& p, const Transform& tf){ p.translate(tf.pos); p.rotate(tf.rotation); p.scale(tf.scaleX, tf.scaleY); }

void drawImage(QPainter& p, const Image& s){
    if (s.img.isNull()) return;
    p.save(); applyTransform(p, s.tf);
    QRectF r = QRectF(-s.img.width()/2.0, -s.img.height()/2.0, s.img.width(), s.img.height());
    p.drawImage(r.topLeft(), s.img); // image not auto-scaled; transform applies scale
    p.restore();
}

void Level1::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    // Set the world rectangle (logical coordinates)
    painter.setWindow(window.toRect()); //world

    // Optional: Set the viewport (physical coordinates)
    painter.setViewport(0, 0, width(), height());

    // Save state
    painter.save();

#ifdef USE_OPENGL
    //cls
    painter.setBrush(Qt::white);
    painter.drawRect(rect());
#endif

    // draw parallax bg
    QPointF scrollOffset = (window.topLeft() - world.topLeft());
    painter.drawPixmap(window, background, QRectF(background.rect().topLeft() + scrollOffset * 0.3, background.rect().size()));
    painter.drawPixmap(window, middleground, QRectF(middleground.rect().topLeft() + scrollOffset * 0.7, middleground.rect().size()));

    painter.setBrush(Qt::blue);

    // Draw platforms
    for (const auto& it : shapes) drawShape(painter, it);

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

    // Draw foreground
    for (const auto& s : images) drawImage(painter, s);

    // Draw very foreground artifacts
    painter.drawPixmap(QRectF(QPointF(900.0 - scrollOffset.x() * 1.7, 400 - scrollOffset.y() * 1.1), 0.9 * foreground.rect().size() * world.height() / foreground.rect().height()), foreground, foreground.rect());

    // Draw HUD
    if (pellsBawl.isCharging()) pellsBawl.paintHUD(painter);

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
        for (const Shape &shape : shapes) {
            if (shape.rect.intersects(enemy.rect)) {
                enemy.move(shape.rect.toRect());  // Move the enemy within its platform
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

void Level1::doFighterSense(double dt) {
    // Opponent AI
    struct OpponentSnapshot os;
    os.pos = pellsBawl.playerRectangle().center();
    os.vel = QPointF(pellsBawl.velX(), pellsBawl.velY());
    os.onGround = !pellsBawl.jumping();
    os.facing = pellsBawl.faceLeft() ? Dir::Left : Dir::Right;

    struct SelfSnapshot ss = { fighter->pos(), fighter->vel(), fighter->onGround(), fighter->facing() };

    struct WorldSnapshot ws;
    ws.timeSeconds = dt;
    ws.walkableBounds = bounds;

    fighterAI->sense(ss, os, ws);
}

void Level1::doScrolling(double dt) {
    QPointF center = fighter->pos() + (pellsBawl.playerRectangle().center() - fighter->pos()) / 2.0;
    QPointF scrollV = (center - window.center()) * 1.0 * dt;
    // window.moveTo(window.topLeft() + scrollV);
    window.moveCenter(window.center() + scrollV);

    QRectF sceneRect = bounds; //rect();
    if (!sceneRect.contains(window)) {
        window.moveTop(qMax(sceneRect.top(), window.top()));
        window.moveBottom(qMin(sceneRect.bottom(), window.bottom()));
        window.moveLeft(qMax(sceneRect.left(), window.left()));
        window.moveRight(qMin(sceneRect.right(), window.right()));
    }
}

void Level1::timerEvent(QTimerEvent *) {
    qint64 now = m_clock.elapsed();
    double dt = (now - m_lastMs) / 1000.0;
    m_lastMs = now;

    joystick.updateTime(now);

    // pellsBawl
    pellsBawl.tick(dt);

    doFighterSense(dt);
    fighterAI->update();

    // Opponent movement
    fighter->update(dt, shapes, bounds);

    // Collisions
    pellsBawl.checkCollisions(shapes, bounds.toRect());
    checkEnemyCollisions();

    doScrolling(dt);

    update(); // Repaint the widget
}

// ---- JSON helpers ----
static QPointF jsonToPoint(const QJsonValue& v){ QJsonObject o=v.toObject(); return QPointF(o.value("x").toDouble(), o.value("y").toDouble()); }
static QRectF jsonToRect(const QJsonValue& v){ QJsonObject o=v.toObject(); return QRectF(o.value("x").toDouble(), o.value("y").toDouble(), o.value("w").toDouble(), o.value("h").toDouble()); }

QList<Shape> loadShapes(const QJsonDocument& doc){
    QList<Shape> shapes;
    QJsonObject root = doc.object();
    for (auto v : root.value("interaction").toArray()){
        QJsonObject o=v.toObject();
        Shape it; it.id = o.value("id").toString("dummy");
        it.shape = o.value("shape").toString("rect") == "rect" ? Shape::Rect : (o.value("shape").toString() == "tri_left" ? Shape::TriLeft : Shape::TriRight);
        it.isWall = o.value("is_wall").toBool(false);
        it.rect = jsonToRect(o.value("rect"));
        shapes.push_back(it);
    }
    return shapes;
}

QList<Image> loadImages(const QJsonDocument& doc){
    QList<Image> images;
    QJsonObject root = doc.object();
    QString basePath = root.value("basePath").toString(":assets/");
    for (auto v : root.value("graphics").toArray()){
        QJsonObject o = v.toObject();
        Image s; s.id = o.value("id").toString();
        s.path = o.value("path").toString();
        s.z = o.value("z").toDouble(0);
        s.tf.pos = jsonToPoint(o.value("pos"));
        s.tf.rotation=o.value("rotation").toDouble(0);
        s.tf.scaleX=o.value("scaleX").toDouble(1.0);
        s.tf.scaleY=o.value("scaleY").toDouble(1.0);
        if (!s.img.load(basePath + s.path)) if(!s.img.load(basePath + s.path.split("/").last())) s.img.load(s.path);
        if (!s.img.isNull()) images.push_back(s);
    }
    return images;
}

void Level1::loadWorld(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open platforms file.");
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonObject root = doc.object();
    if(!root.empty()) {
        auto i = root.value("interaction").toArray();
        if(!i.empty()) {
            shapes = loadShapes(doc);
            images = loadImages(doc);

            world = jsonToRect(root.value("world"));
            window = jsonToRect(root.value("window"));
            bounds = world;

            background.load(":/assets/" + root.value("background").toString());
            middleground.load(":/assets/" + root.value("middleground").toString());
            foreground.load(":/assets/" + root.value("foreground").toString());
        } else { //legacy
            QJsonArray a = root["world"].toArray();
            world.setRect(a.at(0).toInt(), a.at(1).toInt(), a.at(2).toInt(), a.at(3).toInt());

            a = root["bounds"].toArray();
            bounds.setRect(a.at(0).toInt(), a.at(1).toInt(), a.at(2).toInt(), a.at(3).toInt());

            ground = root["ground"].toInt(550);

            QJsonArray platformArray = root["platforms"].toArray();

            // Ground platform
            // platforms.append(Platform(0, height() - 50, width(), 50, true));

            // Read platforms from the JSON file
            for (const QJsonValue &value : platformArray) {
                QJsonObject obj = value.toObject();
                int x = obj["x"].toInt();
                int y = obj["y"].toInt();
                int w = obj["width"].toInt();
                int h = obj["height"].toInt();
                Shape f; f.rect = QRect(x,y,w,h);
                shapes.append(f);
            }
        }
    }
}
