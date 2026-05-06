@echo off
setlocal enabledelayedexpansion

echo Searching for Visual Studio...

set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo Error: Visual Studio with C++ tools not found.
    pause
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

echo Compiling OSMV-Lite C++...

cl /EHsc /std:c++20 /O2 /MT ^
    main.cpp ^
    /I. ^
    /link ^
    /SUBSYSTEM:WINDOWS ^
    /OUT:OSMV-Lite-cpp.exe ^
    user32.lib gdi32.lib shell32.lib gdiplus.lib shlwapi.lib windowsapp.lib

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================
    echo Build successful: OSMV-Lite-cpp.exe
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Build failed.
    echo ========================================
)

pause
