// FighterAI.hpp — single-file Qt6/C++ utility-FSM AI for a 2D platformer
// Drop into your project and include in exactly one translation unit.
// The AI controls a Fighter character vs. a Cloud opponent.
// Fighter supports actions: Stand, Walk, Crouch, Jump, Kick, SlowPunch (specialty),
// CrouchKick, AirKick, CrouchBackflipKick, Victory — each in {Left, Right}.
// Opponent (Cloud) supports: Walk, Jump, Shoot — each in {Left, Right}.
//
// You will still need to hook these intents into your animation/physics layers.
// The AI exposes signals for intents and reads a lightweight blackboard snapshot.
//
// Build: Qt 6.x (Core). No STL RNG reliance; uses QRandomGenerator.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QtCore>

#include "fighter.h"
#include "combo.h"

// ------------------------------
// Shared enums and small types
// ------------------------------
class FighterEnums {
    Q_GADGET
public:
    enum class Action : quint8 {
        Stand, Walk, Crouch, Jump, Kick, SlowPunch, CrouchPunch, AirKick, AirPunch, CrouchBackflipKick, Special, Victory
    }; Q_ENUM(Action)

    enum class Dir : quint8 { Left, Right }; Q_ENUM(Dir)
};

// using Action = FighterEnums::Action;
// using Dir    = FighterEnums::Dir;

// Helper to flip direction
static inline Dir flipped(Dir d){ return d==Dir::Left? Dir::Right : Dir::Left; }

// ------------------------------
// Per-frame sensory snapshot (read-only to AI)
// ------------------------------
struct ProjectileInfo {
    QPointF pos {0,0};
    QPointF vel {0,0};
    qreal radius = 6.0; // for simple collision prediction
};

struct OpponentSnapshot {
    QPointF pos {0,0};
    QPointF vel {0,0};
    bool onGround = true;
    bool isShooting = false; // transient when a shot is fired this frame
    Dir facing = Dir::Right;
    QList<ProjectileInfo> activeProjectiles; // cloud shots
};

struct SelfSnapshot {
    QPointF pos {0,0};
    QPointF vel {0,0};
    bool onGround = true;
    Dir facing = Dir::Right;
    qreal health = 100.0;
    qreal stamina = 100.0; // basic gating for specials
};

struct WorldSnapshot {
    qreal gravity = 1800.0;   // px/s^2; used for air-time heuristics
    QRectF walkableBounds;    // to avoid walking off-screen
    qreal timeSeconds = 0.0;  // world time for seeded randomness
};

// ------------------------------
// Control intents (AI outputs)
// ------------------------------
struct Intent {
    Action action = Action::Stand;
    Dir dir = Dir::Right;
};

// ------------------------------
// Abstract interface to your character command layer
// ------------------------------
class IFighterCommander {
public:
    virtual ~IFighterCommander() = default;
    // Called when AI decides on a high-level action for this frame.
    virtual void applyIntent(const Intent& i) = 0;
    // Optional: invoked when AI wants to trigger specific animation tags/events
    virtual void onComboTag(const QByteArray& tag) { Q_UNUSED(tag); }
};

// ------------------------------
// Config knobs (tune per game feel)
// ------------------------------
struct FighterAIConfig {
    // Ranges (pixels)
    qreal engageRangeMin = 80.0;     // start engaging if closer than this
    qreal engageRangeMax = 280.0;    // if farther, prefer approach
    qreal sweetSpot = 120.0;         // ideal striking distance

    // Cooldowns (seconds)
    qreal cdKick = 0.5;
    qreal cdSlowPunch = 0.65;
    qreal cdCrouchKick = 0.7;
    qreal cdAirKick = 0.4;
    qreal cdAirPunch = 0.3;
    qreal cdBackflipKick = 0.7;
    qreal cdSpecial = 0.6;
    qreal cdJump = 0.35;

    // Threat response windows (seconds)
    qreal dodgeReaction = 0.12;      // minimum predictive time to react to a projectile

    // Preferences
    qreal aggression = 0.90;         // 0..1 (higher -> favor attacks)
    qreal flamboyance = 0.65;        // chance to use backflip kick when safe
    qreal patience = 0.35;           // higher -> wait/feint more

    // Stamina costs
    qreal costSlowPunch = 18.0;
    qreal costBackflip = 12.0;
    qreal costSpecial = 20.0;

    // Movement
    qreal walkBiasJitter = 0.35;     // randomness when choosing left/right walk while circling

    // Safety
    qreal edgeBuffer = 24.0;         // avoid edges by this many pixels
};

// ------------------------------
// Internal timers helper
// ------------------------------
class Cooldowns {
public:
    void set(const QByteArray& key, qreal seconds) { m_until[key] = m_now + seconds; }
    bool ready(const QByteArray& key) const {
        bool result = m_now >= m_until.value(key, 0.0);
        // qDebug() << "(now :" << m_now << "," << m_until.value(key, 0.0) << ")" << key << ":" << result;
        return result;
    }
    void tick(qreal dt){ m_now += dt; }
private:
    QHash<QByteArray, qreal> m_until; // key -> world time when ready
    qreal m_now = 0.0;
};

// ------------------------------
// Fighter AI Controller (utility-scored finite state machine)
// ------------------------------
class FighterAI : public QObject {
    Q_OBJECT
public:
    explicit FighterAI(QObject* parent=nullptr)
        : QObject(parent) {}

    void setCommander(IFighterCommander* c) { m_cmd = c; }

    void setConfig(const FighterAIConfig& c) { m_cfg = c; }

    void seed(quint32 s){ m_rng.seed(s); }

    // Provide fresh snapshots every tick before update()
    void sense(const SelfSnapshot& self, const OpponentSnapshot& opp, const WorldSnapshot& world){
        m_self = self; m_opp = opp; m_world = world; m_cd.tick(world.timeSeconds);
    }

    // Main update — call at 60 Hz (or your tick), after sense().
    void update(){
        if(!m_cmd) return;
        Intent intent = decide();
        clampToBounds(intent);
        m_cmd->applyIntent(intent);
    }

signals:
    void debugChosen(const QString& reason, const Intent& intent);

private:
    // -------------- Decision core --------------
    Intent decide(){
        // Victory check (very simple — plug your actual condition)
        if (m_opp.activeProjectiles.isEmpty() && m_opp.vel.manhattanLength()==0 && m_self.health > 0 && m_opp.pos.y() > 99999){
            return { Action::Victory, m_self.facing };
        }

        // Predict nearest projectile threat
        bool imminentThreat = willProjectileHitWithin(m_cfg.dodgeReaction, m_threatDir);

        const qreal dx = m_opp.pos.x() - m_self.pos.x();
        const qreal dist = std::abs(dx);
        const Dir toOpp = dx < 0 ? Dir::Left : Dir::Right;

        // Utility buckets
        struct Option { Intent intent; qreal score; QString why; } best{{Action::Stand, m_self.facing}, -1e9, "init"};
        auto tryPick = [&](Action a, Dir d, qreal score, QString why){ if(score>best.score){ best = { {a,d}, score, std::move(why) }; } };

        // Base bias: keep facing opponent
        Dir face = toOpp;

        // 1) Reactive layer — dodge if imminent projectile
        if(imminentThreat && m_self.onGround && m_cd.ready("jump")){
            qreal sJump = 10.0; // high priority
            tryPick(Action::Jump, face, sJump, "dodge: jump over projectile");
        }
        if(imminentThreat && m_self.onGround){
            // Crouch if jump is on cooldown or projectile is high
            qreal sCrouch = 8.5;
            tryPick(Action::Crouch, face, sCrouch, "dodge: crouch under projectile");
        }

        // 2) Positioning — approach or kite to sweet spot
        if(dist > m_cfg.engageRangeMax){
            // Approach
            qreal s = 3.0 + 2.0*m_cfg.aggression + jitter(0.0, 0.4);
            tryPick(Action::Walk, face, s, "approach target");
        } else if(dist < m_cfg.engageRangeMin){
            // Back off slightly (circle)
            Dir away = flipped(face);
            qreal s = 2.0 + jitter(0.0,0.3);
            tryPick(Action::Walk, away, s, "give space");
        } else {
            // Within engage window — micro-adjust
            qreal delta = std::abs(dist - m_cfg.sweetSpot);
            if(delta > 12.0){
                Dir d = (dist < m_cfg.sweetSpot) ? flipped(face) : face;
                qreal s = 2.2 + jitter(0.0,0.2);
                tryPick(Action::Walk, d, s, "sweet-spot tune");
            } else {
                tryPick(Action::Stand, face, 2.0 + m_cfg.patience, "hold position");
            }
        }

        // 3) Offensive choices — gated by cooldowns & stamina
        bool canJump = m_cd.ready("jump");
        bool canKick = m_cd.ready("kick");
        bool canSlowPunch = m_cd.ready("slowpunch") && (m_self.stamina >= m_cfg.costSlowPunch);
        bool canCrouchPunch = m_cd.ready("crouchpunch");
        bool canAirKick = m_cd.ready("airkick");
        bool canAirPunch = m_cd.ready("airpunch");
        bool canBackflipKick = m_cd.ready("backflip") && (m_self.stamina >= m_cfg.costBackflip);
        bool canSpecial = m_cd.ready("special") && (m_self.stamina >= m_cfg.costSpecial);

        if(dist < m_cfg.engageRangeMax){
            // SlowPunch (special): best when opponent grounded & not moving fast
            if(canSlowPunch && m_self.onGround && m_opp.onGround && std::abs(m_opp.vel.x()) < 60.0){
                qreal s = 6.0 + 5.0*m_cfg.aggression + jitter(-0.3,0.3);
                tryPick(Action::SlowPunch, face, s, "special slow punch window");
            }
            // Kick: bread & butter
            if(canKick && m_self.onGround && dist <= m_cfg.sweetSpot + 10.0){
                qreal s = 5.5 + 3.0*m_cfg.aggression + jitter(-0.25,0.25);
                tryPick(Action::Kick, face, s, "close kick");
            }
            // Crouch kick: when close and expecting shot or low-profile trade
            if(canCrouchPunch && m_self.onGround && dist <= m_cfg.sweetSpot){
                qreal s = 4.8 + 2.0*m_cfg.aggression + (imminentThreat?1.0:0.0) + jitter(-0.2,0.2);
                tryPick(Action::CrouchPunch, face, s, "low crouch punch");
            }
            // Air kick: jump then strike if we need to vault shots or close vertical gap
            if(canAirKick && canJump && dist <= m_cfg.engageRangeMax && !m_self.onGround){
                qreal s = 5.0 + jitter(-0.2,0.2);
                tryPick(Action::AirKick, face, s, "air kick follow-up");
            }
            // Air punch: jump then strike if we need to vault shots or close vertical gap
            if(canAirPunch && canJump && dist <= m_cfg.engageRangeMax && !m_self.onGround){
                qreal s = 4.0 + jitter(-0.2,0.2);
                tryPick(Action::AirPunch, face, s, "air punch follow-up");
            }
            // Backflip kick: style finisher when safe and with stamina
            if(canBackflipKick && m_self.onGround && dist <= m_cfg.sweetSpot + 25.0 && randChance(m_cfg.flamboyance)){
                qreal s = 5.2 + jitter(-0.2,0.2);
                tryPick(Action::CrouchBackflipKick, face, s, "flamboyant backflip kick");
            }
            // Backflip kick: style finisher when safe and with stamina
            if(canSpecial && m_self.onGround && dist <= m_cfg.sweetSpot + 25.0 && randChance(m_cfg.flamboyance)){
                qreal s = 5.9 + jitter(-0.2,0.2);
                tryPick(Action::Special, face, s, "flamboyant special magic");
            }
        }

        // 4) Opportunistic jump to set up air kick (if we picked Jump earlier, that wins)
        if(canJump && m_self.onGround && dist <= m_cfg.engageRangeMax && (imminentThreat || randChance(0.013))){
            qreal s = 3.8 + jitter(-0.2,0.2);
            tryPick(Action::Jump, face, s, imminentThreat?"jump to avoid + approach":"mixup jump");
        }

        // Commit
        applyCooldowns(best.intent.action);
        faceEdgeSafety(best.intent);
        emit debugChosen(best.why, best.intent);
        return best.intent;
    }

    // Predict if any projectile will intersect our x-range within t seconds.
    bool willProjectileHitWithin(qreal t, Dir& fromDirOut) const {
        const QPointF P = m_self.pos;
        for(const auto& p : m_opp.activeProjectiles){
            // Simple forward projection assuming linear motion
            QPointF future = p.pos + p.vel * t;
            // Consider horizontal pass near our x and y proximity
            bool crossesX = ((p.pos.x()-P.x()) * (future.x()-P.x())) <= 0; // sign change -> passes our x
            qreal avgY = 0.5*(p.pos.y()+future.y());
            qreal yGap = std::abs(avgY - P.y());
            if(crossesX && yGap <= (24.0 + p.radius)){
                fromDirOut = (p.vel.x() >= 0) ? Dir::Left : Dir::Right; // coming from left if moving right
                return true;
            }
        }
        return false;
    }

    // Apply cooldowns for chosen action
    void applyCooldowns(Action a){
        auto set = [&](const char* k, qreal s){ m_cd.set(k, s); };
        switch(a){
        case Action::Kick:              set("kick", m_cfg.cdKick); break;
        case Action::SlowPunch:         set("slowpunch", m_cfg.cdSlowPunch); m_self.stamina = qMax(0.0, m_self.stamina-m_cfg.costSlowPunch); break;
        case Action::CrouchPunch:        set("crouchpunch", m_cfg.cdCrouchKick); break;
        case Action::AirKick:           set("airkick", m_cfg.cdAirKick); break;
        case Action::AirPunch:           set("airpunch", m_cfg.cdAirPunch); break;
        case Action::CrouchBackflipKick:    set("backflip", m_cfg.cdBackflipKick); m_self.stamina = qMax(0.0, m_self.stamina-m_cfg.costBackflip); break;
        case Action::Special:           set("special", m_cfg.cdSpecial); m_self.stamina = qMax(0.0, m_self.stamina-m_cfg.costSpecial); break;
        case Action::Jump:              set("jump", m_cfg.cdJump); break;
        default: break;
        }
    }

    // Keep intent within world bounds and avoid edges
    void clampToBounds(Intent& i){
        Q_UNUSED(i);
        // Here you could override walking intent if within edgeBuffer of bounds
        // to stop from walking out of screen. This demo defers to faceEdgeSafety().
    }

    void faceEdgeSafety(Intent& i){
        if(i.action != Action::Walk) return;
        const QRectF& B = m_world.walkableBounds;
        qreal x = m_self.pos.x();
        if(i.dir == Dir::Left && (x - B.left()) < m_cfg.edgeBuffer){
            i.dir = Dir::Right; // bounce off edge
        } else if(i.dir == Dir::Right && (B.right() - x) < m_cfg.edgeBuffer){
            i.dir = Dir::Left;
        }
    }

    // Jitter helpers
    qreal jitter(qreal a, qreal b) { return QRandomGenerator::global()->bounded(10000) / 10000.0f * (b - a) + a; } //bounded((int)a,(int)b); }
    bool randChance(qreal p) { return QRandomGenerator::global()->generateDouble() < p; }

private:
    IFighterCommander* m_cmd = nullptr;
    FighterAIConfig m_cfg;
    SelfSnapshot m_self;
    OpponentSnapshot m_opp;
    WorldSnapshot m_world;
    Cooldowns m_cd;
    Dir m_threatDir = Dir::Left;
    QRandomGenerator m_rng { 12345u };
};

// ------------------------------
// Example glue: minimal commander that maps intents to your animation/physics
// ------------------------------
class SampleCommander : public QObject, public IFighterCommander {
    Q_OBJECT
public:
    explicit SampleCommander(QObject* parent=nullptr, Fighter *newFighter = nullptr) : QObject(parent) {
        this->fighter = newFighter;

        // In your setup, after creating Fighter and SampleCommander:
        onFace = [&](Dir d){ fighter->turn(d); };
        onWalk = [&](qreal dir){ fighter->moveHoriz(dir); };
        onStand = [&](){ fighter->stop(); };
        onCrouch = [&](){ fighter->crouch(); };
        onJump = [&](){ fighter->jump(); };
        onKick = [&](){ fighter->kick(); };
        onSlowPunch = [&](){ fighter->slowPunch(); };
        onCrouchPunch = [&](){ fighter->crouchPunch(); };
        onAirKick = [&](){ fighter->airKick(); };
        onAirPunch = [&](){ fighter->airPunch(); };
        onBackflipKick = [&](){ fighter->backflipKick(); };
        onSpecial = [&](){ fighter->special(); };
        onVictory = [&](){ fighter->victory(); };
    }

    // Wire these to your character controller
    std::function<void(Dir)> onFace;
    std::function<void(qreal)> onWalk; // negative for left, positive for right
    std::function<void()> onStand;
    std::function<void()> onCrouch;
    std::function<void()> onJump;
    std::function<void()> onKick;
    std::function<void()> onSlowPunch;
    std::function<void()> onCrouchPunch;
    std::function<void()> onAirKick;
    std::function<void()> onAirPunch;
    std::function<void()> onBackflipKick;
    std::function<void()> onVictory;
    std::function<void()> onSpecial;

    void applyIntent(const Intent& i) override {

        if(onFace) onFace(i.dir);
        switch(i.action){
        case Action::Stand: qDebug() << "Stand"; if(onStand) onStand(); break;
        case Action::Walk:  if(onWalk) onWalk(i.dir==Dir::Left? -1.0 : 1.0); break;
        case Action::Crouch:qDebug() << "Crouch"; if(onCrouch) onCrouch(); break;
        case Action::Jump:  qDebug() << "Jump"; if(onJump) onJump(); break;
        case Action::Kick:  qDebug() << "Kick"; if(onKick) onKick(); break;
        case Action::SlowPunch: qDebug() << "SlowPunch"; if(onSlowPunch) onSlowPunch(); break;
        case Action::CrouchPunch: qDebug() << "CrouchPunch"; if(onCrouchPunch) onCrouchPunch(); break;
        case Action::AirKick: qDebug() << "AirKick"; if(onAirKick) onAirKick(); break;
        case Action::AirPunch: qDebug() << "AirPunch"; if(onAirPunch) onAirPunch(); break;
        case Action::CrouchBackflipKick: qDebug() << "BackFlipKick"; if(onBackflipKick) onBackflipKick(); break;
        case Action::Special: qDebug() << "Special"; if(onSpecial) onSpecial(); break;
        case Action::Victory: qDebug() << "Victory"; if(onVictory) onVictory(); break;
        }
    }

    bool applyCombo(QVector<Combo::Key> combo) {
        bool resetCombo = false;
        if(combo.size() >= 3) {
            QVector<Combo::Key> sub = combo.last(3);
            if(sub.last() == Combo::FIRE1) {
                if(sub[0] == Combo::DOWN && sub[1] == Combo::DOWN) {
                    fighter->backflipKick();
                    resetCombo = true;
                }
            }
            if(sub.last() == Combo::FIRE2) {
                if(sub[0] == Combo::DOWN && sub[1] == Combo::DOWN) {
                    fighter->special();
                    resetCombo = true;
                }
            }
        }
        if(!resetCombo & !combo.empty()) {
            switch(combo.last()) {
                case Combo::UP:
                    fighter->jump();
                    break;
                case Combo::DOWN:
                    fighter->crouch();
                    break;
                case Combo::LEFT:
                    fighter->moveHoriz(-1.0);
                    break;
                case Combo::RIGHT:
                    fighter->moveHoriz(1.0);
                    break;
                case Combo::FIRE1:
                    if(!fighter->onGround()) fighter->airKick();
                    else if(fighter->crouching()) fighter->crouchPunch();
                    else fighter->kick();
                    break;
                case Combo::FIRE2:
                    if(!fighter->onGround()) fighter->airPunch();
                    else if(fighter->crouching()) fighter->crouchPunch();
                    else fighter->slowPunch();
                    break;
            }
        }
        if(combo.isEmpty()) {
            fighter->stop();
        }

        return resetCombo;
    }

private:
    Fighter *fighter = nullptr;
};

// ------------------------------
// Quick-start wiring example (drop into your scene setup)
// ------------------------------
/*
// In your level setup code:

auto* ai = new FighterAI(this);
auto* commander = new SampleCommander(this);
ai->setCommander(commander);

// Bind to your character systems (pseudo-code):
commander->onFace = [&](Dir d){ character->setFacing(d==Dir::Left? -1:1); };
commander->onWalk = [&](qreal dir){ character->moveHoriz(dir * character->walkSpeed()); };
commander->onStand = [&](){ character->stop(); };
commander->onCrouch = [&](){ character->crouch(); };
commander->onJump = [&](){ character->jump(); };
commander->onKick = [&](){ character->playAnim("kick"); character->tryHitbox("kick"); };
commander->onSlowPunch = [&](){ character->playAnim("slow_punch"); character->tryHitbox("slowpunch"); };
commander->onCrouchKick = [&](){ character->playAnim("crouch_kick"); character->tryHitbox("crouchkick"); };
commander->onAirKick = [&](){ character->playAnim("air_kick"); character->tryHitbox("airkick"); };
commander->onBackflipKick = [&](){ character->playAnim("crouch_backflip_kick"); character->tryHitbox("backflip"); };
commander->onVictory = [&](){ character->playAnim("victory"); };

QObject::connect(ai, &FighterAI::debugChosen, this, [](const QString& why, const Intent& i){
    qDebug() << "AI:" << why << "->" << (int)i.action << (i.dir==Dir::Left?"L":"R");
});

// In your game loop/tick (e.g., QTimer at 60 Hz):
QTimer* tick = new QTimer(this);
QObject::connect(tick, &QTimer::timeout, this, [=](){
    SelfSnapshot self { character->pos(), character->vel(), character->onGround(), character->facing(), character->health(), character->stamina() };
    OpponentSnapshot opp { cloud->pos(), cloud->vel(), cloud->onGround(), cloud->didShootThisFrame(), cloud->facing(), cloud->projectiles() };
    WorldSnapshot world { physics->gravity(), sceneBounds, gameClock->seconds() };
    ai->sense(self, opp, world);
    ai->update();
});

// Note: The AI emits intents; you handle movement/animation accordingly.
*/
