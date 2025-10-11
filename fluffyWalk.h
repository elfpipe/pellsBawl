#ifndef FLUFFYWALK_H
#define FLUFFYWALK_H

// main.cpp
#include <QtWidgets>
#include <cmath>

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

class WalkerAnim : public QObject {
    Q_OBJECT
public:
    WalkerAnim() {
        loadAnimation();
        selectClip("walk");
        // startTimer(1000/60); // ~60 FPS
        // m_clock.start();
    }

    void paintWalker(QPainter &p, QRectF r, bool turningLeft = false, const double m_animTime = .0);
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
};

// int main(int argc, char** argv) {
//     QApplication app(argc, argv);
//     QApplication::setApplicationName("FluffyWalkQt6 — sized PNG parts");
//     QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
//     QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

//     WalkerWidget w;
//     w.setWindowTitle("Fluffy Walk Cycle (Qt6) — PNG parts + per-part sizes");
//     w.show();
//     return app.exec();
// }

// #include "main.moc"
#endif