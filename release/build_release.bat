@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "BUILD_DIR=%PROJECT_DIR%\build"

:: Read version from source or default to 1.0.0
set "VERSION=1.0.0"
for /f "tokens=3 delims= " %%a in ('findstr /R /C:"#define.*GLOOMBOT_VERSION" "%PROJECT_DIR%\src\bot\bot.h" 2^>nul') do (
    set "VER=%%~a"
    set "VERSION=!VER:"=!"
)
set "ARCHIVE_NAME=q2gloombot-v%VERSION%-windows-x86.zip"

echo === Q2GloomBot Release Build v%VERSION% (Windows x86) ===

:: Clean build
echo --- Cleaning previous build ---
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

:: Configure and build
echo --- Configuring CMake (Release, Win32) ---
cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 (
    echo CMake configuration failed.
    exit /b 1
)

echo --- Building ---
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

:: Run tests
echo --- Running tests ---
"%BUILD_DIR%\Release\bot_test.exe"
if errorlevel 1 (
    echo Tests failed.
    exit /b 1
)
echo --- Tests passed ---

:: Package release
echo --- Packaging release ---
set "STAGING_DIR=%TEMP%\q2gloombot-staging"
set "RELEASE_DIR=%STAGING_DIR%\q2gloombot-v%VERSION%"
if exist "%STAGING_DIR%" rmdir /s /q "%STAGING_DIR%"
mkdir "%RELEASE_DIR%"

copy "%BUILD_DIR%\Release\gamex86.dll" "%RELEASE_DIR%\"
xcopy "%PROJECT_DIR%\config" "%RELEASE_DIR%\config\" /s /e /i /q
xcopy "%PROJECT_DIR%\maps" "%RELEASE_DIR%\maps\" /s /e /i /q
xcopy "%PROJECT_DIR%\docs" "%RELEASE_DIR%\docs\" /s /e /i /q
copy "%PROJECT_DIR%\README.md" "%RELEASE_DIR%\"
copy "%PROJECT_DIR%\LICENSE" "%RELEASE_DIR%\"
copy "%PROJECT_DIR%\CHANGELOG.md" "%RELEASE_DIR%\"

:: Create zip using PowerShell
powershell -NoProfile -Command "Compress-Archive -Path '%RELEASE_DIR%' -DestinationPath '%SCRIPT_DIR%%ARCHIVE_NAME%' -Force"
if errorlevel 1 (
    echo Packaging failed.
    exit /b 1
)

rmdir /s /q "%STAGING_DIR%"

echo === Release package created: release\%ARCHIVE_NAME% ===
endlocal
