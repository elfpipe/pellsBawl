#include <QtCore>

#include "fighter.h"
#include "fighterAI.h"

Intent FighterAI::decide(){
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
bool FighterAI::willProjectileHitWithin(qreal t, Dir& fromDirOut) const {
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
