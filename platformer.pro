QT=widgets openglwidgets
QT += multimedia
SOURCES=main.cpp \
    combo.cpp \
    fighter.cpp \
    fighterAI.cpp \
    joystick.cpp \
    level1.cpp \
    pellsBawl.cpp \
    title.cpp
HEADERS=\
    bezier.h \
    combo.h \
    commander.h \
    fighter.h \
    fighterAI.h \
    joystick.h \
    level1.h \
    pellsBawl.h \
    platform.h \
    title.h
RESOURCES=platformer.qrc
CONFIG+=c++17


include($$PWD/external/QJoysticks/QJoysticks.pri)

# # --- Fix SDL2 linkage for MinGW x64 ---
win32:mingw {
    SDL2_MSYS = C:/msys64/mingw64
    INCLUDEPATH += $$SDL2_MSYS/include
    INCLUDEPATH += $$SDL2_MSYS/include/SDL2
    LIBS -= -LC:\Users\alfki\repos\pellsBawl\external\QJoysticks\lib\SDL\bin\windows\mingw
    LIBS += -L$$SDL2_MSYS/lib -lSDL2 -lSDL2main
    # QMAKE_POST_LINK += $$QMAKE_COPY \"$$SDL2_MSYS/bin/SDL2.dll\" \"$$OUT_PWD/\"
    DEFINES += SDL_MAIN_HANDLED
}
