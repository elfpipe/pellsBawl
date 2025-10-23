#ifndef PELLSBAWL_H
#define PELLSBAWL_H

// main.cpp
#include <QtWidgets>
#include <cmath>

#include "combo.h"
#include "bezier.h"
#include "platform.h"
#include "commander.h"

struct Curve {
    enum Type { Const, Sine, Linear } type = Const;

    // const
    double value = 0.0;

    // sine
    double amp = 0.0, phaseRad = 0.0, bias = 0.0;

    // linear
    double start = 0.0, end = 0.0;

    double eval(double t, double duration) const {
        switch (type) {
        case Const:
            return value;
        case Sine: {
            const double omega = (duration > 0.0) ? (2.0 * M_PI / duration) : 0.0;
            return amp * std::sin(omega * t + phaseRad) + bias;
        }
        case Linear: {
            if (duration <= 0.0) return end;            // safe fallback
            const double u = t / duration;               // 0..1 over the clip
            return start + (end - start) * u;
        }
        }
        return 0.0;
    }

    static Curve fromJson(const QJsonValue& v) {
        Curve c;
        if (!v.isObject()) {
            // allow bare numbers to act like const curves
            c.type = Const;
            c.value = v.toDouble(0.0);
            return c;
        }
        const auto o = v.toObject();
        const QString t = o.value("type").toString("const");

        if (t == "const") {
            c.type = Const;
            c.value = o.value("value").toDouble(0.0);
        } else if (t == "sine") {
            c.type = Sine;
            c.amp = o.value("amp").toDouble(0.0);
            c.bias = o.value("bias").toDouble(0.0);
            c.phaseRad = o.value("phaseDeg").toDouble(0.0) * M_PI / 180.0;
        } else if (t == "linear") {
            c.type = Linear;
            c.start = o.value("start").toDouble(0.0);
            c.end   = o.value("end").toDouble(c.start);
        } else {
            // unknown type → fallback to const 0
            c.type = Const; c.value = 0.0;
        }
        return c;
    }
};

struct Track {
    QString id;
    QPointF baseOffset{0,0};
    Curve x, y, rot;
    int zOrder = 1;
    QPixmap pix;
    QSizeF desiredSize; // logical pixels (DIPs)
};

class PellsBawl : public QWidget {
    Q_OBJECT
public:
    PellsBawl(QWidget *parent = nullptr) : QWidget(parent) {
        playerRect = QRect(100, 500, 50, 50); // Initial position of the player

        loadAnimation();
        selectClip("walk");
    }

    void paintWalker(QPainter &p, qreal ground); //, QRectF r, bool turningLeft = false, const double m_animTime = .0);
    void drawShadow(QPainter& p, const QPointF& center, const QSizeF& size, double opacity) {
        QRadialGradient g(center, size.width()/2.0, center);
        QColor c(0,0,0, int(255*opacity));
        g.setColorAt(0.0, c);
        g.setColorAt(1.0, QColor(0,0,0,0));
        QPainterPath path;
        QRectF r(center.x()-size.width()/2.0, center.y()-size.height()/2.0, size.width(), size.height());
        path.addEllipse(r);
        p.save(); p.setBrush(g); p.setPen(Qt::NoPen); p.drawPath(path); p.restore();
    }
    void paintHUD(QPainter &p) {
        if (btw && isCharging()) btw->drawPowerBar(p);
    }
    bool selectClip(const QString& id) {
        auto it = m_clips.find(id);
        if (it == m_clips.end()) return false;
        m_activeClipId = id;
        m_tracks       = it->tracks;
        m_durationSec  = it->durationSec;
        m_useFootCapsule = it->useFootCapsule;
        // m_animTime = 0.0; // optional: reset time on switch
        // update();
        return true;
    }

    void updatePlayerPosition() {
        {
            // Handle horizontal movement
            if (isMovingLeft) {
                velocityX = -std::max(0.1, std::min(5.0, -velocityX * 1.25));
            } else if (isMovingRight) {
                velocityX = std::max(0.1, std::min(5.0, velocityX * 1.25));
            } else {
                velocityX *= 0.95;
            }

            // set the playback rate
            if (std::fabs(velocityX) < 0.01)
                m_playbackRate = 0.0;
            else if (!isThrowing)
                m_playbackRate = qAbs(velocityX/2);
            if (isThrowing)
                m_playbackRate = 2.0;
            if (isJumping)
                m_playbackRate = 1.0;
            if (m_finishAnim)
                m_playbackRate = 2.0;

            // Apply gravity and jump velocity
            if (isJumping || isFalling) {
                velocityY += gravity;
            }
            if(velocityY > 0) isFalling = true; else isFalling = false;

            // Update player position
            playerRect.moveLeft(playerRect.left() + velocityX);
            playerRect.moveTop(playerRect.top() + velocityY);
        }
    }

    bool m_onGround = false;

    bool checkCollisions(QList<Shape> platforms, QRect bounds) {
        bool onGround = false;
        for (auto platform : platforms) {
            if (playerRect.intersects(platform.rect)) {
                // Collision with ground or other platform
                // if (playerRect.bottom() <= platform.rect.top() && playerRect.bottom() >= platform.rect.top() - velocityY) {
                //     playerRect.moveBottom(platform.rect.top());
                if (isFalling) {
                    isJumping = false;
                    canJump = true;
                    isFalling = false;
                    // isThrowing = false;
                    velocityY = 0;
                    onGround = true;
                    playerRect.moveBottom(platform.rect.top());
                    // selectClip("walk");
                    if (!m_onGround) {
                        m_finishAnim = true;
                        m_onGround = true;
                    }
                }
            }
        }

        if (!onGround) {
            isFalling = true;
        }

        QRectF sceneRect = bounds; //rect();
        if (!sceneRect.contains(playerRect)) {
            playerRect.moveTop(qMax(sceneRect.top(), playerRect.top()));
            playerRect.moveBottom(qMin(sceneRect.bottom(), playerRect.bottom()));
            playerRect.moveLeft(qMax(sceneRect.left(), playerRect.left()));
            playerRect.moveRight(qMin(sceneRect.right(), playerRect.right()));
        }
        return onGround;
    }
    void tick(double dt) {
        bool m_animDone = false;

        updatePlayerPosition();

        if (!m_paused) {
            m_animTime += dt * m_playbackRate;
            // keep in [0, duration)
            if (m_durationSec > 0.0) {
                if (std::fabs(m_animTime) > m_durationSec)
                    m_animDone = true;
                m_animTime = std::fmod(m_animTime, m_durationSec);
                if (m_animTime < 0) m_animTime += m_durationSec; // support reverse
            }
        }

        if (m_animDone && (m_finishAnim || !isLoop())) { selectClip("walk"); isThrowing = false; isJumping = false; m_finishAnim = false; }

        for (auto btw : shots) btw->onTick();
    }

    double durationSec() {
        return m_durationSec;
    }

    double velX() { return velocityX; }
    double velY() { return velocityY; }
    double jumping() { return isJumping; }
    bool faceLeft() { return isFacingLeft; }
    QRectF playerRectangle() { return playerRect; }
private:

    struct AnimClip {
        QString id;
        double durationSec = 1.0;
        bool loop = true;
        bool useFootCapsule = false;
        QVector<Track> tracks;   // tracks for this clip (pixmaps are implicitly shared)
    };

    QHash<QString, AnimClip> m_clips;
    QString m_activeClipId;
    bool m_useFootCapsule = false;   // read from active clip
    QHash<QString, QPixmap> m_pixById; // loaded once per part id

    bool m_finishAnim = false;

private:
    QVector<Track> m_tracks;
    double m_durationSec = 1.0;
    double m_globalScale = 1.25; // 1.0 = original size
    bool m_allPixLoaded = false;
    QElapsedTimer m_clock;

    const Track* trackById(const QString& id) const {
        for (const auto& tr : m_tracks) if (tr.id == id) return &tr;
        return nullptr;
    }

    bool isLoop() const {
        return m_clips[m_activeClipId].loop;
    }

    static QPointF readPoint(const QJsonValue& v) {
        const auto o = v.toObject();
        return QPointF(o.value("x").toDouble(0.0), o.value("y").toDouble(0.0));
    }
    static QSizeF readSizeFromJson(const QJsonValue& v) {
        if (v.isDouble()) { double s=v.toDouble(); return QSizeF(s,s); }
        if (v.isObject()) {
            auto o=v.toObject(); 
            if (o.contains("w") && o.contains("h"))
                return QSizeF(o.value("w").toDouble(), o.value("h").toDouble());
        }
        return QSizeF(); // invalid => use natural image size
    }

    void loadAnimation();


    void drawPixmapScaledCentered(QPainter& p, const Track& tr, const QPointF& pos, double rotDeg) {
        if (tr.pix.isNull()) return;

        // Natural size in *logical* pixels
        const qreal dpr = tr.pix.devicePixelRatio();
        const QSizeF natural = tr.pix.size() / dpr;

        // Desired size (fallback to natural if not specified)
        const QSizeF desired = (!tr.desiredSize.isValid() || tr.desiredSize.isEmpty())
                                 ? natural
                                 : tr.desiredSize;

        const qreal sx = desired.width()  / qMax(1.0, natural.width());
        const qreal sy = desired.height() / qMax(1.0, natural.height());

        p.save();
        p.translate(pos);
        p.rotate(rotDeg);
        p.scale(sx, sy);
        // draw centered at natural coordinates
        p.drawPixmap(QPointF(-natural.width()/2.0, -natural.height()/2.0), tr.pix);
        p.restore();
    }

public:
    // pöellsBawll
    void keyLeft() {
        isMovingLeft = true;
        isMovingRight = false;
        isFacingLeft = true;
    }
    void keyRight() {
        isMovingLeft = false;
        isMovingRight = true;
        isFacingLeft = false;
    }
    void keyJump() {
        if (canJump) {
            m_onGround = false;
            if (isJumping) {
                canJump = false;
                velocityY -= doubleJumpVelocity;
                selectClip("jump_spin");
            } else {
                velocityY = -jumpVelocity;
                selectClip("jump_straight");
            }
            isJumping = true;
        }
    }
    void keyThrow() {
        if (btw) { // must be thrown by release
            shots.removeAll(btw);
            btw = nullptr;
        }
        btw = new BezierThrowWidget(this);
        if (btw) {
            QObject::connect(btw, &BezierThrowWidget::hasHit, this, [&](){
                shots.removeAll(sender());
            });
            shots.append(btw);
            btw->handleSpaceDown();
        }
    }
    bool isCharging() { return btw && btw->isCharging(); }

    void release(){
        isMovingLeft = false;
        isMovingRight = false;
    }
    void releaseThrowKey() {
        selectClip("throw");
        m_returnToWalk = true;
        m_animTime = 0.0;
        isThrowing = true;

        if(btw) {
            btw->handleSpaceRelease(playerRect.center() - QPoint(0, 50), isFacingLeft);
            btw = nullptr;
        }
    }

    // if (event->key() == Qt::Key_J) {
    //     playerAnim.selectClip("jump_straight");
    // }
    // if (event->key() == Qt::Key_S) {
    //     playerAnim.selectClip("jump_spin");
    // }


private:
    QRectF playerRect;

    bool isJumping = false;
    bool isMovingLeft = false;
    bool isMovingRight = false;
    bool isFalling = false;
    bool isFacingLeft = false;
    bool isThrowing = false;
    bool canJump = true;

    int jumpVelocity = 15.0;
    int doubleJumpVelocity = 25.0;
    int gravity = 1.0;
    double velocityX = 0.0;
    double velocityY = 0.0;

    QVector<BezierThrowWidget *> shots;
    BezierThrowWidget *btw = nullptr;

    // in WalkerWidget:
    double m_playbackRate = 1.0;   // 1.0 = normal speed
    bool   m_paused = false;
    double m_animTime = 0.0;       // current time within [0, durationSec)

    bool m_returnToWalk = false;
};

// ------------------------------
// Example glue: minimal commander that maps intents to your animation/physics
// ------------------------------
class PellsBawlCommander : public QObject, public IJoystickCommander {
    Q_OBJECT
public:
    explicit PellsBawlCommander(QObject* parent=nullptr, PellsBawl *pb = nullptr) : QObject(parent) {
        this->pellsBawl = pb;

        // In your setup, after creating Fighter and FighterCommander:
        // onFace = [&](Dir d){ fighter->turn(d); };
        // onWalk = [&](qreal dir){ fighter->moveHoriz(dir); };
        // onStand = [&](){ fighter->stop(); };
        // onCrouch = [&](){ fighter->crouch(); };
        // onJump = [&](){ fighter->jump(); };
        // onKick = [&](){ fighter->kick(); };
        // onSlowPunch = [&](){ fighter->slowPunch(); };
        // onCrouchPunch = [&](){ fighter->crouchPunch(); };
        // onAirKick = [&](){ fighter->airKick(); };
        // onAirPunch = [&](){ fighter->airPunch(); };
        // onBackflipKick = [&](){ fighter->backflipKick(); };
        // onSpecial = [&](){ fighter->special(); };
        // onVictory = [&](){ fighter->victory(); };
    }

    // Wire these to your character controller
    // std::function<void(Dir)> onFace;
    // std::function<void(qreal)> onWalk; // negative for left, positive for right
    // std::function<void()> onStand;
    // std::function<void()> onCrouch;
    // std::function<void()> onJump;
    // std::function<void()> onKick;
    // std::function<void()> onSlowPunch;
    // std::function<void()> onCrouchPunch;
    // std::function<void()> onAirKick;
    // std::function<void()> onAirPunch;
    // std::function<void()> onBackflipKick;
    // std::function<void()> onVictory;
    // std::function<void()> onSpecial;

    // void applyIntent(const Intent& i) override {

    //     if(onFace) onFace(i.dir);
    //     switch(i.action){
    //     case Action::Stand: qDebug() << "Stand"; if(onStand) onStand(); break;
    //     case Action::Walk:  if(onWalk) onWalk(i.dir==Dir::Left? -1.0 : 1.0); break;
    //     case Action::Crouch:qDebug() << "Crouch"; if(onCrouch) onCrouch(); break;
    //     case Action::Jump:  qDebug() << "Jump"; if(onJump) onJump(); break;
    //     case Action::Kick:  qDebug() << "Kick"; if(onKick) onKick(); break;
    //     case Action::SlowPunch: qDebug() << "SlowPunch"; if(onSlowPunch) onSlowPunch(); break;
    //     case Action::CrouchPunch: qDebug() << "CrouchPunch"; if(onCrouchPunch) onCrouchPunch(); break;
    //     case Action::AirKick: qDebug() << "AirKick"; if(onAirKick) onAirKick(); break;
    //     case Action::AirPunch: qDebug() << "AirPunch"; if(onAirPunch) onAirPunch(); break;
    //     case Action::CrouchBackflipKick: qDebug() << "BackFlipKick"; if(onBackflipKick) onBackflipKick(); break;
    //     case Action::Special: qDebug() << "Special"; if(onSpecial) onSpecial(); break;
    //     case Action::Victory: qDebug() << "Victory"; if(onVictory) onVictory(); break;
    //     }
    // }

    bool applyCombo(QVector<Combo::Key> combo) override {
        bool resetCombo = false;
        if(!resetCombo & !combo.empty()) {
            switch(combo.last()) {
            case Combo::UP:
                break;
            case Combo::DOWN:
                break;
            case Combo::LEFT:
                pellsBawl->keyLeft();
                break;
            case Combo::RIGHT:
                pellsBawl->keyRight();
                break;
            case Combo::FIRE1:
                pellsBawl->keyJump();
                break;
            case Combo::FIRE2:
                pellsBawl->keyThrow();
                break;
            }
        }
        return resetCombo;
    }

    void releaseLeftRight() override {
        pellsBawl->release();
    }

    void releasePowerButton() override {
        pellsBawl->releaseThrowKey();
    }

private:
    PellsBawl *pellsBawl = nullptr;
};

#endif
