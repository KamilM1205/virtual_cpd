@echo off

if "%1"=="" goto DEBUG
if %1==help goto HELP_MSG
if %1==debug goto DEBUG
if %1==release goto RELEASE

goto END

:HELP_MSG
echo Syntax: build_xYY.bat *command*(default - debug)
echo - help - show this message
echo Commands:
echo - release - build release *.sys file
echo - debug - build debug *.sys file

goto END

:DEBUG
set TARGET=Debug
goto BUILD

:RELEASE
set TARGET=Release
goto BUILD

:BUILD
cd build
cmake -G "Visual Studio 17 2022" -A x64 -S .. -B "build64"
cmake --build build64 --config %TARGET%

goto END

:: Return to main directory

cd ../..

:END