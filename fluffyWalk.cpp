#include <QtWidgets>
#include <cmath>

#include "fluffyWalk.h"

static const char* kAnimJson = R"JSON(
{
  "meta": { "name": "fluffy_walk_cycle_v3", "version": 3, "durationSec": 1.0, "loop": true, "globalScale": 0.7 },
  "rig": {
    "body": { "pivot": "center", "sizeHintPx": 220 },
    "left_hand":  { "pivot": "center" },
    "right_hand": { "pivot": "center" },
    "left_foot":  { "pivot": "center" },
    "right_foot": { "pivot": "center" }
  },
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
}
)JSON";

void WalkerAnim::paintWalker(QPainter &p, QRectF r, bool m_flipHorizontal, const double m_animTime) {
    // QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
    // p.fillRect(rect(), QColor("#1b1e23"));

    const QPointF center = r.center(); //rect().center();
    // const double t = std::fmod(m_clock.elapsed()/1000.0, m_durationSec) * speedConst;
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
    const double W = 120.0, H = 20.0, duty = 0.60;
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
    double flipX = m_flipHorizontal ? -1.0 : 1.0;
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
               QPointF(r.width(), center.y() + bodyBobY * m_globalScale + 70 * m_globalScale));
}


void WalkerAnim::loadAnimation() {
    auto doc = QJsonDocument::fromJson(QByteArray(kAnimJson));
    auto root = doc.object();
    m_durationSec = root.value("meta").toObject().value("durationSec").toDouble(1.0);
    m_globalScale = root.value("meta").toObject().value("globalScale").toDouble(1.0);

    const QJsonArray tracks = root.value("tracks").toArray();
    m_tracks.clear(); m_tracks.reserve(tracks.size());

    for (const auto& tv : tracks) {
        const auto to = tv.toObject();
        Track tr;
        tr.id = to.value("id").toString();
        auto bo = to.value("baseOffset").toObject();
        tr.baseOffset = QPointF(bo.value("x").toDouble(), bo.value("y").toDouble());
        auto props = to.value("properties").toObject();
        tr.x   = Curve::fromJson(props.value("x"));
        tr.y   = Curve::fromJson(props.value("y"));
        tr.rot = Curve::fromJson(props.value("rotationDeg"));
        tr.zOrder = to.value("zOrder").toInt(1);

        // Size (optional)
        tr.desiredSize = readSizeFromJson(to.value("size"));

        // Load "<id>.png"
        const QString fileName = tr.id + ".png";
        QPixmap pm(fileName);
        tr.pix = pm; // may be null; checked after loop
        m_tracks.push_back(std::move(tr));
    }

    // Validate all images present
    m_allPixLoaded = true;
    for (const auto& tr : m_tracks) {
        if (tr.pix.isNull()) { m_allPixLoaded = false; break; }
    }
}
