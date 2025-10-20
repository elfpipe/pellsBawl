#ifndef COMBO_H
#define COMBO_H

#include <QObject>
#include <QVector>

#include <QDebug>

class Combo : public QObject
{
    Q_OBJECT
public:
    enum Key {
        UP, DOWN, LEFT, RIGHT, FIRE1, FIRE2
    };

public:
    explicit Combo(QObject *parent = nullptr, quint64 resetTimer = 1000);
    void update(quint64 time) {
        currTime = time;
        if(time > stamp + reset) { current.clear(); stamp = time; emit currentReset(); }
    }
    void key(Key key, qreal time) {
        update(time);
        current.append(key);
        // qDebug() << "(" << time << ") : " << current;
    }
    void setReset(quint64 resetTimer) { reset = resetTimer; }
    QVector<Key> getCurrent() { return current; }
    void resetCurrent() { current.clear(); }

signals:
    void currentReset();

private:
    QVector<Key> current;
    quint64 currTime = 0, stamp = 0, reset = 1000;
};

#endif // COMBO_H
