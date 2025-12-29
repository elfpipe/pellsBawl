#include "Game.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QSoundEffect>
#include <QJoysticks.h>

#include "pellsBawl.h"
#include "title.h"

#include <QDebug>

#if 0
"parallax" : [
                 { "image" : "landscape.jpg", "off" : [ 0, 0 ], "rate" : [0, 0], "scale" : 0.0 , "z" : -3}
],
#endif

Game::Game(QWidget *parent)
#ifdef USE_OPENGL
: QOpenGLWidget(parent)
#else
: QWidget(parent)
#endif
{
    audio = new QAudioOutput(this);
    player = new QMediaPlayer(this);
    player->setAudioOutput(audio);

    joystick = new GameJoystick(this);

    QTimer *oneShot = new QTimer(this);
    oneShot->setSingleShot(true);
    connect(oneShot, &QTimer::timeout, this, &Game::action);
    oneShot->start(50);
}

void Game::displayGraphics(QPixmap pixmap, const QColor &color, bool full) {
    auto *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
      qDebug() << "No current OpenGL context!";
      return;
    }

    QOpenGLFunctions *f = ctx->functions();
    const char *vendor   = reinterpret_cast<const char*>(f->glGetString(GL_VENDOR));
    const char *renderer = reinterpret_cast<const char*>(f->glGetString(GL_RENDERER));
    const char *version  = reinterpret_cast<const char*>(f->glGetString(GL_VERSION));
    const char *slVer    = reinterpret_cast<const char*>(f->glGetString(GL_SHADING_LANGUAGE_VERSION));

    qDebug().noquote()
      << "GL_VENDOR  :" << (vendor ? vendor : "(null)") << "\n"
      << "GL_RENDERER:" << (renderer ? renderer : "(null)") << "\n"
      << "GL_VERSION :" << (version ? version : "(null)") << "\n"
      << "GLSL       :" << (slVer ? slVer : "(null)");

    pause(); titleGraphics = pixmap; showTitle = true; showFullscreen = full; titleBg = color; update();
}

void Game::playJingle(QString jingle, bool repeat) {
    if(!jingle.isEmpty()) {
        // Local file or qrc (see below)
        player->setSource(QUrl(jingle));
        audio->setVolume(0.8);                 // 0.0 .. 1.0
        player->setLoops(repeat ? QMediaPlayer::Infinite : QMediaPlayer::Once);  // or QMediaPlayer::Infinite for bg music
        player->play();
    }
}

void Game::playSfx(const QString &sfx){
    QSoundEffect fx;
    fx.setSource(QUrl(sfx));
    fx.setLoopCount(1);         // or QSoundEffect::Infinite
    fx.setVolume(0.35f);
}
void Game::action() {
    showFullScreen();
    displayGraphics(QPixmap(":/assets/intro/splash.png"), QColor(0, 200, 50));
    playJingle("qrc:/assets/intro/pellsBawl_intro_jingle.wav");
    joystick->waitForPush();
    showTitle = false;
    level1();
}

void Game::level1()
{
    // Load platforms from a JSON file
    loadWorld("level2.json", ":/assets/level2/");

    pellsBawl = new PellsBawl(this);
    joyCommander = new PellsBawlCommander(this, pellsBawl);
    joystick->setCommander(joyCommander);

    // Create enemies and assign them to platforms
    enemies.append(Enemy(110, 412, 40, 40));  // Example enemy 1 on platform
    enemies.append(Enemy(310, 312, 40, 40));  // Example enemy 2 on platform

    unpause();
}

void Game::level2()
{
    pause(); clear(false); // keep pb

    //load artifacts for level 2...
}

void Game::level3()
{
    pause(); clear(false); // keep pb

    // load level data.... (todo)

    fighter = new Fighter(this);
    fighter->loadFromJson(":/assets/mt2/mt2.json");

    enemyCommander = new FighterCommander(this, fighter);

    fighterAI = new FighterAI(this);
    fighterAI->setCommander(enemyCommander);

    unpause();
}

void Game::unpause() {
    pauseId = startTimer(1000/60); // ~60 FPS
    m_clock.start();

    m_lastMs = m_clock.elapsed();
}

void Game::pause() {
    if (pauseId >= 0) killTimer(pauseId);
}

void Game::clear(bool pb){
    pause();
    if (pb) delete pellsBawl;
    if (fighter) delete fighter;
    if (joyCommander) delete joyCommander;
    if (enemyCommander) delete enemyCommander;
    if (fighterAI) delete fighterAI;
    enemies.clear();
    shapes.clear();
    images.clear();

    animLayers.clear();
}

Game::~Game() { clear(true); }

void Game::keyPressEvent(QKeyEvent *event) {
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

void Game::drawAnimationLayer(QPainter &p, ParallaxLayer &a, QPointF &scrollOffset) {
    p.drawPixmap(a.scale == 0 ? window.toRect() : QRectF(QPointF(world.left() + a.off.x() - scrollOffset.x() * a.rate.x(),
                                                                 world.bottom() - a.off.y() - scrollOffset.y() * a.rate.y() - a.image.rect().height() * a.scale),
                                                         a.scale == 0.0 ? window.size() : a.image.rect().size() * a.scale).toRect(), a.image);
}

#ifdef USE_OPENGL
void Game::paintGL() {
#else
void Game::paintEvent(QPaintEvent *) {
#endif
    QPainter painter(this);

    if (showTitle) {
        if (showFullscreen) painter.drawPixmap(painter.window(), titleGraphics);
        else {
            painter.setBrush(titleBg); painter.drawRect(rect());
            painter.drawPixmap((rect().bottomRight() - titleGraphics.rect().bottomRight()) / 2, titleGraphics); }
        return;
    }

    // Set the world rectangle (logical coordinates)
    painter.setWindow(window.toRect()); //world

    // Optional: Set the viewport (physical coordinates)
    painter.setViewport(0, 0, width(), height());

    // Save state
    painter.save();

#ifdef USE_OPENGL
    //cls
    painter.setBrush(Qt::white);
    painter.drawRect(0, 0, width(), height());
#endif

    // draw parallax bg
    QPointF scrollOffset = window.topLeft() - world.topLeft();

    for (auto b : animLayers) if (b.z <= -3) drawAnimationLayer(painter, b, scrollOffset);
    for (auto b : animLayers) if (b.z <= -2) drawAnimationLayer(painter, b, scrollOffset);
    for (auto b : animLayers) if (b.z <= -1) drawAnimationLayer(painter, b, scrollOffset);

    // painter.drawPixmap(window, background, QRectF(bgScrollOff + scrollOffset * bgScrollRate, background.rect().size()));
    // painter.drawPixmap(window, middleground, QRectF(mdScrollOff + scrollOffset * mdScrollRate, middleground.rect().size()));

    painter.setBrush(Qt::blue);

    // Draw shapes (for debug?)
    // for (const auto& it : shapes) drawShape(painter, it);

    // Draw enemies with the correct animation based on their state
    for (const Enemy &enemy : enemies) {
        if (!enemy.isDefeated) {
            painter.drawPixmap(enemy.rect, enemy.alivePixmap); // Draw enemy when alive
        } else {
            painter.drawPixmap(enemy.rect, enemy.defeatedPixmap); // Draw enemy when defeated
        }
    }

    // This includes shots and shadow
    if (pellsBawl) pellsBawl->paintWalker(painter, ground); //, playerRect, turningLeft, m_animTime); // Draw player character

    // Fighter (MT2)
    if (fighter) fighter->paint(&painter);

    // Draw foreground
    for (const auto& s : images) drawImage(painter, s);

    // Draw very foreground artifacts (parallax)
    for (auto &b : animLayers) if (b.z > 0) drawAnimationLayer(painter, b, scrollOffset);

    // Draw HUD
    if (pellsBawl && pellsBawl->isCharging()) pellsBawl->paintHUD(painter);

    // debug
    // painter.setPen(QColor(255,255,255,180));
    // painter.setFont(QFont("DejaVu", 10));
    // painter.drawText(10, height()-10,
    // QString("rate: %1x  %2").arg(m_playbackRate,0,'f',2).arg(m_paused? "paused":""));

    painter.restore();
}

void Game::checkEnemyCollisions() {
    QRectF rect = pellsBawl->playerRectangle();
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

void Game::doFighterSense(double dt) {
    // Opponent AI
    struct OpponentSnapshot os;
    os.pos = pellsBawl->playerRectangle().center();
    os.vel = QPointF(pellsBawl->velX(), pellsBawl->velY());
    os.onGround = !pellsBawl->jumping();
    os.facing = pellsBawl->faceLeft() ? Dir::Left : Dir::Right;

    struct SelfSnapshot ss = { fighter->pos(), fighter->vel(), fighter->onGround(), fighter->facing() };

    struct WorldSnapshot ws;
    ws.timeSeconds = dt;
    ws.walkableBounds = bounds;

    fighterAI->sense(ss, os, ws);
}

void Game::doScrolling(double dt, bool twoPlayer = false) {
  qreal scrollSpeed = 1.5;
    if (pellsBawl->slopeRiding()) { scrollSpeed = 10.0; }

    QPointF center = twoPlayer ? fighter->pos() + (pellsBawl->playerRectangle().center() - fighter->pos()) / 2.0 : pellsBawl->playerRectangle().center();
    QPointF scrollV = (center - QPointF(window.center().x(), window.center().y() + window.height() / 5.0)) * scrollSpeed * dt;
    window.moveCenter(window.center() + scrollV);

    QRectF sceneRect = bounds; //rect();
    if (!sceneRect.contains(window)) {
        window.moveTop(qMax(sceneRect.top(), window.top()));
        window.moveBottom(qMin(sceneRect.bottom(), window.bottom()));
        window.moveLeft(qMax(sceneRect.left(), window.left()));
        window.moveRight(qMin(sceneRect.right(), window.right()));
    }
}

void Game::timerEvent(QTimerEvent *) {
    qint64 now = m_clock.elapsed();
    double dt = (now - m_lastMs) / 1000.0;
    m_lastMs = now;

    joystick->updateTime(now);

    // pellsBawl
    if (pellsBawl) pellsBawl->tick(dt);

    if (fighterAI) {
        doFighterSense(dt);
        fighterAI->update();
    }

    // // Opponent movement
    if (fighter) fighter->update(dt, shapes, bounds);

    // Collisions
    if (pellsBawl) {
        pellsBawl->checkCollisions(shapes, bounds);
        checkEnemyCollisions();
    }

    if (pellsBawl) doScrolling(dt);

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

QList<Image> loadImages(const QJsonDocument& doc, const QString &path){
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
        if (!s.img.load(basePath + s.path))
            if(!s.img.load(basePath + s.path.split("/").last()))
                if (!s.img.load(path + s.path))
                    if(!s.img.load(path + s.path.split("/").last()))
                        s.img.load(s.path);
        if (!s.img.isNull()) images.push_back(s);
    }
    return images;
}

void Game::loadWorld(const QString &filename, const QString &path) {
    QFile file(path + filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open platforms file.");
        return;
    }

    QByteArray data = file.readAll();
    qDebug() << "data:" << data.size();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonObject root = doc.object();
    qDebug() << root.isEmpty();
    if(!root.empty()) {
        auto i = root.value("interaction").toArray();
        qDebug() << i.isEmpty();
        if(!i.empty()) {
            shapes = loadShapes(doc);
            qDebug() << "shapes:" << shapes.size();
            images = loadImages(doc, path);
            qDebug() << "images:" << images.size();

            world = jsonToRect(root.value("world"));
            qDebug() << "world:" << world;
            window = jsonToRect(root.value("window"));
            qDebug() << "window:" << window;
            bounds = world;

            auto p = root.value("parallax").toArray();
            for (auto l: p) {
                ParallaxLayer g;
                auto o = l.toObject();
                g.image.load(path + o.value("image").toString());
                qDebug() << g.image.rect();
                auto a = o.value("off").toArray(); g.off = QPointF(a.at(0).toDouble(0.0), a.at(1).toDouble(0.0));
                a = o.value("rate").toArray(); g.rate = QPointF(a.at(0).toDouble(0.0), a.at(1).toDouble(0.0));
                g.scale = o.value("scale").toDouble();
                g.z = o.value("z").toInt();
                animLayers.append(g);
            }
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
