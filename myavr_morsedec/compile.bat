@echo off

set "PATH=%PATH%;%PROGRAMFILES%\Atmel\AVR Tools\AVR Toolchain\bin"

echo Cleaning source tree...
make.exe distclean V=1 BINEXT=.exe NODEPS=1

echo Compiling...
make.exe V=1 BINEXT=.exe NODEPS=1

pause
