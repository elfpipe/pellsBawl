#include <QJoysticks.h>
#include "joystick.h"
#include <QEventLoop>
GameJoystick::GameJoystick(QObject *parent)
    : QObject{parent}
{
}

void GameJoystick::setGameMode() {
    if(gameMode) return;
    auto js = QJoysticks::getInstance();
    js->setVirtualJoystickEnabled(true);

    a = QObject::connect(js, &QJoysticks::axisChanged, this, [&](int /*id*/, int axis, qreal value){
        // X axis -> LEFT / RIGHT
        bool got = false;
        static bool left = false, right = false;
        if(axis == 0 && value < -0.35) { if (!left) { combo.key(Combo::LEFT, m_lastMs); got = true; } left = true; right = false; }
        if(axis == 0 && value > 0.35) { if (!right) { combo.key(Combo::RIGHT, m_lastMs); got = true; } right = true; left = false; }
        if(axis == 1 && value < -0.35) { combo.key(Combo::UP, m_lastMs); got = true; }
        if(axis == 1 && value > 0.35) { combo.key(Combo::DOWN, m_lastMs); got = true; }
        if(got && joyCommander && joyCommander->applyCombo(combo.getCurrent())) combo.resetCurrent();
        if(axis == 0 && std::abs(value) < 0.25 && joyCommander) { left = false; right = false; joyCommander->releaseLeftRight(); }
    });

    b = QObject::connect(js, &QJoysticks::buttonChanged, this, [&](int /*id*/, int button, bool pressed){
        // qDebug() << "button:" << button;
        bool got = false;
        static bool powerDown = false;
        if(button == 0 && pressed) { combo.key(Combo::FIRE1, m_lastMs); got = true; }
        if(button == 1 && pressed) { if(!powerDown) { combo.key(Combo::FIRE2, m_lastMs); got = true; powerDown = true; } }
        if(button == 1 && !pressed) { joyCommander->releasePowerButton(); powerDown = false; }
        if(button == 11 && pressed) { combo.key(Combo::UP, m_lastMs); got = true; }
        if(button == 12 && pressed) { combo.key(Combo::DOWN, m_lastMs); got = true; }
        if(button == 13 && pressed) { combo.key(Combo::LEFT, m_lastMs); got = true; }
        if(button == 14 && pressed) { combo.key(Combo::RIGHT, m_lastMs); got = true; }
        if(got && joyCommander && joyCommander->applyCombo(combo.getCurrent())) combo.resetCurrent();
        if((button == 13 || button == 14) && !pressed && joyCommander) joyCommander->releaseLeftRight();
    });
    gameMode = true;
}

void GameJoystick::unsetGameMode() {
    if(!gameMode) return;
    disconnect(a); disconnect(b);
    gameMode = false;
}

void GameJoystick::waitForPush() {
    unsetGameMode();
    QEventLoop loop;
    auto js = QJoysticks::getInstance();
    js->setVirtualJoystickEnabled(true);
    auto a = QObject::connect(js, &QJoysticks::buttonChanged, this, [&](int /*id*/, int button, bool pressed){
        if((button == 0 || button == 1 || button == 2 || button == 3) && pressed) loop.quit();
    });
    loop.exec();
    disconnect(a);
    setGameMode();
}
