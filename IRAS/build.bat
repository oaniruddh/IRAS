@echo off
echo Building IRAS System Monitor...

REM Create build directory
if not exist "build" mkdir build
cd build

REM Generate build files with CMake
echo Configuring with CMake...
cmake .. -G "Visual Studio 16 2019" -A x64

REM Check if CMake was successful
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    echo Trying with MinGW Makefiles...
    cmake .. -G "MinGW Makefiles"
    if %ERRORLEVEL% NEQ 0 (
        echo CMake configuration failed with MinGW too!
        pause
        exit /b 1
    )
)

REM Build the project
echo Building project...
cmake --build . --config Release

REM Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build successful!
    echo Executable location: build\bin\Release\SystemMonitor.exe
    echo                  or: build\bin\SystemMonitor.exe
    echo ========================================
    echo.
    
    REM Try to run the executable
    if exist "bin\Release\SystemMonitor.exe" (
        echo Would you like to run the application? (y/n)
        set /p choice=
        if /i "%choice%"=="y" (
            start "" "bin\Release\SystemMonitor.exe"
        )
    ) else if exist "bin\SystemMonitor.exe" (
        echo Would you like to run the application? (y/n)
        set /p choice=
        if /i "%choice%"=="y" (
            start "" "bin\SystemMonitor.exe"
        )
    )
) else (
    echo Build failed!
)

cd ..
pause
