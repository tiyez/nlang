@echo off

SETLOCAL

set param1=%~1
if "%param1%"=="" set param1="DebugMode"

cl.exe /nologo /Z7 /D %param1% main.c


ENDLOCAL


