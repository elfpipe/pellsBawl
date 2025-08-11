#include <QtWidgets>
#include <cmath>

#include "fluffyWalk.h"

static const char* kAnimJson = R"JSON(
{
  "meta": { "name": "fluffy_walk_cycle_v3", "version": 3, "durationSec": 1.0, "loop": true },
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
      "baseOffset": { "x": 0, "y": 0 },
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
      "baseOffset": { "x": -30, "y": 120 },
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
      "baseOffset": { "x": 30, "y": 120 },
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
      "baseOffset": { "x": -80, "y": 10 },
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
      "baseOffset": { "x": 80, "y": 10 },
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

void WalkerAnim::paintWalker(QPainter &p, QRect r) {
    // QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
    // p.fillRect(rect(), QColor("#1b1e23"));

    const QPointF center = r.center(); //rect().center();
    const double t = std::fmod(m_clock.elapsed()/1000.0, m_durationSec);

    if (!m_allPixLoaded) {
        drawShadow(p, center + QPointF(0, 150), QSizeF(220, 30), 0.35);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0,0,0,160));
        p.drawRoundedRect(QRectF(r.width()*0.5-240, r.height()*0.5-60, 480, 120), 12, 12);
        p.setPen(Qt::white);
        p.setFont(QFont("Inter,Arial", 11, QFont::DemiBold));
        p.drawText(QRectF(r.width()*0.5-220, r.height()*0.5-40, 440, 80),
                    Qt::AlignCenter,
                    "PNG parts not found.\nPlace: body.png, left_hand.png, right_hand.png, left_foot.png, right_foot.png");
        return;
    }

    struct DrawItem { const Track* tr; QPointF pos; double rotDeg; };
    QVector<DrawItem> items; items.reserve(m_tracks.size());

    // Body (for bob/tilt)
    const Track* body = trackById("body");
    double bodyBobY = 0.0, bodyRot = 0.0, bodyX = 0.0;
    if (body) {
        bodyX    = body->x.eval(t, m_durationSec);
        bodyBobY = body->y.eval(t, m_durationSec);
        bodyRot  = body->rot.eval(t, m_durationSec);
        items.push_back({body,
                            QPointF(center.x() + bodyX + body->baseOffset.x(),
                                    center.y() + bodyBobY + body->baseOffset.y()),
                            bodyRot});
    }

    // Foot capsule params
    const double W = 50.0;     // contact width
    const double H = 30.0;      // swing lift height
    const double duty = 0.30;   // stance fraction
    const double toeDown = -8.0, toeUp = +12.0;

    auto evalFoot = [&](const Track& tr, double phase01)->std::tuple<QPointF,double> {
        double u = std::fmod(t / m_durationSec + phase01, 1.0);
        if (u < 0) u += 1.0;

        if (u < duty) {
            double s = u / duty;
            double x = tr.baseOffset.x() + ( +W/2.0 + (-W) * s );
            double y = tr.baseOffset.y(); // flat
            return { QPointF(center.x() + x, center.y() + bodyBobY + y), toeDown };
        } else {
            double s  = (u - duty) / (1.0 - duty);
            double e  = 0.5 * (1.0 - std::cos(M_PI*s));
            double x  = tr.baseOffset.x() + (-W/2.0 + W * e);
            double y  = tr.baseOffset.y() - ( H * std::sin(M_PI * e) );
            double r  = toeDown * (1.0 - e) + toeUp * e * (1.0 - e) * 4.0;
            return { QPointF(center.x() + x, center.y() + bodyBobY + y), r };
        }
    };

    // Other tracks
    for (const auto& tr : m_tracks) {
        if (tr.id == "body") continue;

        if (tr.id.contains("foot")) {
            double phase = tr.id.contains("left") ? 0.0 : 0.5;
            auto [pos, rdeg] = evalFoot(tr, phase);
            items.push_back({ &tr, pos, rdeg });
        } else {
            double px = tr.baseOffset.x() + tr.x.eval(t, m_durationSec);
            double py = tr.baseOffset.y() + tr.y.eval(t, m_durationSec);
            double rot = tr.rot.eval(t, m_durationSec);
            items.push_back({ &tr, QPointF(center.x() + px, center.y() + bodyBobY + py), rot });
        }
    }

    // Sort by z
    std::stable_sort(items.begin(), items.end(),
                        [](const DrawItem& a, const DrawItem& b){ return a.tr->zOrder < b.tr->zOrder; });

    // Shadow
    drawShadow(p, center + QPointF(0, 150), QSizeF(220, 30), 0.35);

    // Render (with scaling)
    for (const auto& it : items) drawPixmapScaledCentered(p, *it.tr, it.pos, it.rotDeg);

    // Optional ground guide
    p.setPen(QPen(QColor(255,255,255,40), 1, Qt::DashLine));
    p.drawLine(QPointF(0, center.y() + bodyBobY + 70), QPointF(r.width(), center.y() + bodyBobY + 70));
}


void WalkerAnim::loadAnimation() {
    auto doc = QJsonDocument::fromJson(QByteArray(kAnimJson));
    auto root = doc.object();
    m_durationSec = root.value("meta").toObject().value("durationSec").toDouble(1.0);

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
