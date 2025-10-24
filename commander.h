#ifndef COMMANDER_H
#define COMMANDER_H

#include "combo.h"

// ------------------------------
// Abstract interface to your character command layer
// ------------------------------
class IJoystickCommander {
public:
    virtual ~IJoystickCommander() = default;
    virtual bool applyCombo(QVector<Combo::Key> combo) = 0;
    virtual void releaseLeftRight() = 0;
    virtual void releasePowerButton() {}
    virtual void releaseDownButton() {}
};

#endif // COMMANDER_H
