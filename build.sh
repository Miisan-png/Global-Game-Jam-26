#!/bin/bash

PROJECT_PATH="/mnt/d/GGJ/GGJ26"
PROJECT_NAME="GGJ26"

# Adjust these paths to your setup
UE_VERSION="UE_5.4"
UE_DRIVE="D:"
UE_DRIVE_MNT="/mnt/d"

WIN_PROJECT="D:\\GGJ\\GGJ26\\${PROJECT_NAME}.uproject"
UE_ROOT="${UE_DRIVE_MNT}/${UE_VERSION}"

echo "========================================"
echo "  Cleaning ${PROJECT_NAME}..."
echo "========================================"

rm -rf "${PROJECT_PATH}/Binaries"
rm -rf "${PROJECT_PATH}/Intermediate"
rm -rf "${PROJECT_PATH}/.vs"
rm -f "${PROJECT_PATH}/${PROJECT_NAME}.sln"

echo "  Deleted: Binaries/, Intermediate/, .vs/, .sln"

echo "========================================"
echo "  Building ${PROJECT_NAME}..."
echo "========================================"

cmd.exe /c "${UE_DRIVE}\\${UE_VERSION}\\Engine\\Build\\BatchFiles\\Build.bat" ${PROJECT_NAME}Editor Win64 Development "${WIN_PROJECT}" -waitmutex

if [ $? -eq 0 ]; then
    echo "========================================"
    echo "  Build SUCCESS! Launching Editor..."
    echo "========================================"
    cmd.exe /c "${UE_DRIVE}\\${UE_VERSION}\\Engine\\Binaries\\Win64\\UnrealEditor.exe" "${WIN_PROJECT}" &
else
    echo "========================================"
    echo "  Build FAILED! Check errors above."
    echo "========================================"
    exit 1
fi
