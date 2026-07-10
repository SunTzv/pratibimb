$ErrorActionPreference = "Continue"

Write-Host "========================================="
Write-Host "       Pratibimb Windows Uninstaller     "
Write-Host "========================================="
Write-Host ""

$installDir = "$env:LOCALAPPDATA\Pratibimb"

Write-Host "Removing registry entries..."
$browsers = @(
    "Software\Google\Chrome\NativeMessagingHosts",
    "Software\BraveSoftware\Brave-Browser\NativeMessagingHosts",
    "Software\Microsoft\Edge\NativeMessagingHosts",
    "Software\Chromium\NativeMessagingHosts",
    "Software\Opera Software\Opera Stable\NativeMessagingHosts",
    "Software\Vivaldi\Vivaldi\NativeMessagingHosts"
)

foreach ($b in $browsers) {
    $regPath = "HKCU:\$b\com.suntzv.pratibimb"
    if (Test-Path "HKCU:\$b") {
        # Only attempt remove if the com.suntzv.pratibimb key actually exists
        if (Test-Path $regPath) {
            Remove-Item -Path $regPath -Recurse -Force
            Write-Host "  -> Removed from $b"
        }
    }
}

Write-Host "Removing files from AppData..."
if (Test-Path $installDir) {
    Remove-Item -Path $installDir -Recurse -Force
    Write-Host "  -> Removed $installDir"
}

Write-Host ""
Write-Host "Uninstallation complete. Don't forget to remove the extension from your browser!"
