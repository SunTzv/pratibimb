$ErrorActionPreference = "Stop"

Write-Host "========================================="
Write-Host "        Pratibimb Windows Installer      "
Write-Host "========================================="
Write-Host ""

$installDir = "$env:LOCALAPPDATA\Pratibimb"
$hostDir = "$installDir\host"
$extDir = "$installDir\extension"

# Create directories
New-Item -ItemType Directory -Force -Path $hostDir | Out-Null
New-Item -ItemType Directory -Force -Path $extDir | Out-Null

Write-Host "[1/4] Downloading files from GitHub..."
$repoBase = "https://raw.githubusercontent.com/SunTzv/Pratibimb/main"

Invoke-WebRequest -Uri "$repoBase/host/pratibimb_host.exe" -OutFile "$hostDir\pratibimb_host.exe"
Invoke-WebRequest -Uri "$repoBase/extension.zip" -OutFile "$installDir\extension.zip"

Write-Host "[2/4] Extracting extension..."
Expand-Archive -LiteralPath "$installDir\extension.zip" -DestinationPath $extDir -Force
Remove-Item "$installDir\extension.zip"

Write-Host "Opening extension folder..."
Invoke-Item $extDir

Write-Host ""
Write-Host "Before we continue, please install the extension in your browser:"
Write-Host "1. Go to your browser's extensions page (e.g., chrome://extensions or edge://extensions)."
Write-Host "2. Turn on 'Developer mode'."
Write-Host "3. Click 'Load unpacked' and select the opened 'extension' folder."
Write-Host "4. Copy the generated Extension ID."
Write-Host ""

$extId = Read-Host "Paste your Extension ID here"
while ($extId.Length -ne 32 -or $extId -notmatch '^[a-zA-Z]+$') {
    Write-Host "Invalid Extension ID. It should be 32 alphabetical characters."
    $extId = Read-Host "Paste your Extension ID here"
}

Write-Host "[3/4] Creating Native Messaging Manifest..."
$manifestPath = "$hostDir\com.suntzv.pratibimb.json"
$hostExePath = "$hostDir\pratibimb_host.exe"

$escapedHostPath = $hostExePath -replace "\\", "\\"

$manifest = @"
{
  "name": "com.suntzv.pratibimb",
  "description": "Pratibimb Native Host",
  "path": "$escapedHostPath",
  "type": "stdio",
  "allowed_origins": [
    "chrome-extension://$extId/"
  ]
}
"@

$manifest | Out-File -FilePath $manifestPath -Encoding UTF8

Write-Host "[4/4] Registering with browsers..."
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
    New-Item -Path $regPath -Force | Out-Null
    New-ItemProperty -Path $regPath -Name "(default)" -Value $manifestPath -Force | Out-Null
    Write-Host "  -> Registered for $b"
}

Write-Host ""
Write-Host "========================================="
Write-Host "          Installation Complete          "
Write-Host "========================================="
Write-Host "You can now open a new tab in your browser."
Write-Host "If it doesn't work immediately, try reloading the extension."
