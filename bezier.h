#ifndef BEZIER_H
#define BEZIER_H

#include <QtWidgets>
#include <cmath>

#include "platform.h"
// =========================== Bézier helpers ===========================
static inline QPointF bezierPoint(double t, const QPointF& P0, const QPointF& P1, const QPointF& P2) {
    const double u = 1.0 - t;
    return QPointF(u*u*P0.x() + 2*u*t*P1.x() + t*t*P2.x(),
                   u*u*P0.y() + 2*u*t*P1.y() + t*t*P2.y());
}

static inline QPointF bezierTangent(double t, const QPointF& P0, const QPointF& P1, const QPointF& P2) {
    return QPointF(2*(1-t)*(P1.x()-P0.x()) + 2*t*(P2.x()-P1.x()),
                   2*(1-t)*(P1.y()-P0.y()) + 2*t*(P2.y()-P1.y()));
}

static inline QPointF makeControlPoint(const QPointF& P0, const QPointF& P2, double arcHeight, double skew01) {
    const double cx = P0.x() + (P2.x() - P0.x()) * qBound(0.0, skew01, 1.0);
    const double cy = 0.5*(P0.y() + P2.y()) - arcHeight; // y-down, so minus is up
    return QPointF(cx, cy);
}

// =========================== Widget demo =============================
class BezierThrowWidget : public QWidget {
    Q_OBJECT
public:
    BezierThrowWidget(QWidget* parent=nullptr)
        : QWidget(parent)
    {
        setWindowTitle("pellsBawl — Charged Bézier throw (pure C++)");
        resize(920, 560);
        setFocusPolicy(Qt::StrongFocus);

#if 0
        // Load cookie image (optional). If not found, a procedural cookie is drawn.
        m_cookie = QImage(":/assets/pb/pellsbawl_heart_cookie_master.png");
        if (!m_cookie.isNull())
            m_cookie = m_cookie.scaled(int(m_spriteSize), int(m_spriteSize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_useImage = !m_cookie.isNull();
#endif
        // Timer always on (drives charge + flight)
        // connect(&m_timer, &QTimer::timeout, this, &BezierThrowWidget::onTick);
        // m_timer.start(m_dtMs);

        resetEndpoints();
        resetPose();
    }

    void setGlobalScale(double globalScale = 1.0) { m_globalScale = globalScale; }

    bool isCharging() { return m_state == State::Charging; }

    void handleSpaceDown() {
        if (m_state == State::Idle) {
            beginCharge();
        }
    }

    void handleSpaceRelease(QPointF startingPoint, bool facingLeft) {
        if (m_state == State::Charging) {
            m_facingLeft = facingLeft;
            m_origin = startingPoint;
            launchFromCharge();
        }
    }

    void paintCookies(QPainter &p) {
        if(m_state == State::Flying) {
            // cookie sprite at m_pos with rotation m_angleDeg
            p.save();
            p.translate(m_origin);
            if(m_facingLeft) p.scale(-1, 1);
            p.translate(m_pos);
            p.rotate(m_angleDeg);
            drawHeartCookie(p, QSizeF(m_spriteSize, m_spriteSize));
            p.restore();
        }
        // Power bar
        // if (m_state == State::Charging) drawPowerBar(p);
    }

protected:
    // --- input ---
    void keyPressEvent(QKeyEvent* e) override {
        if (e->isAutoRepeat()) return;
        if (e->key() == Qt::Key_Space) {
            if (m_state == State::Idle) {
                beginCharge();
            }
        } else if (e->key() == Qt::Key_R) {
            m_alignToPath = !m_alignToPath;
            update();
        } else if (e->key() == Qt::Key_Escape) {
            close();
        } else {
            QWidget::keyPressEvent(e);
        }
    }

    void keyReleaseEvent(QKeyEvent* e) override {
        if (e->isAutoRepeat()) return;
        if (e->key() == Qt::Key_Space) {
            if (m_state == State::Charging)
                launchFromCharge();
        } else {
            QWidget::keyReleaseEvent(e);
        }
    }

    void resizeEvent(QResizeEvent*) override {
        resetEndpoints();           // keep start/end sensible on resize
        if (m_state == State::Idle) // keep cookie parked at start when idle
            resetPose();
    }

    // --- drawing ---
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor("#0e1116"));

        // curve preview
        QPen curvePen(QColor("#334155")); curvePen.setWidth(2);
        p.setPen(curvePen);
        QPainterPath path; path.moveTo(m_P0); path.quadTo(m_P1, m_P2); p.drawPath(path);

        // control points
        drawPoint(p, m_P0, QColor("#6ae3ff"), "P0");
        drawPoint(p, m_P1, QColor("#ffd166"), "P1");
        drawPoint(p, m_P2, QColor("#90ee90"), "P2");

        // cookie sprite at m_pos with rotation m_angleDeg
        p.save();
        p.translate(m_pos);
        p.rotate(m_angleDeg);
        const QRectF dst(-m_spriteSize/2.0, -m_spriteSize/2.0, m_spriteSize, m_spriteSize);
        // if (m_useImage) p.drawImage(dst, m_cookie); else drawHeartCookie(p, dst);
        p.restore();

        // HUD text
        p.setPen(QColor("#cbd5e1"));
        p.setFont(QFont("Monospace", 10));
        p.drawText(10, 18, "Hold SPACE to charge, release to throw   |   R: toggle spin vs. path-align   |   ESC: quit");
        p.drawText(10, 36, QString("Mode: %1").arg(m_alignToPath ? "Align to path" : "Constant spin"));

        // Power bar
        drawPowerBar(p);
    }

signals:
    void hasHit(Shape *p);

// private slots:
public:
    void onTick() {
        const double dt = m_dtMs / 1000.0;

        switch (m_state) {
        case State::Idle:
            // no-op
            break;

        case State::Charging: {
            // grow charge smoothly (with slight ease-in)
            m_charge = std::min(1.0, m_charge + dt / m_chargeTimeSec);
            // optional pulse while charging
            m_chargePulse = std::fmod(m_chargePulse + dt*2.0, 1.0);
            updateTrajectoryPreviewFromCharge();
            // update();
            break;
        }

        case State::Flying: {
            m_t += dt / m_durationSec;

            m_pos = bezierPoint(m_t, m_P0, m_P1, m_P2);

            if (m_alignToPath) {
                const QPointF tan = bezierTangent(m_t, m_P0, m_P1, m_P2);
                m_angleDeg = std::atan2(tan.y(), tan.x()) * 180.0 / M_PI;
            } else {
                m_angleDeg += m_angularSpeedDegPerSec * dt;
            }
            break;
        }
        }
    }

    void checkCollisions(QRectF &bounds, QList<Shape> &platforms) {
        QPointF pos = m_origin + (m_facingLeft ? -m_pos : m_pos);
        for (auto &p : platforms) {
            switch (p.shape) {
            case Shape::Rect:
                if (p.rect.contains(pos))
                    emit hasHit(&p);
                break;
            case Shape::TriLeft:
            case Shape::TriRight:
                break;
            }
        }
        if (!bounds.contains(pos)) emit hasHit(0);
    }
private:
    // ---------------- state + behavior ----------------
    enum class State { Idle, Charging, Flying };

    void resetEndpoints() {
        m_P0 = QPointF(0, 0); //80, height() - 80);
        // default “medium” throw to the right
        const double baseDx = 420.0;
        m_P2 = QPointF(m_P0.x() + baseDx, //std::min<double>(width() - 140, m_P0.x() + baseDx),
                       40);
        m_arcHeight = 200.0;
        m_skew = 0.7;
        m_P1 = makeControlPoint(m_P0, m_P2, m_arcHeight, m_skew);
    }

    void resetPose() {
        m_t = 0.0;
        m_pos = m_P0;
        m_angleDeg = 0.0;
        m_charge = 0.0;
        m_chargePulse = 0.0;
    }

    void beginCharge() {
        if (m_state != State::Idle) return;
        m_state = State::Charging;
        m_charge = 0.0;
        m_chargePulse = 0.0;
        // while charging, keep cookie at start pose
        m_pos = m_P0;
        m_angleDeg = 0.0;
        updateTrajectoryPreviewFromCharge();
        update();
    }

    void launchFromCharge() {
        // Compute endpoints from charge level.
        // Distance, arcHeight, and duration scale with charge.
        const double charge01 = m_charge;

        // Horizontal reach (pixels)
        const double minDx = 400.0;
        // const double maxDx = std::min<double>(width() - 160, 780.0);
        const double maxDx = 1100.0;
        const double dx = minDx + (maxDx - minDx) * easeOutQuad(charge01);

        // End Y: a bit higher for stronger throws (optional)
        const double minEndY = 60.0; //height() - 220;
        const double maxEndY = 0.0; //height() - 280; // stronger = slightly higher landing
        const double endY = lerp(minEndY, maxEndY, charge01);

        // m_P2 = QPointF(std::min<double>(width() - 80, m_P0.x() + dx), endY);
        m_P2 = QPointF(m_P0.x() + dx, endY);

        // Arc height: stronger throw → higher apex
        const double minH = 90.0, maxH = 220.0;
        m_arcHeight = lerp(minH, maxH, charge01);

        // Apex skew: slightly forward on strong throws
        m_skew = lerp(0.48, 0.60, charge01);

        // Duration: scale with distance to keep readable flight
        const double minDur = 0.20, maxDur = 0.80; // seconds
        m_durationSec = lerp(minDur, maxDur, 0.25 + 0.75*charge01);

        // Recompute control point
        m_P1 = makeControlPoint(m_P0, m_P2, m_arcHeight, m_skew);

        // Start flight
        m_state = State::Flying;
        m_t = 0.0;
        m_charge = 0.0;
        m_chargePulse = 0.0;

        update(); // redraw immediately
    }

    void updateTrajectoryPreviewFromCharge() {
        // Keep P0 fixed; suggest where P2 & P1 will end up at current charge (for the preview path).
        const double charge01 = m_charge;

        const double minDx = 280.0;
        const double maxDx = std::min<double>(width() - 160, 780.0);
        const double dx = minDx + (maxDx - minDx) * easeOutQuad(charge01);
        const double minEndY = height() - 220;
        const double maxEndY = height() - 280;
        const double endY     = lerp(minEndY, maxEndY, charge01);
        const double minH = 90.0, maxH = 220.0;
        const double H = lerp(minH, maxH, charge01);
        const double skew = lerp(0.48, 0.60, charge01);

        m_P2 = QPointF(std::min<double>(width() - 80, m_P0.x() + dx), endY);
        m_P1 = makeControlPoint(m_P0, m_P2, H, skew);
    }

    // ---------------- drawing helpers ----------------
    static void drawPoint(QPainter& p, const QPointF& c, const QColor& col, const QString& label) {
        p.save();
        p.setBrush(col);
        p.setPen(Qt::black);
        const QRectF r(c.x()-5, c.y()-5, 10, 10);
        p.drawEllipse(r);
        p.setPen(QColor("#cbd5e1"));
        p.setBrush(Qt::NoBrush);
        p.drawText(c + QPointF(8, -8), label);
        p.restore();
    }

    void drawHeartCookie(QPainter& p, const QSizeF& dst) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);

        const QColor dough(214,173,117);
        const QColor crust(176,129,71);
        const QColor chip(77,50,35);

        QPainterPath heart;
        p.scale(m_globalScale * dst.width()/100.0, m_globalScale * dst.height()/100.0);

        heart.moveTo(0, -20);
        heart.cubicTo(-50, -80, -50, -10, 0, 20);
        heart.cubicTo(50, -10, 50, -80, 0, -20);

        p.setPen(QPen(crust, 3));
        p.setBrush(dough);
        p.drawPath(heart);

        p.setBrush(chip);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(-20, -30, 10, 8));
        p.drawEllipse(QRectF(10, -10, 12, 9));
        p.drawEllipse(QRectF(-5,  0, 9, 7));

        p.restore();
    }

public:
    void drawPowerBar(QPainter& p) {
        p.save();
        p.resetTransform();

        double scale = p.window().width() / 800.0;
        const double x = 10.0 * scale, y = 50 * scale, w = 280 * scale, h = 16 * scale;

        // frame
        p.setPen(QPen(QColor("#475569"), 1.0 * scale));
        p.setBrush(QColor(0,0,0,40));
        p.drawRoundedRect(QRectF(x, y, w, h), 4, 4);

        // fill
        double fill01 = (m_state == State::Charging) ? m_charge : 0.0;
        const double innerPad = 2.0;
        const double fw = (w - 2*innerPad) * fill01;
        QRectF fillRect(x + innerPad, y + innerPad, fw, h - 2*innerPad);

        QColor bar = QColor("#22c55e");            // green
        if (m_state == State::Charging) {
            // slight pulsing while charging
            double pulse = 0.5 + 0.5*std::sin(m_chargePulse*2*M_PI);
            bar = QColor::fromHslF(0.32, 0.68, 0.45 + 0.15*pulse); // around green
        }
        p.setBrush(bar);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(fillRect, 3, 3);

        // percent text
        p.setPen(QColor("#cbd5e1"));
        p.setFont(QFont("Monospace", 9 * scale, QFont::DemiBold));
        QString pct = QString("%1%").arg(int(std::round(fill01*100)));
        p.drawText(QRect(x, y-1, w, h), Qt::AlignCenter, pct);

        p.restore();
    }

    // ---------------- tiny math utils ----------------
    static inline double lerp(double a, double b, double t) { return a + (b-a)*t; }
    static inline double easeOutQuad(double t) { return 1.0 - (1.0 - t)*(1.0 - t); }

private:
    // --- state ---
    /*enum class*/ State m_state = State::Idle;
    QPointF m_P0, m_P1, m_P2, m_pos;
    double  m_t = 0.0;                      // 0..1 along curve
    double  m_durationSec = 0.9;            // flight time (sec), varies with charge
    double  m_arcHeight = 140.0;            // controls P1
    double  m_skew = 0.5;                   // controls P1
    bool    m_alignToPath = false;          // false = spin, true = path-orient

    bool    m_facingLeft = false;
    QPointF m_origin;

    // charging
    double  m_charge = 0.0;                 // 0..1
    double  m_chargeTimeSec = 1.2;          // time to fill from 0 to 1
    double  m_chargePulse = 0.0;            // visual pulse

    // sprite
    QImage  m_cookie;
    bool    m_useImage = false;
    double  m_spriteSize = 64.0;            // render size
    double  m_angleDeg = 0.0;
    double  m_angularSpeedDegPerSec = 1200.0;

    double m_globalScale = 1.0 / 0.7;

    // timing
    QTimer  m_timer;
    int     m_dtMs = 16;                    // ~60 FPS
};
/*
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    BezierThrowWidget w;
    w.show();
    return app.exec();
}

#include "main.moc"
*/
#endif
