# Simple and Lightweight alternative to AlwaysOnTop from Powertoys (by Microsoft), written in C++ :)

### HOW TO RUN:
 - Go to [Releases](https://github.com/CyberCactus64/light-always-on-top/releases) and download `AlwaysOnTop.exe`
 - If you prefer to compile the code yourself, use: `./Compile.bat`

#### NB: inside the script compile.bat there are two instructions:
 - ```windres IconSet.rc -O coff -o IconSet.o``` -> **To compile the icon into an object file** (see IconSet.rc for the icon path).
 - ```g++ Light-AlwaysOnTop.cpp IconSet.o -o "AlwaysOnTop.exe" -mwindows``` -> **To compile the code** and link the icon (from IconSet.o) into the final executable, -mwindows is used to hide the terminal when running the application.

### HOW TO USE:
 - Press __WIN + CTRL + T__ on the active window to enable/disable Always On Top on it!

---

### FUTURE UPDATES:
 - Add colored borders for the active window
 - When the user clicks on "Exit", every window set as Always on top lose this feature 
 - Simple management application where the user can change stuff like the borders color, shortcuts and exceptions