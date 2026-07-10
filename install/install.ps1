$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
$Host.UI.RawUI.WindowTitle = "Pratibimb Setup"
$ESC = [char]27

function Write-Gradient {
    param([string]$text, [int]$r1, [int]$g1, [int]$b1, [int]$r2, [int]$g2, [int]$b2, [int]$delayMs = 0)
    $len = $text.Length
    if ($len -eq 0) { return }
    $out = ""
    for ($i = 0; $i -lt $len; $i++) {
        $r = [Math]::Round($r1 + ($r2 - $r1) * ($i / $len))
        $g = [Math]::Round($g1 + ($g2 - $g1) * ($i / $len))
        $b = [Math]::Round($b1 + ($b2 - $b1) * ($i / $len))
        $out += "$ESC[38;2;$r;$g;${b}m" + $text[$i]
    }
    Write-Host ($out + "$ESC[0m")
    if ($delayMs -gt 0) { Start-Sleep -Milliseconds $delayMs }
}

function Show-Progress {
    param([string]$task)
    $spinners = @("⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏")
    for ($i = 0; $i -lt 12; $i++) {
        $spin = $spinners[$i % $spinners.Length]
        $color = "$ESC[38;2;100;200;255m"
        Write-Host -NoNewline "`r    $color$spin$ESC[0m $task..."
        Start-Sleep -Milliseconds 40
    }
}

Clear-Host
Write-Host ""
Write-Gradient "    ██████╗ ██████╗  █████╗ ████████╗██╗██████╗ ██╗███╗   ███╗██████╗ " 255 50 150 255 150 50 40
Write-Gradient "    ██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██║██╔══██╗██║████╗ ████║██╔══██╗" 255 70 140 255 170 50 40
Write-Gradient "    ██████╔╝██████╔╝███████║   ██║   ██║██████╔╝██║██╔████╔██║██████╔╝" 255 90 130 255 190 50 40
Write-Gradient "    ██╔═══╝ ██╔══██╗██╔══██║   ██║   ██║██╔══██╗██║██║╚██╔╝██║██╔══██╗" 255 110 120 255 210 50 40
Write-Gradient "    ██║     ██║  ██║██║  ██║   ██║   ██║██████╔╝██║██║ ╚═╝ ██║██████╔╝" 255 130 110 255 230 50 40
Write-Gradient "    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝╚═════╝ ╚═╝╚═╝     ╚═╝╚═════╝ " 255 150 100 255 255 50 40
Write-Host ""
Write-Host "    $ESC[38;2;150;150;150mSync your desktop wallpaper to your New Tab page.$ESC[0m"
Write-Host ""
Start-Sleep -Milliseconds 200

$installDir = "$env:LOCALAPPDATA\Pratibimb"
$hostDir = "$installDir\host"
$extDir = "$installDir\extension"

New-Item -ItemType Directory -Force -Path $hostDir | Out-Null
New-Item -ItemType Directory -Force -Path $extDir | Out-Null

$repoBase = "https://raw.githubusercontent.com/SunTzv/Pratibimb/main"

Write-Host "  $ESC[38;2;100;150;255m[1/4]$ESC[0m Downloading assets"
Show-Progress "Fetching host binary"
Invoke-WebRequest -Uri "$repoBase/host/pratibimb_host.exe" -OutFile "$hostDir\pratibimb_host.exe" -UseBasicParsing
Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Fetched host binary             "

Show-Progress "Fetching extension package"
Invoke-WebRequest -Uri "$repoBase/extension.zip" -OutFile "$installDir\extension.zip" -UseBasicParsing
Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Fetched extension package       "

Write-Host "  $ESC[38;2;100;150;255m[2/4]$ESC[0m Extracting contents"
Show-Progress "Unpacking files"
Expand-Archive -LiteralPath "$installDir\extension.zip" -DestinationPath $extDir -Force
Remove-Item "$installDir\extension.zip"
Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Unpacked files                  "

Write-Host "  $ESC[38;2;100;150;255m[3/4]$ESC[0m Configuring Native Messaging"
$extId = "cbcdepgnlldcpbigcgjdkmnjcoekggji"
$manifestPath = "$hostDir\com.suntzv.pratibimb.json"
$escapedHostPath = "$hostDir\pratibimb_host.exe" -replace "\\", "\\"

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

Show-Progress "Writing manifest"
$manifest | Out-File -FilePath $manifestPath -Encoding UTF8
Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Manifest created                "

Write-Host "  $ESC[38;2;100;150;255m[4/4]$ESC[0m Registering browsers"
$browsers = @(
    "Software\Google\Chrome\NativeMessagingHosts",
    "Software\BraveSoftware\Brave-Browser\NativeMessagingHosts",
    "Software\Microsoft\Edge\NativeMessagingHosts",
    "Software\Chromium\NativeMessagingHosts",
    "Software\Opera Software\Opera Stable\NativeMessagingHosts",
    "Software\Vivaldi\Vivaldi\NativeMessagingHosts"
)

Show-Progress "Updating registry"
foreach ($b in $browsers) {
    $regPath = "HKCU:\$b\com.suntzv.pratibimb"
    New-Item -Path $regPath -Force | Out-Null
    New-ItemProperty -Path $regPath -Name "(default)" -Value $manifestPath -Force | Out-Null
}
Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Registry updated                "

Write-Host ""
Write-Gradient "  ✨ Installation Complete! ✨" 50 255 150 50 150 255 30
Write-Host ""

Write-Host "  $ESC[38;2;255;255;255mFinal Steps:$ESC[0m"
Write-Host "  1. The extension folder has been opened for you."
Write-Host "  2. Go to your browser's extensions page (e.g. $ESC[38;2;100;200;255mchrome://extensions$ESC[0m)"
Write-Host "  3. Turn on $ESC[38;2;255;200;100m'Developer mode'$ESC[0m."
Write-Host "  4. Click $ESC[38;2;255;200;100m'Load unpacked'$ESC[0m and select the opened folder."
Write-Host ""
Write-Host "  $ESC[38;2;150;255;150mOpen a new tab to see the magic. 🎉$ESC[0m"
Write-Host ""

Start-Sleep -Milliseconds 500
Invoke-Item $extDir
