// Fighter.hpp — Qt6 single-file character container + sprite animator
// Implements the Fighter character with physics (ground & platforms), actions,
// JSON-driven animations, and painting with scaling & rotation.
//
// Designed to pair with the previously provided FighterAI / SampleCommander.
// Drop this header in your project and include it in exactly one TU.
//
// Build: Qt 6 (Core, Gui). If you use QGraphicsScene, you can call paint() from your item.
// SPDX-License-Identifier: MIT

#include <QtCore>
#include <QtGui>

#include "fighter.h"


// -----------------------------------------------------------------------------
// Fighter class
// -----------------------------------------------------------------------------

// ----- Loading -----
bool Fighter::loadFromJson(const QString& filePath, QString* err){
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

// ----- Simulation -----
void Fighter::update(qreal dt, const QVector<Platform>& platforms, const QRectF& worldBounds){
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
void Fighter::paint(QPainter* p) const {
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
    p->scale(m_cfg.spriteScale, m_cfg.spriteScale);
    const qreal s = fr->scale;
    p->translate(fr->imageOffset);
    // if(mirror){ p->scale(-s, s); }
    /*else*/ { p->scale(s, s); }


    // Draw centered at (0,0) unless offsets push it elsewhere
    p->drawPixmap(QPointF(0,0), fr->pix);

    p->restore();
}
