#include <QtWidgets>
#include <cmath>

#include "pellsBawl.h"

static const char* kAnimJson = R"JSON(
{
  "meta": {
    "name": "fluffy_multi_clips_v1",
    "version": 1,
    "unit": "px",
    "coordinateSpace": "body-local",
    "defaultClip": "walk",
    "globalScale": 1.0
  },
  "rig": {
    "body":       { "pivot": "center", "sizeHintPx": 220 },
    "left_hand":  { "pivot": "center" },
    "right_hand": { "pivot": "center" },
    "left_foot":  { "pivot": "center" },
    "right_foot": { "pivot": "center" }
  },
  "animations": [
    {
      "id": "walk",
      "durationSec": 1.0,
      "loop": true,
      "useFootCapsule": true,
      "tracks": [
        {
          "id": "body",
          "size": { "w": 220, "h": 220 },
          "baseOffset": { "x": 0, "y": -100 },
          "properties": {
            "x": { "type": "const", "value": 0 },
            "y": { "type": "sine", "amp": 6, "phaseDeg": 0, "bias": 0 },
            "rotationDeg": { "type": "sine", "amp": 3, "phaseDeg": 90, "bias": 0 }
          },
          "zOrder": 2
        },
        {
          "id": "left_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": -30, "y": 20 },
          "properties": {
            "x": { "type": "sine", "amp": 30, "phaseDeg": 0, "bias": 0 },
            "y": { "type": "sine", "amp": 8, "phaseDeg": 180, "bias": 2 },
            "rotationDeg": { "type": "sine", "amp": 14, "phaseDeg": 0, "bias": -6 }
          },
          "zOrder": 1
        },
        {
          "id": "right_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": 30, "y": 20 },
          "properties": {
            "x": { "type": "sine", "amp": 30, "phaseDeg": 180, "bias": 0 },
            "y": { "type": "sine", "amp": 8, "phaseDeg": 0, "bias": 2 },
            "rotationDeg": { "type": "sine", "amp": 14, "phaseDeg": 180, "bias": 6 }
          },
          "zOrder": 3
        },
        {
          "id": "left_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": -80, "y": -90 },
          "properties": {
            "x": { "type": "sine", "amp": 18, "phaseDeg": 180, "bias": 0 },
            "y": { "type": "sine", "amp": 6, "phaseDeg": 0, "bias": 0 },
            "rotationDeg": { "type": "sine", "amp": 6, "phaseDeg": 180, "bias": -2 }
          },
          "zOrder": 3
        },
        {
          "id": "right_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": 80, "y": -90 },
          "properties": {
            "x": { "type": "sine", "amp": 18, "phaseDeg": 0, "bias": 0 },
            "y": { "type": "sine", "amp": 6, "phaseDeg": 180, "bias": 0 },
            "rotationDeg": { "type": "sine", "amp": 6, "phaseDeg": 0, "bias": 2 }
          },
          "zOrder": 1
        }
      ]
    },

    {
      "id": "jump_straight",
      "durationSec": 0.9,
      "loop": false,
      "useFootCapsule": false,
      "tracks": [
        {
          "id": "body",
          "size": { "w": 220, "h": 220 },
          "baseOffset": { "x": 0, "y": -50 },
          "properties": {
            "x": { "type": "const", "value": 0 },
            "y": { "type": "sine", "amp": 35, "phaseDeg": 90, "bias": -35 },
            "rotationDeg": { "type": "sine", "amp": 5, "phaseDeg": 0, "bias": 0 }
          },
          "zOrder": 1
        },
        {
          "id": "left_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": -50, "y": 70 },
          "properties": {
            "x": { "type": "const", "value": -50 },
            "y": { "type": "sine", "amp": 12, "phaseDeg": 90, "bias": -10 },
            "rotationDeg": { "type": "sine", "amp": 18, "phaseDeg": 90, "bias": -5 }
          },
          "zOrder": 1
        },
        {
          "id": "right_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": 50, "y": 70 },
          "properties": {
            "x": { "type": "const", "value": 50 },
            "y": { "type": "sine", "amp": 12, "phaseDeg": 90, "bias": -10 },
            "rotationDeg": { "type": "sine", "amp": 18, "phaseDeg": 90, "bias": 5 }
          },
          "zOrder": 1
        },
        {
          "id": "left_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": -50, "y": 10 },
          "properties": {
            "x": { "type": "const", "value": -80 },
            "y": { "type": "sine", "amp": 15, "phaseDeg": 90, "bias": -15 },
            "rotationDeg": { "type": "sine", "amp": 10, "phaseDeg": 90, "bias": -5 }
          },
          "zOrder": 2
        },
        {
          "id": "right_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": 50, "y": 10 },
          "properties": {
            "x": { "type": "const", "value": 80 },
            "y": { "type": "sine", "amp": 15, "phaseDeg": 90, "bias": -15 },
            "rotationDeg": { "type": "sine", "amp": 10, "phaseDeg": 90, "bias": 5 }
          },
          "zOrder": 0
        }
      ]
    },

    {
      "id": "jump_spin",
      "durationSec": 1.0,
      "loop": true,
      "useFootCapsule": false,
      "tracks": [
        {
          "id": "body",
          "size": { "w": 220, "h": 220 },
          "baseOffset": { "x": 0, "y": -50 },
          "properties": {
            "x": { "type": "const", "value": 0 },
            "y": { "type": "sine", "amp": 35, "phaseDeg": 90, "bias": -35 },
            "rotationDeg": { "type": "linear", "start": 0, "end": 360 }
          },
          "zOrder": 1
        },
        {
          "id": "left_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": -45, "y": 68 },
          "properties": {
            "x": { "type": "const", "value": -45 },
            "y": { "type": "sine", "amp": 14, "phaseDeg": 90, "bias": -12 },
            "rotationDeg": { "type": "sine", "amp": 22, "phaseDeg": 90, "bias": -8 }
          },
          "zOrder": 1
        },
        {
          "id": "right_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": 45, "y": 68 },
          "properties": {
            "x": { "type": "const", "value": 45 },
            "y": { "type": "sine", "amp": 14, "phaseDeg": 90, "bias": -12 },
            "rotationDeg": { "type": "sine", "amp": 22, "phaseDeg": 90, "bias": 8 }
          },
          "zOrder": 1
        },
        {
          "id": "left_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": -65, "y": 0 },
          "properties": {
            "x": { "type": "const", "value": -95 },
            "y": { "type": "sine", "amp": 10, "phaseDeg": 90, "bias": -10 },
            "rotationDeg": { "type": "const", "value": -10 }
          },
          "zOrder": 2
        },
        {
          "id": "right_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": 65, "y": 0 },
          "properties": {
            "x": { "type": "const", "value": 95 },
            "y": { "type": "sine", "amp": 10, "phaseDeg": 90, "bias": -10 },
            "rotationDeg": { "type": "const", "value": 10 }
          },
          "zOrder": 0
        }
      ]
    },

    {
      "id": "throw",
      "durationSec": 0.8,
      "loop": false,
      "useFootCapsule": false,
      "tracks": [
        {
          "id": "body",
          "size": { "w": 220, "h": 220 },
          "baseOffset": { "x": 0, "y": -100 },
          "properties": {
            "x": { "type": "sine", "amp": 6, "phaseDeg": 270, "bias": 0 },
            "y": { "type": "sine", "amp": 12, "phaseDeg": 270, "bias": 4 },
            "rotationDeg": { "type": "sine", "amp": 8, "phaseDeg": 270, "bias": 0 }
          },
          "zOrder": 1
        },
        {
          "id": "left_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": -30, "y": 20 },
          "properties": {
            "x": { "type": "const", "value": -58 },
            "y": { "type": "sine", "amp": 4, "phaseDeg": 270, "bias": 2 },
            "rotationDeg": { "type": "sine", "amp": 6, "phaseDeg": 270, "bias": -4 }
          },
          "zOrder": 1
        },
        {
          "id": "right_foot",
          "size": { "w": 120, "h": 80 },
          "baseOffset": { "x": 30, "y": 20 },
          "properties": {
            "x": { "type": "const", "value": 62 },
            "y": { "type": "sine", "amp": 6, "phaseDeg": 270, "bias": 3 },
            "rotationDeg": { "type": "sine", "amp": 8, "phaseDeg": 270, "bias": 6 }
          },
          "zOrder": 1
        },
        {
          "id": "left_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": -80, "y": -90 },
          "properties": {
            "x": { "type": "sine", "amp": 18, "phaseDeg": 270, "bias": -10 },
            "y": { "type": "sine", "amp": 8, "phaseDeg": 270, "bias": 0 },
            "rotationDeg": { "type": "sine", "amp": 12, "phaseDeg": 270, "bias": -6 }
          },
          "zOrder": 2
        },
        {
          "id": "right_hand",
          "size": { "w": 100, "h": 80 },
          "baseOffset": { "x": 80, "y": -90 },
          "properties": {
            "x": { "type": "linear", "start": -30, "end": 100 },
            "y": { "type": "sine", "amp": 10, "phaseDeg": 270, "bias": -4 },
            "rotationDeg": { "type": "linear", "start": -25, "end": 20 }
          },
          "zOrder": 0
        }
      ]
    }
  ],
  "evaluation": {
    "curveTypes": {
      "const": "f(t) = value",
      "sine": "f(t) = amp * sin(2π * t/duration + phaseRad) + bias",
      "linear": "f(t) = start + (end - start) * (t / duration)"
    },
    "notes": "All coordinates are body-local; positive y is down. For jump/throw, set useFootCapsule=false so feet don't walk during non-walk clips."
  }
}
)JSON";
// "baseOffset": { "x": -90, "y": 8 },
// "baseOffset": { "x": 85, "y": 8 },

void PellsBawl::paintWalker(QPainter &p, qreal ground) { //}, QRectF r, bool m_flipHorizontal, const double m_animTime) {
    // shots
    for(auto btw : shots)
        btw->paintCookies(p);

    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

    const QPointF center = playerRect.center();
    const double t = m_animTime;

    // compute animation in BODY-LOCAL space (origin = body center)
    struct DrawItem { const Track* tr; QPointF posLocal; double rotDeg; };
    QVector<DrawItem> items; items.reserve(m_tracks.size());

    const Track* body = trackById("body");
    double bodyBobY = 0.0, bodyRot = 0.0, bodyX = 0.0;
    if (body) {
        bodyX    = body->x.eval(t, m_durationSec);
        bodyBobY = body->y.eval(t, m_durationSec);
        bodyRot  = body->rot.eval(t, m_durationSec);
        items.push_back({ body, QPointF(bodyX + body->baseOffset.x(),
                                        bodyBobY + body->baseOffset.y()),
                          bodyRot });
    }

    // foot path params (unchanged)
    const double W = 60.0, H = 20.0, duty = 0.60;
    const double toeDown = -8.0, toeUp = +12.0;

    auto evalFootLocal = [&](const Track& tr, double phase01)->std::tuple<QPointF,double> {
        double u = std::fmod(t / m_durationSec + phase01, 1.0);
        if (u < 0) u += 1.0;
        if (u < duty) {
            double s = u / duty;
            double x = tr.baseOffset.x() + ( +W/2.0 + (-W) * s );
            double y = tr.baseOffset.y() + bodyBobY; // include body bob in local Y
            return { QPointF(x, y), toeDown };
        } else {
            double s = (u - duty) / (1.0 - duty);
            double e = 0.5 * (1.0 - std::cos(M_PI*s));
            double x = tr.baseOffset.x() + (-W/2.0 + W * e);
            double y = tr.baseOffset.y() + bodyBobY - ( H * std::sin(M_PI * e) );
            double r = toeDown * (1.0 - e) + toeUp * e * (1.0 - e) * 4.0;
            return { QPointF(x, y), r };
        }
    };

    for (const auto& tr : m_tracks) {
        if (tr.id == "body") continue;
        if (tr.id.contains("foot")) {
            double phase = tr.id.contains("left") ? 0.0 : 0.5;
            auto [pos, rdeg] = evalFootLocal(tr, phase);
            items.push_back({ &tr, pos, rdeg });
        } else {
            double px = tr.baseOffset.x() + tr.x.eval(t, m_durationSec);
            double py = tr.baseOffset.y() + bodyBobY + tr.y.eval(t, m_durationSec);
            double rot = tr.rot.eval(t, m_durationSec);
            items.push_back({ &tr, QPointF(px, py), rot });
        }
    }

    std::stable_sort(items.begin(), items.end(),
        [](const DrawItem& a, const DrawItem& b){ return a.tr->zOrder < b.tr->zOrder; });

    // === global transform: translate to center, then scale the whole character ===
    p.save();
    double flipX = isFacingLeft ? -1.0 : 1.0;
    p.translate(center);
    p.scale(flipX * m_globalScale, m_globalScale);

    // draw shadow INSIDE the scaled space if you want it to scale with the character:
    // drawShadow(p, QPointF(0, 50), QSizeF(220, 30), 0.35);

    // draw parts at local positions
    for (const auto& it : items)
        drawPixmapScaledCentered(p, *it.tr, it.posLocal, it.rotDeg);

    p.restore();

    // (Alternative) If you want the shadow to stay constant size regardless of character scale,
    // comment the shadow call above and use this unscaled one:
    // drawShadow(p, center + QPointF(0, 100), QSizeF(220, 30), 0.35);

    // ground guide (unscaled world)
    p.setPen(QPen(QColor(255,255,255,40), 1, Qt::DashLine));
    p.drawLine(QPointF(0, center.y() + bodyBobY * m_globalScale + 70 * m_globalScale),
               QPointF(playerRect.width(), center.y() + bodyBobY * m_globalScale + 70 * m_globalScale));

    drawShadow(p, QPointF(playerRect.center().x(), ground + 12), QSizeF(120, 17), 0.35);
}


void PellsBawl::loadAnimation() {
    m_clips.clear();
    m_tracks.clear();
    m_pixById.clear();
    m_allPixLoaded = false;

    // Parse root
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(kAnimJson));
    const QJsonObject root  = doc.object();
    const QJsonObject meta  = root.value("meta").toObject();
    const QString defaultClipId = meta.value("defaultClip").toString();

    m_globalScale = meta.value("globalScale").toDouble(1.0);

    // --- Collect defaults ----------------------------------------------------
    // Part size defaults from "parts":[{id,size}]
    QHash<QString, QSizeF> sizeDefaults;
    QSet<QString> allIds;
    if (root.contains("parts")) {
        const auto parts = root.value("parts").toArray();
        for (const auto& pv : parts) {
            const auto po = pv.toObject();
            const QString id = po.value("id").toString();
            if (id.isEmpty()) continue;
            allIds.insert(id);
            QSizeF sz = readSizeFromJson(po.value("size")); // optional
            if (sz.isValid() && !sz.isEmpty()) sizeDefaults.insert(id, sz);
        }
    }

    // baseOffset & zOrder defaults from "defaults"
    QHash<QString, QPointF> baseDefaults;
    QHash<QString, int>     zDefaults;
    if (root.contains("defaults")) {
        const auto defs = root.value("defaults").toObject();
        // baseOffsets: { "id": {x,y}, ... }
        const auto bo = defs.value("baseOffsets").toObject();
        for (auto it = bo.begin(); it != bo.end(); ++it)
            baseDefaults.insert(it.key(), readPoint(it.value()));
        // zOrder: { "id": 1, ... }
        const auto zo = defs.value("zOrder").toObject();
        for (auto it = zo.begin(); it != zo.end(); ++it)
            zDefaults.insert(it.key(), it.value().toInt(1));
    }

    // --- Peek animations to gather any missing part IDs (if "parts" omitted) -
    const QJsonArray anims = root.value("animations").toArray();
    for (const auto& av : anims) {
        const auto ao = av.toObject();
        const auto tracks = ao.value("tracks").toArray();
        for (const auto& tv : tracks) {
            const auto to = tv.toObject();
            const QString id = to.value("id").toString();
            if (!id.isEmpty()) allIds.insert(id);
        }
    }

    // --- Load PNGs once per part id -----------------------------------------
    for (const auto& id : allIds) {
        QPixmap pm(":/assets/pb/" + id + ".png");
        m_pixById.insert(id, pm); // (may be null; we validate below)
    }
    m_allPixLoaded = true;
    for (auto it = m_pixById.begin(); it != m_pixById.end(); ++it) {
        if (it.value().isNull()) { m_allPixLoaded = false; break; }
    }

    // --- Build clips ---------------------------------------------------------
    QString firstClipId; // for fallback if no defaultClip provided
    for (const auto& av : anims) {
        const auto ao = av.toObject();

        AnimClip clip;
        clip.id            = ao.value("id").toString();
        clip.durationSec   = ao.value("durationSec").toDouble(1.0);
        clip.loop          = ao.value("loop").toBool(true);
        clip.useFootCapsule= ao.value("useFootCapsule").toBool(false);

        if (firstClipId.isEmpty() && !clip.id.isEmpty()) firstClipId = clip.id;

        const auto tracks = ao.value("tracks").toArray();
        clip.tracks.reserve(tracks.size());

        for (const auto& tv : tracks) {
            const auto to = tv.toObject();
            Track tr;
            tr.id = to.value("id").toString();
            if (tr.id.isEmpty()) continue;

            // Size: per-track or default from "parts"
            QSizeF sizeFromTrack = readSizeFromJson(to.value("size"));
            if (sizeFromTrack.isValid() && !sizeFromTrack.isEmpty())
                tr.desiredSize = sizeFromTrack;
            else if (sizeDefaults.contains(tr.id))
                tr.desiredSize = sizeDefaults.value(tr.id); // optional

            // baseOffset: track override -> defaults -> (0,0)
            if (to.contains("baseOffset"))
                tr.baseOffset = readPoint(to.value("baseOffset"));
            else if (baseDefaults.contains(tr.id))
                tr.baseOffset = baseDefaults.value(tr.id);
            else
                tr.baseOffset = QPointF(0,0);

            // zOrder: track override -> defaults -> 1
            tr.zOrder = to.value("zOrder").toInt(
                            zDefaults.contains(tr.id) ? zDefaults.value(tr.id) : 1);

            // Curves (x,y,rotationDeg). If missing, default to const 0.
            const auto props = to.value("properties").toObject();
            tr.x   = Curve::fromJson(props.value("x"));
            tr.y   = Curve::fromJson(props.value("y"));
            tr.rot = Curve::fromJson(props.value("rotationDeg"));
            if (props.isEmpty()) {
                // ensure stable defaults when properties absent
                tr.x.type = Curve::Const; tr.x.value = 0.0;
                tr.y.type = Curve::Const; tr.y.value = 0.0;
                tr.rot.type = Curve::Const; tr.rot.value = 0.0;
            } else {
                // fill any missing channel
                if (!props.contains("x"))   { tr.x.type=Curve::Const; tr.x.value=0.0; }
                if (!props.contains("y"))   { tr.y.type=Curve::Const; tr.y.value=0.0; }
                if (!props.contains("rotationDeg")) { tr.rot.type=Curve::Const; tr.rot.value=0.0; }
            }

            // Assign pixmap (Qt copies are cheap and shared)
            tr.pix = m_pixById.value(tr.id);

            clip.tracks.push_back(std::move(tr));
        }

        if (!clip.id.isEmpty())
            m_clips.insert(clip.id, std::move(clip));
    }

    // --- Select default/first clip ------------------------------------------
    QString toSelect = defaultClipId.isEmpty() ? firstClipId : defaultClipId;
    if (toSelect.isEmpty() && !m_clips.isEmpty())
        toSelect = m_clips.begin().key(); // fallback

    if (!toSelect.isEmpty()) {
        selectClip(toSelect);
    } else {
        // final fallback: keep empty but sane
        m_durationSec = 1.0;
        m_useFootCapsule = false;
        m_activeClipId.clear();
    }
}
