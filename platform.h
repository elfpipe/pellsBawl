#ifndef PLATFORM_H
#define PLATFORM_H
#include <QRect>
struct Platform {
    QRect rect;
    bool isGround;

    Platform(int x, int y, int w, int h, bool ground = false)
        : rect(x, y, w, h), isGround(ground) {}
};
#endif // PLATFORM_H
