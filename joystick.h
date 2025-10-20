#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <QObject>
#include "commander.h"

class Joystick : public QObject
{
    Q_OBJECT
public:
    explicit Joystick(QObject *parent = nullptr);
    void setCommander(IJoystickCommander *commander) { joyCommander = commander; }
    void updateTime(double time) { m_lastMs = time; }
signals:

private:
    IJoystickCommander *joyCommander = 0;
    Combo combo;
    double m_lastMs = 0.0;
};

#endif // JOYSTICK_H
