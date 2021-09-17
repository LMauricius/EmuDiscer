<img src="https://raw.githubusercontent.com/LegendaryMauricius/EmuDiscer/main/EmuDiscer.svg" width="10%">

# EmuDiscer 
A simple tool for automatically lauching emulators when a game disc is inserted.
EmuDiscer recognizes which system the game is made for, and lauches the right emulator.

- [EmuDiscer](#emudiscer)
- [Supported consoles](#supported-consoles)
- [Installation](#installation)
- [Usage](#usage)
- [EmuDiscer command line options](#emudiscer-command-line-options)
- [Emulator launch command line macros](#emulator-launch-command-line-macros)
- [Changelog](#changelog)
  - [v1.1](#v11)
- [Building the program from source code](#building-the-program-from-source-code)
  - [Building on Windows:](#building-on-windows)
  - [Building on Linux:](#building-on-linux)

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
To open the settings window click on EmuDiscer's icon in the tray. Now select the tab for the game system you want to setup, choose the emulator (by either browsing for the executable or selecting an app) and enter the command line options needed for the emulator to launch the game from disc. To make things easier, EmuDiscer has built-in options for popular emulators so you don't need to read the emulators' docs.

![Alt text](docs/settings.png?raw=true)

# EmuDiscer command line options
* --open-settings - Opens the settings window on launch
* --multi-instance - Open EmuDiscer even if another instance is running

# Emulator launch command line macros
Several macros are supported for CLI options when launching an emulator. The macros will be replaced with their values before the options are passed to the program.
* (DRIVE) - The drive letter or a device file of the inserted media. 
* (DIRECTORY) - The path of the inserted game's files, without a trailing slash
* (BOOT_FILE) - The executable game file, relative to the directory (might only be useful for PS1 and PS2 games)

# Changelog
## v1.1
* Allowing running only one instance of the app
* Installer bugfixes
* Clicking on the "Emulator not configured" notification sends the user the emulator configuration page
* UI naming changes
* Performance improvement for the app chooser dialog
* Internal changes that allow full functionality in the Appimage version of EmuDiscer

# Building the program from source code

EmuDiscer is made with [Qt](https://www.qt.io/) and its only dependencies are included with Qt.

To build the source you can open it with QtCreator and launch the build process, or use qmake directly. 

## Building on Windows:
To allow running the app without QtCreator, locate the built .exe file and move it to an empty folder. Run windeployqt.exe from the Qt binaries location with the .exe as the only argument to copy the needed libraries. EmuDiscer is now ready to be launched.

## Building on Linux:
If the qt libraries are installed, the built exe is ready to be launched on your system. For running on other systems, the relesed AppImage is built on Ubuntu Bionic using [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy), with the [qt plugin](https://github.com/linuxdeploy/linuxdeploy-plugin-qt). After building the executable, the following command is run: 
```
<linuxdeploy exe path> -e <executable path> -d <.desktop file path (inside the source dir)> -i <.svg icon path (inside the source dir)> --appdir AppDir --plugin qt --output appimage
```
