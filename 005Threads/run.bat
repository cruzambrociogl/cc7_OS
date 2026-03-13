@echo off
echo Compiling...
gcc -Wall -o log_analyzer.exe main_windows.c log_processor.c

if %ERRORLEVEL% == 0 (
    echo.
    echo Running with 4 threads...
    echo =========================
    log_analyzer.exe access.log 4
)
pause
