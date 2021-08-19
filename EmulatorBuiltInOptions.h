#ifndef EMULATORBUILTINOPTIONS_H
#define EMULATORBUILTINOPTIONS_H

#include <map>
#include <list>
#include <QString>

std::map<QString, std::list<std::pair<QString, QString>>> OptionsPerProgram = {
    {"epsxe", {
         {"Run from CD", " -cdrom (DRIVE)"},
         {"Show PS splashscreen", " -slowboot"},
         {"Start in fullscreen", " -fullscreen"},
         {"Run without a GUI", " -nogui"}
     }
    },
    {"pcsx2", {
         {"Run from CD/DVD", " --usecd"},
         {"Show PS splashscreen (not recommended - game doesn't boot)", " --fullboot"},
         {"Start in fullscreen", " --fullscreen"},
         {"Run without a GUI", " --nogui"}
     }
    },
    {"ppsspp", {
         {"Don't use iso", "\"\""},
         {"Start in fullscreen", " --fullscreen"}
     }
    },
    {"dolphin", {
         // Windows version requires a ':' at end of drive to recognize it as a drive
#ifdef _WIN32
         {"Run from CD/DVD", " --exec=(DRIVE):"},
#elif defined __unix__
         {"Run from CD/DVD", " --exec=(DRIVE)"},
#endif
         {"Skip library loading (fast open)", " --batch"}
     }
    }
};

#endif // EMULATORBUILTINOPTIONS_H
