<img src="https://raw.githubusercontent.com/LegendaryMauricius/EmuDiscer/main/EmuDiscer.svg" width="10%">

# EmuDiscer 
A simple tool for automatically lauching emulators when a game disc is inserted.
EmuDiscer recognizes the game system the game is made for, and lauches the right emulator.

# Supported consoles
* PS1
* PS2
* PSP
* Gamecube
* Wii
* More coming soon (hopefully)

# Usage
Before playing games you need to configure EmuDiscer. After launching, the app will stay minimized in the system tray.
To open the settings window click on EmuDiscer's icon in the tray. Now select the tab for the game system you want to setup, choose the emulator (by either browsing for the executable or selecting an app) and enter the command line options needed for the emulator to launch the game from disc. To make things easier, EmuDiscer has built-in options for popular emulators so you don't need to read the emulator's documentation.

![Alt text](docs/settings.png?raw=true)

# Command line option macros
Several macros are supported for CLI options when launching an emulator:
* (DRIVE) - The drive letter or a device file of the inserted media. 
* (DIRECTORY) - The path of the inserted game's files, without a trailing slash
* (BOOT_FILE) - The executable game file, relative to the directory (might only be useful for PS1 and PS2 games)