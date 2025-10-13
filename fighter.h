// Fighter.hpp — Qt6 single-file character container + sprite animator
// Implements the Fighter character with physics (ground & platforms), actions,
// JSON-driven animations, and painting with scaling & rotation.
//
// Designed to pair with the previously provided FighterAI / SampleCommander.
// Drop this header in your project and include it in exactly one TU.
//
// Build: Qt 6 (Core, Gui). If you use QGraphicsScene, you can call paint() from your item.
// SPDX-License-Identifier: MIT

#pragma once

#include <QtCore>
#include <QtGui>

// -----------------------------------------------------------------------------
// Shared enums (same names as AI for consistency)
// -----------------------------------------------------------------------------
class FighterTypes {
    Q_GADGET
public:
    enum class Action : quint8 {
        Stand, Walk, Crouch, Jump, Kick, SlowPunch, CrouchPunch, AirKick, AirPunch, CrouchBackflipKick, Special, Victory
    }; Q_ENUM(Action)
    enum class Dir : quint8 { Left, Right }; Q_ENUM(Dir)
};
using Action = FighterTypes::Action;
using Dir    = FighterTypes::Dir;

static inline Dir flip(Dir d){ return d==Dir::Left? Dir::Right : Dir::Left; }

// -----------------------------------------------------------------------------
// World/platform data
// -----------------------------------------------------------------------------
// struct Platform { QRectF rect; }; // axis-aligned platform
#include "platform.h"

struct FighterConfig {
    // Physics
    qreal gravity = 1800.0;      // px/s^2
    qreal groundY = 540.0;       // absolute floor if no platform beneath
    qreal walkSpeed = 180.0;     // px/s
    qreal crouchSpeed = 80.0;    // px/s
    qreal jumpSpeed = 1400.0;     // px/s upward
    qreal airControl = 0.9;      // 0..1 multiply walk control in air

    // Jump-rotate
    qreal jumpSpinDegPerSec = 540.0; // spin while airborne

    // Collision box (feet origin at pos)
    QSizeF bodySize = {40, 80};  // width, height

    // Painter scale (global multiplier)
    qreal spriteScale = 1.0;

    // Optional: image base directory; can also come from JSON
    QString basePath;
};

// -----------------------------------------------------------------------------
// Animation data driven by JSON
// -----------------------------------------------------------------------------
struct AnimFrame {
    QPixmap pix;           // loaded image
    int durationMs = 100;  // frame time
    QPointF offset = {0,0};      // world offset from character origin (feet) before draw
    QPointF imageOffset = {0,0}; // additional draw offset in image space
    qreal scale = 1.0;     // per-frame scale multiplier
    qreal rotation = 0.0;  // per-frame extra rotation (degrees)
};

struct Animation {
    QString key;                 // e.g., "Stand_Right"
    QVector<AnimFrame> frames;
    bool loop = true;
};

// JSON schema (example)
// {
//   "imagesBasePath": "assets/fighter/",
//   "spriteScale": 1.0,
//   "physics": { "gravity":1800, "groundY":540, "walkSpeed":180, "jumpSpeed":520, "crouchSpeed":80, "airControl":0.6, "jumpSpinDegPerSec":540, "bodySize":[40,80] },
//   "animations": {
//     "Stand_Right": [ {"image":"stand_0.png","duration":120,"offset":[0,0],"imageOffset":[-24,-72],"scale":1.0,"rotation":0}, ...],
//     "Stand_Left":  [ ... ],
//     "Walk_Right":  [ ... ],
//     "Walk_Left":   [ ... ],
//     "Crouch_Right": [ ... ],
//     "Crouch_Left":  [ ... ],
//     "Jump_Right":   [ ... ],
//     "Jump_Left":    [ ... ],
//     "Kick_Right":   [ ... ],
//     "Kick_Left":    [ ... ],
//     "SlowPunch_Right": [ ... ],
//     "SlowPunch_Left":  [ ... ],
//     "CrouchKick_Right": [ ... ],
//     "CrouchKick_Left":  [ ... ],
//     "AirKick_Right":  [ ... ],
//     "AirKick_Left":   [ ... ],
//     "CrouchBackflipKick_Right": [ ... ],
//     "CrouchBackflipKick_Left":  [ ... ],
//     "Victory_Right": [ ... ],
//     "Victory_Left":  [ ... ]
//   }
// }

// Keys are optional: if you omit "*_Left", the renderer mirrors "*_Right".

// -----------------------------------------------------------------------------
// Fighter class
// -----------------------------------------------------------------------------
class Fighter : public QObject {
    Q_OBJECT
public:
    explicit Fighter(QObject* parent=nullptr) : QObject(parent) {}

    // ----- Loading -----
    bool loadFromJson(const QString& filePath, QString* err=nullptr){
        QFile f(filePath);
        if(!f.open(QIODevice::ReadOnly)){
            if(err) *err = QString("Failed to open %1").arg(filePath);
            return false;
        }
        QJsonParseError pe; auto doc = QJsonDocument::fromJson(f.readAll(), &pe);
        if(pe.error != QJsonParseError::NoError){ if(err) *err = pe.errorString(); return false; }
        if(!doc.isObject()) { if(err) *err = "Root must be object"; return false; }
        const QJsonObject root = doc.object();
        m_cfg.basePath = root.value("imagesBasePath").toString(m_cfg.basePath);
        m_cfg.spriteScale = root.value("spriteScale").toDouble(m_cfg.spriteScale);
        if(root.contains("physics")){
            auto ph = root.value("physics").toObject();
            m_cfg.gravity = ph.value("gravity").toDouble(m_cfg.gravity);
            m_cfg.groundY = ph.value("groundY").toDouble(m_cfg.groundY);
            m_cfg.walkSpeed = ph.value("walkSpeed").toDouble(m_cfg.walkSpeed);
            m_cfg.crouchSpeed = ph.value("crouchSpeed").toDouble(m_cfg.crouchSpeed);
            m_cfg.jumpSpeed = ph.value("jumpSpeed").toDouble(m_cfg.jumpSpeed);
            m_cfg.airControl = ph.value("airControl").toDouble(m_cfg.airControl);
            m_cfg.jumpSpinDegPerSec = ph.value("jumpSpinDegPerSec").toDouble(m_cfg.jumpSpinDegPerSec);
            if(ph.contains("bodySize")){
                auto arr = ph.value("bodySize").toArray();
                if(arr.size()>=2) m_cfg.bodySize = QSizeF(arr.at(0).toDouble(), arr.at(1).toDouble());
            }
        }
        // Animations
        m_anims.clear();
        if(root.contains("actions")){
            const auto anims = root.value("actions").toObject();
            for(auto it = anims.begin(); it != anims.end(); ++it){
                auto io = it->toObject();
                if(io.contains("frames")){
                    const auto frames = io.value("frames").toArray();

                    Animation A; A.key = it.key(); A.loop = true;
                    // const auto frames = it.value().toArray();

                    A.frames.reserve(frames.size());
                    for(const auto& v : frames){
                        const auto fo = v.toObject();
                        AnimFrame fr;
                        const QString imgPath = m_cfg.basePath + fo.value("img").toString();
                        QPixmap px(imgPath);
                        if(px.isNull()){
                            // create placeholder if missing
                            px = QPixmap(32,32); px.fill(Qt::magenta);
                        }
                        fr.pix = px;
                        fr.durationMs = fo.value("dur").toInt(100);
                        if(fo.contains("dx")) fr.offset.setX(fo.value("dx").toDouble());
                        if(fo.contains("dy")) fr.offset.setX(fo.value("dy").toDouble());
                        if(fo.contains("imgOffset")){
                            auto a = fo.value("imgOffset").toArray(); if(a.size()>=2) fr.imageOffset = { a.at(0).toDouble(), a.at(1).toDouble() };
                        }
                        fr.scale = fo.value("imgScale").toDouble(1.0);
                        fr.rotation = fo.value("rot").toDouble(0.0);
                        A.frames.push_back(fr);
                    }
                    m_anims.insert(A.key, A);
                }
            }
        }
        selectAnim(animKeyFor(Action::Stand)); //, Dir::Right));
        return true;
    }

    // ----- State access -----
    QPointF pos() const { return m_pos; }
    QPointF vel() const { return m_vel; }

    bool onGround() const { return m_onGround; }
    bool crouching() const { return m_isCrouching; }
    // bool inAir() const { return m_inAir; }
    // bool isAttacking() const { return m_isAttacking; }

    Dir facing() const { return m_facing; }

    void setPos(const QPointF& p){ m_pos = p; }

    // ----- Control API (can be wired from AI commander) -----
    void turn(Dir d){ if(m_canTurn) m_facing = d; }

    void stop() {
        m_isCrouching = false; m_canCrouch = true; m_canJump = true; m_canTurn = true; m_canMoveHoriz = true;
        m_actionTimerMs = 0;
        m_moveIntent = 0.0;
        if(!m_inAir) {
            m_vel = QPointF(0.0, 0.0);
            setAction(Action::Stand);
        }
    }
    void moveHoriz(qreal axis){
        if(!m_canMoveHoriz) return;
        m_moveIntent = std::clamp(axis, -1.0, 1.0);
        if(onGround() && std::abs(axis)>0.01) {
            m_actionTimerMs = 0;
            setAction(Action::Walk);
        }
    }
    void crouch(){ if(m_onGround && m_canCrouch) { m_isCrouching = true; m_canTurn = true; m_canMoveHoriz = false; setAction(Action::Crouch); m_moveIntent = 0.0; } }
    void jump(){ if(m_onGround && m_canJump) { m_vel.setY(-m_cfg.jumpSpeed); m_inAir = true; m_onGround = false; m_canTurn = true; m_canMoveHoriz = true; m_spin = 0.0; setAction(Action::Jump); } }

    void kick(){ if(m_onGround && !m_isCrouching && m_canAttack) { m_canAttack = false; m_isAttacking = true; m_canJump = false; m_canCrouch = false; m_canTurn = false; m_canMoveHoriz = false; triggerOneShot(Action::Kick, 320); } }
    void slowPunch(){ if(m_onGround && m_canAttack) { m_canAttack = false; m_isAttacking = true; m_canJump = false; m_canCrouch = false; m_canTurn = false; m_canMoveHoriz = false; triggerOneShot(Action::SlowPunch, 1500); } }
    void crouchPunch(){ if(m_onGround && m_isCrouching  && m_canAttack) { m_canAttack = false; m_isAttacking = true; m_canJump = false; m_canCrouch = false; m_canTurn = false; m_canMoveHoriz = false; triggerOneShot(Action::CrouchPunch, 500); } }
    void airKick(){ if(m_inAir && m_canAttack) { m_canAttack = false; m_isAttacking = true; m_canJump = false; m_canCrouch = false; m_canTurn = false; m_canMoveHoriz = false; triggerOneShot(Action::AirKick, 700); } }
    void airPunch(){ if(m_inAir && m_canAttack){ m_canAttack = false; m_isAttacking = true; m_canJump = false; m_canCrouch = false; m_canTurn = false; m_canMoveHoriz = false; triggerOneShot(Action::AirPunch, 500); } }
    void backflipKick(){ if(m_onGround && m_canAttack) { m_canAttack = false; m_isAttacking = true; m_canJump = false; m_canCrouch = false; m_canTurn = false; m_canMoveHoriz = false; triggerOneShot(Action::CrouchBackflipKick, 1700); } }
    void special(){ if(m_onGround && m_canAttack) { m_canAttack = false; m_isAttacking = true; m_canJump = false; m_canCrouch = false; m_canTurn = false; m_canMoveHoriz = false; triggerOneShot(Action::Special, 1600); } }
    void victory(){ triggerOneShot(Action::Victory, 4000, false); }

    // ----- Simulation -----
    void update(qreal dt, const QVector<Platform>& platforms, const QRectF& worldBounds){
        // Horizontal control
        const qreal speed = (m_action==Action::Crouch ? m_cfg.crouchSpeed : m_cfg.walkSpeed);
        const qreal control = m_onGround? 1.0 : m_cfg.airControl;
        // if(m_inAir) m_moveIntent *= 1.0 + 0.1 * dt;
        m_vel.setX(m_moveIntent * speed * control);
        if(m_moveIntent < 0) m_facing = Dir::Left; else if(m_moveIntent>0) m_facing = Dir::Right;

        // Gravity
        m_vel.setY(m_vel.y() + m_cfg.gravity * dt);
        // qDebug() << "vel:" << m_vel;

        // Integrate
        QPointF next = m_pos + m_vel * dt;

        // Platform collisions (simple: check feet crossing top surface)
        bool landed = false; qreal landY = m_cfg.groundY;
        // Find highest platform below next feet that we intersect horizontally and descend onto
        qreal feetX = next.x(); qreal halfW = m_cfg.bodySize.width()/2.0;
        qreal prevFeetY = m_pos.y();
        qreal nextFeetY = next.y();
        // Consider world ground as a platform at groundY
        landY = m_cfg.groundY;
        for(const Platform& p : platforms){
            const QRectF& r = p.rect;
            // If horizontally over platform and moving down past its top
            if( (feetX+halfW) >= r.left() && (feetX-halfW) <= r.right() ){
                if(prevFeetY <= r.top() && nextFeetY >= r.top()){
                    // Candidate landing
                    if(r.top() < landY) landY = r.top();
                }
            }
        }
        if(nextFeetY >= landY){
            next.setY(landY);
            m_vel.setY(0);
            landed = true;
        }
        // Clamp to world bounds horizontally
        next.setX(qBound(worldBounds.left(), next.x(), worldBounds.right()));

        // Commit
        m_pos = next;
        if(landed && (!m_onGround || m_inAir)){ onLanded(); m_moveIntent = 0.0; m_onGround = true; m_inAir = false; }
        // m_onGround = (m_pos.y() >= landY - 0.5);

        // Spin while airborne (jump-rotate)
        if(!m_onGround){ m_spin += m_cfg.jumpSpinDegPerSec * dt; if(m_spin >= 360.0) m_spin = std::fmod(m_spin, 360.0); }
        else { if(m_action==Action::Jump) setAction(Action::Stand); }

        // One-shot action timers
        if(m_actionTimerMs > 0){ m_actionTimerMs -= int(dt*1000.0); if(m_actionTimerMs <= 0){ endOneShot(); }}

        // Advance animation clock
        advanceAnim(int(dt*1000.0));
    }

    // ----- Painting -----
    void paint(QPainter* p) const {
        p->save();
        const AnimFrame* fr = currentFrame();
        if(!fr){ p->restore(); return; }

        // Apply facing via mirroring if we don't have explicit Left animation
        bool mirror = m_facing == Dir::Right; //false;

        const QString desired = animKeyFor(m_action); //, m_facing);
        // if(!m_anims.contains(desired)){
        //     // Mirror from the other side if available
        //     mirror = (m_facing==Dir::Left);
        // }

        // Compute draw transform: feet origin at m_pos
        // Apply world offset then image offset
        QPointF drawPos = m_pos + fr->offset;
        p->translate(drawPos);

        if (mirror) { p->scale(-1, 1); }

        qreal totalRotation = fr->rotation + (m_action==Action::Jump || !m_onGround ? m_spin : 0.0);
        if(totalRotation != 0.0){ p->rotate(totalRotation); }

        // Image space transform
        const qreal s = m_cfg.spriteScale * fr->scale;
        p->translate(fr->imageOffset);
        // if(mirror){ p->scale(-s, s); }
        /*else*/ { p->scale(s, s); }


        // Draw centered at (0,0) unless offsets push it elsewhere
        p->drawPixmap(QPointF(0,0), fr->pix);

        p->restore();
    }

signals:
    void animationChanged(const QString& key);

private:
    // ----- Action/animation handling -----
    QString animKeyFor(Action a) const { //, Dir d) const {
        // auto L = QString(d==Dir::Left?"Left":"Right");
        switch(a){
        case Action::Stand: return QStringLiteral("stand"); // + L;
        case Action::Walk: return QStringLiteral("walk"); //Walk_") + L;
        case Action::Crouch: return QStringLiteral("crouch"); //Crouch_") + L;
        case Action::Jump: return QStringLiteral("jump"); //Jump_") + L;
        case Action::Kick: return QStringLiteral("kick"); //Kick_") + L;
        case Action::SlowPunch: return QStringLiteral("punch"); //SlowPunch_") + L;
        case Action::CrouchPunch: return QStringLiteral("crouch-punch"); //CrouchKick_") + L;
        case Action::AirKick: return QStringLiteral("air-kick"); //AirKick_") + L;
        case Action::AirPunch: return QStringLiteral("air-punch"); //AirKick_") + L;
        case Action::CrouchBackflipKick: return QStringLiteral("back-flip"); //CrouchBackflipKick_") + L;
        case Action::Victory: return QStringLiteral("victory"); //Victory_") + L;
        case Action::Special: return QStringLiteral("special"); //Victory_") + L;
        }
        return QStringLiteral("stand"); //_Right");
    }

    void setAction(Action a){
        if(m_action == a) return;
        m_action = a;
        selectAnim(animKeyFor(m_action)); //, m_facing));
    }

    void triggerOneShot(Action a, int ms, bool returnToStand=true){
        m_returnToStand = returnToStand;
        m_actionTimerMs = ms;
        setAction(a);
        // For airborne specials, keep current vertical state
        if(m_inAir){ /* keep flying */ }
    }

    void endOneShot(){
        if(m_isAttacking) { m_isAttacking = false; m_canAttack = true; }

        m_actionTimerMs = 0;

        if(m_returnToStand){
            if(m_onGround && !m_isCrouching) {
                m_isCrouching = false; m_canCrouch = true; m_canJump = true; m_canTurn = true; m_canMoveHoriz = true;
                setAction(Action::Stand);
            } else if(m_isCrouching) {
                m_canJump = true; m_canTurn = true; m_canMoveHoriz = false;
                setAction(Action::Crouch);
            } else if(m_inAir) {
                m_canMoveHoriz = true;
                setAction(Action::Jump);
            }
        }
    }

    void onLanded(){
        m_spin = 0.0;
        if(m_action==Action::Jump || m_action==Action::AirKick) setAction(Action::Stand);
    }

    void selectAnim(const QString& key){
        qDebug() << key;
        // // Prefer exact key; if missing and key is *Left*, try Right and mirror on paint.
        if(!m_anims.contains(key)){
            // Fallback handled in paint via mirror
            m_animKey = key; m_animIndex = 0; m_animTimeMs = 0; emit animationChanged(key); return;
        }
        m_animKey = key; m_animIndex = 0; m_animTimeMs = 0; emit animationChanged(key);
    }

    const AnimFrame* currentFrame() const {
        // If we don't have frames for current key, try mirrored side
        auto it = m_anims.constFind(m_animKey);
        if(it == m_anims.constEnd()){
            // Try opposite side key
            QString alt = m_animKey;
            alt.replace("Left","Right").replace("Right","Left");
            it = m_anims.constFind(alt);
            if(it == m_anims.constEnd()) return nullptr;
        }
        const Animation& A = it.value();
        if(A.frames.isEmpty()) return nullptr;
        return &A.frames.at(std::clamp(m_animIndex, 0, (int)A.frames.size()-1));
    }

    void advanceAnim(int deltaMs){
        auto it = m_anims.find(m_animKey);
        if(it == m_anims.end()){
            // Advance alt if needed
            QString alt = m_animKey; alt.replace("Left","Right").replace("Right","Left");
            it = m_anims.find(alt);
            if(it == m_anims.end()) return;
        }
        Animation& A = it.value();
        if(A.frames.isEmpty()) return;
        m_animTimeMs += deltaMs;
        while(m_animTimeMs >= A.frames[m_animIndex].durationMs){
            m_animTimeMs -= A.frames[m_animIndex].durationMs;
            if(m_animIndex+1 < A.frames.size()){
                ++m_animIndex;
            } else if(A.loop) {
                m_animIndex = 0;
            } else {
                // Hold on last frame
                m_animIndex = A.frames.size()-1; break;
            }
        }
    }

private:
    // State
    QPointF m_pos {0,0};
    QPointF m_vel {0,0};

    bool m_onGround = false;
    bool m_inAir = true;
    bool m_isCrouching = false;
    bool m_isAttacking = false;
    bool m_isWalking = false;
    bool m_canAttack = true;
    bool m_canTurn = true;
    bool m_canJump = false;
    bool m_canCrouch = false;
    bool m_canMoveHoriz = true;

    Dir m_facing = Dir::Right;

    // Control intents
    qreal m_moveIntent = 0.0; // -1..1

    // Action state
    Action m_action = Action::Stand;
    int m_actionTimerMs = 0;
    bool m_returnToStand = true;

    // Animation
    QHash<QString, Animation> m_anims; // key -> animation
    QString m_animKey;
    int m_animIndex = 0;
    int m_animTimeMs = 0;

    // Spin accumulator for jump-rotate
    qreal m_spin = 0.0;

    // Config
    FighterConfig m_cfg;
};

// -----------------------------------------------------------------------------
// Mini glue with the previous SampleCommander (optional)
// -----------------------------------------------------------------------------
// In your setup, after creating Fighter and SampleCommander:
// commander->onFace = [&](Dir d){ fighter->setFacing(d); };
// commander->onWalk = [&](qreal dir){ fighter->moveHoriz(dir); };
// commander->onStand = [&](){ fighter->stop(); };
// commander->onCrouch = [&](){ fighter->crouch(); };
// commander->onJump = [&](){ fighter->jump(); };
// commander->onKick = [&](){ fighter->kick(); };
// commander->onSlowPunch = [&](){ fighter->slowPunch(); };
// commander->onCrouchKick = [&](){ fighter->crouchKick(); };
// commander->onAirKick = [&](){ fighter->airKick(); };
// commander->onBackflipKick = [&](){ fighter->backflipKick(); };
// commander->onVictory = [&](){ fighter->victory(); };
//
// In your tick:
// fighter->update(dtSeconds, platforms, worldBounds);
//
// In your renderer (e.g. QGraphicsItem::paint or QWidget::paintEvent):
// QPainter p(this); fighter->paint(&p);
