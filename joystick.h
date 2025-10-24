#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <QObject>
#include "commander.h"
#include <QEventLoop>
class GameJoystick : public QObject
{
    Q_OBJECT
public:
    explicit GameJoystick(QObject *parent = nullptr);
    void setCommander(IJoystickCommander *commander) { joyCommander = commander; }
    void updateTime(double time) { m_lastMs = time; }

public:
    void waitForPush();
    void setGameMode();
    void unsetGameMode();

private:
    IJoystickCommander *joyCommander = 0;
    Combo combo;
    double m_lastMs = 0.0;

    bool gameMode = false;
    QEventLoop waitLoop;
    bool waitState = false;
    QMetaObject::Connection a, b;
};

#endif // JOYSTICK_H
