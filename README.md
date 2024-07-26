# OperaUninstall

This repository contains an automated process to cleanly uninstall Opera browser from a Windows system. It references a manual cleaning process described on [Reddit](https://www.reddit.com/r/operabrowser/wiki/opera/clean_opera_from_windows/).

## Prerequisites

Ensure you have a C compiler installed on your system.

## Compilation Instructions

### Windows
1. Install MinGW-w64 for Windows from [mingw-w64.org](https://www.mingw-w64.org/).
2. Open the MinGW-w64 terminal.
3. Navigate to the directory containing `OperaUninstall.c`.
4. Run the following command to compile:
   ```sh
   gcc -o OperaUninstall.exe OperaUninstall.c -ladvapi32 -lshell32 -lshlwapi

### Ubuntu
1. Open a terminal.
2. Install GCC and MinGW-w64 by running:
   ```sh
   sudo apt update
   sudo apt install gcc mingw-w64
   x86_64-w64-mingw32-gcc -o OperaUninstall.exe OperaUninstall.c -ladvapi32 -lshell32 -lshlwapi
 
