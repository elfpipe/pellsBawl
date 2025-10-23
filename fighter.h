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
    qreal spriteScale = 0.8;

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
    bool loadFromJson(const QString& filePath, QString* err=nullptr);

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
        if(m_isAttacking) return;
        m_isCrouching = false; m_canCrouch = true; m_canJump = true; m_canTurn = true; m_canMoveHoriz = true;
        m_canAttack = true;
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
    void update(qreal dt, const QVector<Shape>& platforms, const QRectF& worldBounds);
    // ----- Painting -----
    void paint(QPainter* p) const;
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
