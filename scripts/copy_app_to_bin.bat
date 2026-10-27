@echo off
rem ============================================================================
rem  post-build copy: deploy the freshly built TrafficMonitor binaries into the
rem  runtime folder (E:\code\TrafficMonitor_bin) so the app can be started
rem  straight from there without copying files by hand.
rem
rem  usage: copy_app_to_bin.bat "<path to built TrafficMonitor.exe>"
rem ============================================================================
setlocal enabledelayedexpansion

set "SRC_EXE=%~1"
set "DEST=E:\code\TrafficMonitor_bin"

if "%SRC_EXE%"=="" (
    echo [post-build] ERROR: no source executable given.
    exit /b 1
)

if not exist "%SRC_EXE%" (
    echo [post-build] ERROR: source executable not found: %SRC_EXE%
    exit /b 1
)

rem derive the output directory from the executable path (%~dp1 keeps the
rem trailing backslash and, unlike "\$(OutDir)\"" quoting, survives cmd parsing)
set "SRC_DIR=%~dp1"

if not exist "%DEST%" mkdir "%DEST%"
if not exist "%DEST%" (
    echo [post-build] ERROR: cannot create destination: %DEST%
    exit /b 1
)

set "FAILED=0"

copy /Y "%SRC_EXE%" "%DEST%\" >nul
if errorlevel 1 (
    echo [post-build] ERROR: failed to copy %~nx1
    set "FAILED=1"
) else (
    echo [post-build] copied "%~nx1"  -^>  "%DEST%"
)

rem dependency assemblies that sit next to the executable
for %%F in (OpenHardwareMonitorApi.dll LibreHardwareMonitorLib.dll) do (
    if exist "%SRC_DIR%%%F" (
        copy /Y "%SRC_DIR%%%F" "%DEST%\" >nul
        if errorlevel 1 (
            echo [post-build] ERROR: failed to copy %%F
            set "FAILED=1"
        ) else (
            echo [post-build] copied "%%F"  -^>  "%DEST%"
        )
    ) else (
        echo [post-build] ERROR: missing %%F in %SRC_DIR%
        set "FAILED=1"
    )
)

if "!FAILED!"=="1" (
    echo [post-build] one or more files could not be copied ^(is TrafficMonitor still running?^)
    exit /b 1
)

echo [post-build] deployment up to date.
exit /b 0
