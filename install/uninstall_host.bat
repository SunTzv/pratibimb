@echo off
:: Remove the registry key from Brave
REG DELETE "HKCU\Software\BraveSoftware\Brave-Browser\NativeMessagingHosts\com.suntzv.pratibimb" /f

echo.
echo ===================================================
echo   Pratibimb Uninstalled Successfully!
echo ===================================================
pause