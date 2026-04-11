@echo off
setlocal

:: 1. Figure out the absolute paths automatically
set "HOST_DIR=%~dp0..\host"
pushd "%HOST_DIR%"
set "HOST_DIR=%CD%"
popd

set "MANIFEST_PATH=%HOST_DIR%\com.suntzv.pratibimb.json"
set "EXE_PATH=%HOST_DIR%\pratibimb_host.exe"

:: Escape backslashes for the JSON format (C:\path becomes C:\\path)
set "EXE_PATH_ESCAPED=%EXE_PATH:\=\\%"

:: 2. Generate the Native Messaging Manifest dynamically
echo { > "%MANIFEST_PATH%"
echo   "name": "com.suntzv.pratibimb", >> "%MANIFEST_PATH%"
echo   "description": "Pratibimb Native Host Engine", >> "%MANIFEST_PATH%"
echo   "path": "%EXE_PATH_ESCAPED%", >> "%MANIFEST_PATH%"
echo   "type": "stdio", >> "%MANIFEST_PATH%"
echo   "allowed_origins": [ >> "%MANIFEST_PATH%"
echo     "chrome-extension://fnldepapgimkeegfohgdjnhjneoljcil/" >> "%MANIFEST_PATH%"
echo   ] >> "%MANIFEST_PATH%"
echo } >> "%MANIFEST_PATH%"

:: 3. Register the app for supported Chromium browsers
for %%B in (
    "HKCU\Software\Google\Chrome\NativeMessagingHosts"
    "HKCU\Software\BraveSoftware\Brave-Browser\NativeMessagingHosts"
    "HKCU\Software\Microsoft\Edge\NativeMessagingHosts"
    "HKCU\Software\Chromium\NativeMessagingHosts"
) do (
    REG ADD "%%~B\com.suntzv.pratibimb" /ve /t REG_SZ /d "%MANIFEST_PATH%" /f >nul 2>&1
)

echo.
echo ===================================================
echo   Pratibimb Engine Successfully Installed!
echo ===================================================
echo Native host registered for supported Chromium browsers.
echo.
pause