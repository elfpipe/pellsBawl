#ifndef PLATFORM_H
#define PLATFORM_H
#include <QRect>
#include <QImage>

typedef QString Id;

struct Transform {
    QPointF pos{0,0};
    qreal rotation = 0.0; // degrees
    qreal scaleX = 1.0, scaleY = 1.0;    // uniform
};

struct Shape {
    Id id; // unique
    enum {
        Rect, TriLeft, TriRight
    } shape;
    QRectF rect; // used if rect
    bool isWall = false;
};

struct Image {
    Id id;
    QString path;   // disk path
    QImage img;     // loaded image
    Transform tf;
    qreal z = 0;    // for future sorting
};
#endif // PLATFORM_H
