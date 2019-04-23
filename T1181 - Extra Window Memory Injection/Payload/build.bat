@echo off
cl -nologo -Os xbin.cpp
echo.
cl -DWINDOW -c -nologo -Os -O2 -Gm- -GR- -EHa -Oi -GS- T1181-payload.c
link /order:@extrabytes.txt /entry:WndProc /base:0 T1181-payload.obj -subsystem:console -nodefaultlib -stack:0x100000,0x100000
xbin T1181-payload.exe .text
ren T1181-payload.exe64.bin T1181-payload.bin
