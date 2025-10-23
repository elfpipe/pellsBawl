import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    width: 800; height: 450; color: "#0e1116"
    focus: true

    // --- Throw parameters ---
    property point P0: Qt.point(80,  height - 80)   // start
    property point P2: Qt.point(width - 120, height - 180) // end
    property real  arcHeight: 140                    // peak height (pixels)
    property real  skew: 0.5                         // 0..1, where the apex sits between P0 and P2
    property real  duration: 800                     // ms
    property real  angularSpeed: 480                 // deg/sec spin

    // --- Derived ---
    property point P1: controlPoint(P0, P2, arcHeight, skew)
    property real  t: 0.0                            // 0..1 along curve

    // Cookie sprite (replace source with your sprite/frame if you like)
    Image {
        id: cookie
        width: 64; height: 64
        source: "heart_cookie.png" // placeholder; point this at your cookie frame/spritesheet frame
        fillMode: Image.PreserveAspectFit
        transformOrigin: Item.Center
        anchors.verticalCenterOffset: 0
    }

    // Markers to visualize the curve control points (optional)
    Repeater {
        model: [
            { p: P0, c:"#6ae3ff", name: "P0" },
            { p: P1, c:"#ffd166", name: "P1" },
            { p: P2, c:"#90ee90", name: "P2" }
        ]
        delegate: Column {
            x: modelData.p.x - 5; y: modelData.p.y - 5
            spacing: 2
            Rectangle { width: 10; height: 10; radius: 5; color: modelData.c; border.color: "#000000"; border.width: 1 }
            Text { text: modelData.name; color: "#cbd5e1"; font.pixelSize: 12 }
        }
    }

    // A simple visual curve preview (optional)
    Canvas {
        id: preview
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0,0,width,height);
            ctx.beginPath();
            ctx.moveTo(P0.x, P0.y);
            ctx.quadraticCurveTo(P1.x, P1.y, P2.x, P2.y);
            ctx.lineWidth = 2;
            ctx.strokeStyle = "#334155";
            ctx.stroke();
        }
        Connections { target: parent; function onP1Changed(){ preview.requestPaint(); } }
        Connections { target: parent; function onP0Changed(){ preview.requestPaint(); } }
        Connections { target: parent; function onP2Changed(){ preview.requestPaint(); } }
    }

    // Timer drives t from 0..1
    Timer {
        id: tick
        interval: 16; running: false; repeat: true
        onTriggered: {
            t += interval / duration;
            if (t >= 1.0) { t = 1.0; running = false; }
            var p = bezierPoint(t, P0, P1, P2);
            cookie.x = p.x - cookie.width/2;
            cookie.y = p.y - cookie.height/2;

            // Option A: spinning cookie
            cookie.rotation += angularSpeed * (interval / 1000);

            // Option B: align to path direction (comment in to use)
            // var tan = bezierTangent(t, P0, P1, P2);
            // cookie.rotation = Math.atan2(tan.y, tan.x) * 180/Math.PI;
        }
    }

    // Helpers
    function controlPoint(P0, P2, h, skew) {
        // Skew=0.5 -> apex mid-way; <0.5 closer to P0; >0.5 closer to P2
        var cx = P0.x + (P2.x - P0.x) * skew;
        var cy = (P0.y + P2.y) * 0.5 - h; // up by h
        return Qt.point(cx, cy);
    }
    function bezierPoint(t, P0, P1, P2) {
        var u = 1 - t;
        return Qt.point(
            u*u*P0.x + 2*u*t*P1.x + t*t*P2.x,
            u*u*P0.y + 2*u*t*P1.y + t*t*P2.y
        );
    }
    function bezierTangent(t, P0, P1, P2) {
        // derivative: 2(1-t)(P1-P0) + 2t(P2-P1)
        return Qt.point(
            2*(1-t)*(P1.x - P0.x) + 2*t*(P2.x - P1.x),
            2*(1-t)*(P1.y - P0.y) + 2*t*(P2.y - P1.y)
        );
    }

    // Fire!
    function startThrow() {
        t = 0.0;
        P1 = controlPoint(P0, P2, arcHeight, skew);
        tick.start();
    }

    // init pose and key to fire
    Component.onCompleted: {
        var p = bezierPoint(0.0, P0, P1, P2);
        cookie.x = p.x - cookie.width/2;
        cookie.y = p.y - cookie.height/2;
        cookie.rotation = 0;
    }
    Keys.onSpacePressed: startThrow()

    // Tiny UI to tweak on the fly (optional)
    Row {
        spacing: 12; anchors.left: parent.left; anchors.leftMargin: 12; anchors.top: parent.top; anchors.topMargin: 12
        Text { text: "Height"; color: "#cbd5e1" }
        Slider { from: 20; to: 240; value: arcHeight; onValueChanged: arcHeight=value; width: 160 }
        Text { text: "Skew"; color: "#cbd5e1" }
        Slider { from: 0.1; to: 0.9; value: skew; onValueChanged: skew=value; width: 120 }
        Text { text: "Time"; color: "#cbd5e1" }
        Slider { from: 300; to: 1400; value: duration; onValueChanged: duration=value; width: 160 }
        Button { text: "Throw"; onClicked: startThrow() }
    }
}
