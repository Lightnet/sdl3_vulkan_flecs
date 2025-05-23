@echo off
setlocal
set APPNAME=VulkanTriangle
set APPPATH=build\Debug\%APPNAME%.exe
set EXECUTABLE=%APPNAME%.exe

set SCRIPT_SRC=script.lua
set SCRIPT_DEST=build\Debug\script.lua

echo copying %SCRIPT_SRC%
copy %SCRIPT_SRC% %SCRIPT_DEST%

if not exist %APPPATH% (
    echo Executable not found! Please build the project first.
    exit /b 1
)

@REM echo Running VulkanTriangle...

cd build\Debug\

%EXECUTABLE%

@REM if %ERRORLEVEL% NEQ 0 (
@REM     echo Program exited with error code %ERRORLEVEL%
@REM     exit /b %ERRORLEVEL%
@REM )

@REM echo Program ran successfully!
endlocal