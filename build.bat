@echo off
setlocal EnableDelayedExpansion

echo ==========================================
echo      Spell Checker Build Script
echo ==========================================


if not exist "build"  mkdir build
if not exist "output" mkdir output
if not exist "data"   mkdir data


set "DICT=data\words_alpha.txt"
if not exist "%DICT%" (
    echo [INFO] Dictionary not found. Downloading...
    powershell -NoProfile -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/dwyl/english-words/master/words_alpha.txt' -OutFile '%DICT%'"
    if exist "%DICT%" (
        echo [SUCCESS] Dictionary downloaded.
    ) else (
        echo [ERROR] Failed to download dictionary.
        pause
        exit /b 1
    )
)


echo [INFO] Checking for compilers...

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 goto :BuildMSVC

echo [INFO] MSVC not in path. Searching for Visual Studio...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VSROOT=%%i"
    )
)

if defined VSROOT (
    echo [INFO] Found VS at: "!VSROOT!"
    echo [INFO] Initializing MSVC environment...

    if exist "!VSROOT!\VC\Auxiliary\Build\vcvars64.bat" (
        call "!VSROOT!\VC\Auxiliary\Build\vcvars64.bat" >nul
    ) else (
        call "!VSROOT!\VC\Auxiliary\Build\vcvars32.bat" >nul
    )

    where cl >nul 2>nul
    if !ERRORLEVEL! EQU 0 goto :BuildMSVC
)

where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 goto :BuildGPP

echo [ERROR] No C++ compiler found!
echo [TIP]   Please run this script from the "Developer Command Prompt for VS"
echo         or install MinGW (g++).
pause
exit /b 1

:: ================================================================
:: Build
:: ================================================================

:BuildMSVC
echo [INFO] MSVC [cl.exe] detected. Compiling...

cl /std:c++17 /O2 /EHsc /Fe:build\spell_checker.exe src\spell-checker.cpp >nul

if exist spell-checker.obj del spell-checker.obj

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build complete using MSVC.
    goto :Run
)
echo [ERROR] Compilation failed.
pause
exit /b 1

:BuildGPP
echo [INFO] G++ detected. Compiling...
g++ -std=c++17 -Wall -Wextra -O3 -pthread -o build/spell_checker.exe src/spell-checker.cpp

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build complete using G++.
    goto :Run
)
echo [ERROR] Compilation failed.
pause
exit /b 1

:: ================================================================
:: Run
:: ================================================================

:Run
echo.
echo ==========================================
echo      Running Test
echo ==========================================
echo [INFO] Processing data/input.txt...

build\spell_checker.exe data\words_alpha.txt < data\input.txt > output\test.html

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Output generated at output\test.html
    echo [INFO] Opening file...
    start output\test.html
) else (
    echo [ERROR] Runtime error occurred.
)

echo.
echo Done.
pause
