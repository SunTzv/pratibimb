@echo off
:: Remove registry keys from supported Chromium browsers
for %%B in (
    "HKCU\Software\Google\Chrome\NativeMessagingHosts\com.suntzv.pratibimb"
    "HKCU\Software\BraveSoftware\Brave-Browser\NativeMessagingHosts\com.suntzv.pratibimb"
    "HKCU\Software\Microsoft\Edge\NativeMessagingHosts\com.suntzv.pratibimb"
    "HKCU\Software\Chromium\NativeMessagingHosts\com.suntzv.pratibimb"
) do (
    REG DELETE "%%~B" /f >nul 2>&1
)

echo.
echo ===================================================
echo   Pratibimb Uninstalled Successfully!
echo ===================================================
pause