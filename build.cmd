@echo off
if not "%~1"=="" (
    echo Usage: build.cmd
    exit /b 1
)
call "%~dp0distribution\build.cmd"
exit /b %errorlevel%
