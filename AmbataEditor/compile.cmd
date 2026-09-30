@echo off
if not exist "images\app_icon.ico" (
    echo ERROR: images\app_icon.ico not found
    pause
    exit /b 1
)
windres app.rc -O coff -o app_res.o
if %errorlevel% neq 0 (
    echo ERROR: windres failed - check that app_icon.ico is a real .ico file
    pause
    exit /b 1
)
g++ main.cpp app_res.o -o Ambata_Editor.exe -std=gnu++17 -O2 -mwindows -static -lgdiplus -lcomctl32 -lgdi32 -lcomdlg32
if %errorlevel% equ 0 (
    echo ****** COMPILE SUCCESSED ******
) else (
    pause
)
