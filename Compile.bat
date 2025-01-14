@echo off

echo Compilation started...

windres IconSet.rc -O coff -o IconSet.o
g++ Light-AlwaysOnTop.cpp IconSet.o -o "AlwaysOnTop.exe" -mwindows

:: check if there are errors during compilation
if %ERRORLEVEL% neq 0 (
    echo Error during compilation.
    exit /b %ERRORLEVEL%
)

echo You can run AlwaysOnTop.exe now!