@echo off
setlocal

set PROJECT_PATH=D:\GGJ\GGJ26
set PROJECT_NAME=GGJ26
set UE_VERSION=UE_5.4
set UE_PATH=D:\%UE_VERSION%

echo ========================================
echo   Cleaning %PROJECT_NAME%...
echo ========================================

if exist "%PROJECT_PATH%\Binaries" rmdir /s /q "%PROJECT_PATH%\Binaries"
if exist "%PROJECT_PATH%\Intermediate" rmdir /s /q "%PROJECT_PATH%\Intermediate"
if exist "%PROJECT_PATH%\.vs" rmdir /s /q "%PROJECT_PATH%\.vs"
if exist "%PROJECT_PATH%\%PROJECT_NAME%.sln" del /q "%PROJECT_PATH%\%PROJECT_NAME%.sln"

echo   Deleted: Binaries/, Intermediate/, .vs/, .sln

echo ========================================
echo   Building %PROJECT_NAME%...
echo ========================================

call "%UE_PATH%\Engine\Build\BatchFiles\Build.bat" %PROJECT_NAME%Editor Win64 Development "%PROJECT_PATH%\%PROJECT_NAME%.uproject" -waitmutex

if %ERRORLEVEL% EQU 0 (
    echo ========================================
    echo   Build SUCCESS! Launching Editor...
    echo ========================================
    start "" "%UE_PATH%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_PATH%\%PROJECT_NAME%.uproject"
) else (
    echo ========================================
    echo   Build FAILED! Check errors above.
    echo ========================================
    pause
)

endlocal
