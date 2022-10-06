@echo off

SETLOCAL

set oldcwd=%cd%
cd C:\Users\other\Documents\repos\nlang

call build.bat DebugMode
move main.exe release/nlang.exe
copy main.pdb release

cd %oldcwd%

ENDLOCAL

