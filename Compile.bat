@echo off

echo Compilation started...
g++ Light-AlwaysOnTop.cpp -o "AlwaysOnTop.exe" -mwindows

:: check if there are errors during compilation
if %ERRORLEVEL% neq 0 (
    echo Error during compilation.
    exit /b %ERRORLEVEL%
)

echo You can run AlwaysOnTop.exe now!