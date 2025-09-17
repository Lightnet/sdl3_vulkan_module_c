@echo off
setlocal EnableDelayedExpansion

REM Check if the build directory exists
if not exist "build" (
    echo Error: Build directory 'build\Debug' not found.
    echo Please run build.bat first to compile the project.
    exit /b 1
)
cd build 
REM Check if the executable exists
set "EXE_PATH=sdl3_imgui.exe"
if not exist "!EXE_PATH!" (
    echo Error: Executable '!EXE_PATH!' not found.
    echo Ensure the build succeeded by running build.bat.
    exit /b 1
)

REM Run the executable
echo Running application example...
"!EXE_PATH!"

REM Check if the program ran successfully
if !ERRORLEVEL! neq 0 (
    echo Program exited with error code !ERRORLEVEL!
    exit /b !ERRORLEVEL!
) else (
    echo Program finished successfully!
)

endlocal