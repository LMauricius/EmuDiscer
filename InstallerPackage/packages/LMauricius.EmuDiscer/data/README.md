<img src="https://raw.githubusercontent.com/LegendaryMauricius/EmuDiscer/main/EmuDiscer.svg" width="10%">

# EmuDiscer 
A simple tool for automatically lauching emulators when a game disc is inserted.
EmuDiscer recognizes which system the game is made for, and lauches the right emulator.

# Supported consoles
* PS1
* PS2
* PSP
* Gamecube
* Wii
* More coming soon (hopefully)

# Installation
You can find installers at the [release page](https://github.com/LegendaryMauricius/EmuDiscer/releases).

# Usage
Before playing games you need to configure EmuDiscer. After launching, the app will stay minimized in the system tray.
To open the settings window click on EmuDiscer's icon in the tray. Now select the tab for the game system you want to setup, choose the emulator (by either browsing for the executable or selecting an app) and enter the command line options needed for the emulator to launch the game from disc. To make things easier, EmuDiscer has built-in options for popular emulators so you don't need to read the emulator's documentation.

![Alt text](docs/settings.png?raw=true)

# Emulator command line macros
Several macros are supported for CLI options when launching an emulator:
* (DRIVE) - The drive letter or a device file of the inserted media. 
* (DIRECTORY) - The path of the inserted game's files, without a trailing slash
* (BOOT_FILE) - The executable game file, relative to the directory (might only be useful for PS1 and PS2 games)

# Building the program from source code

EmuDiscer is made with [Qt](https://www.qt.io/) and its only dependencies are included with Qt.

To build the source you can open it with QtCreator and launch the build process, or use qmake directly. 

## Building on Windows:
To allow running the app without QtCreator, locate the built .exe file and move it to an empty folder. Run windeployqt.exe from the Qt binaries location with the .exe as the only argument to copy the needed libraries. EmuDiscer is now ready to be launched.