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
$wallpapersDir = "$installDir\wallpapers"

New-Item -ItemType Directory -Force -Path $hostDir | Out-Null
New-Item -ItemType Directory -Force -Path $extDir | Out-Null
New-Item -ItemType Directory -Force -Path $wallpapersDir | Out-Null

Write-Host "  $ESC[38;2;100;150;255m[1/6]$ESC[0m Select Installation Version"
Write-Host "    1) Stable Release (Recommended)"
Write-Host "    2) Latest Dev Build (main branch)"
$versionChoice = Read-Host "  Select an option [1]"

if ($versionChoice -eq "2") {
    Write-Host "  $ESC[38;2;100;150;255m[2/6]$ESC[0m Downloading latest dev build..."
    Show-Progress "Fetching latest code"
    $zipUrl = "https://github.com/SunTzv/Pratibimb/archive/refs/heads/main.zip"
} else {
    Write-Host "  $ESC[38;2;100;150;255m[2/6]$ESC[0m Downloading latest stable release..."
    Show-Progress "Fetching latest stable release"
    $releaseApiUrl = "https://api.github.com/repos/SunTzv/Pratibimb/releases/latest"
    $releaseInfo = Invoke-RestMethod -Uri $releaseApiUrl
    $zipUrl = $releaseInfo.zipball_url
}

Invoke-WebRequest -Uri $zipUrl -OutFile "$installDir\release.zip" -UseBasicParsing
Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Download complete                   "

Write-Host "  $ESC[38;2;100;150;255m[3/6]$ESC[0m Extracting contents"
Show-Progress "Unpacking files"
Expand-Archive -LiteralPath "$installDir\release.zip" -DestinationPath "$installDir\temp" -Force
Remove-Item "$installDir\release.zip" -Force

$extractedDir = (Get-ChildItem -Path "$installDir\temp" -Directory | Select-Object -First 1).FullName
Copy-Item -Path "$extractedDir\extension\*" -Destination $extDir -Recurse -Force
if (Test-Path "$extractedDir\wallpapers") {
    Copy-Item -Path "$extractedDir\wallpapers\*" -Destination $wallpapersDir -Recurse -Force
}
Copy-Item -Path "$extractedDir\host\pratibimb_host.exe" -Destination "$hostDir\pratibimb_host.exe" -Force
Remove-Item "$installDir\temp" -Recurse -Force
Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Unpacked files                  "

Write-Host "  $ESC[38;2;100;150;255m[4/6]$ESC[0m Configuring Native Messaging"
$bytes = [System.Text.Encoding]::UTF8.GetBytes($extDir)
$sha256 = [System.Security.Cryptography.SHA256]::Create()
$hash = $sha256.ComputeHash($bytes)
$hexStr = [System.BitConverter]::ToString($hash, 0, 16).Replace("-", "").ToLower()
$extId = ""
foreach ($c in $hexStr.ToCharArray()) {
    if ($c -ge 'a') {
        $extId += [char]([int]$c + 10)
    } else {
        $extId += [char]([int]$c + 49)
    }
}

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

Write-Host "  $ESC[38;2;100;150;255m[5/6]$ESC[0m Registering browsers"
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

Write-Host "  $ESC[38;2;100;150;255m[6/6]$ESC[0m Whitelisting extension in Browser Policies"
$policyBrowsers = @(
    "Software\Policies\Google\Chrome",
    "Software\Policies\BraveSoftware\Brave",
    "Software\Policies\Microsoft\Edge",
    "Software\Policies\Chromium"
)

$policySuccess = $true
foreach ($pb in $policyBrowsers) {
    $allowlistPath = "HKCU:\$pb\ExtensionInstallAllowlist"
    try {
        if (-not (Test-Path $allowlistPath)) {
            New-Item -Path $allowlistPath -Force -ErrorAction Stop | Out-Null
        }
        
        $i = 1
        $alreadyExists = $false
        while ($true) {
            $prop = Get-ItemProperty -Path $allowlistPath -Name "$i" -ErrorAction SilentlyContinue
            if (-not $prop) {
                break
            }
            if ($prop."$i" -eq $extId) {
                $alreadyExists = $true
                break
            }
            $i++
        }
        if (-not $alreadyExists) {
            Set-ItemProperty -Path $allowlistPath -Name "$i" -Value $extId -ErrorAction Stop
        }
    } catch [System.UnauthorizedAccessException] {
        $policySuccess = $false
        break
    } catch {
        $policySuccess = $false
        break
    }
}

if ($policySuccess) {
    Write-Host "`r    $ESC[38;2;50;255;100m✓$ESC[0m Whitelist policies applied      "
} else {
    Write-Host "`r    $ESC[38;2;255;200;100m!$ESC[0m Could not apply policies (Requires Administrator)"
    Write-Host "      If Brave removes the extension, please run PowerShell as Administrator."
}

Write-Host ""
Write-Gradient "  ✨ Installation Complete! ✨" 50 255 150 50 150 255 30
Write-Host ""

Write-Host "  $ESC[38;2;255;255;255mFinal Steps:$ESC[0m"
Write-Host "  1. The Pratibimb folder has been opened for you."
Write-Host "  2. Go to your browser's extensions page (e.g. $ESC[38;2;100;200;255mchrome://extensions$ESC[0m)"
Write-Host "  3. Turn on $ESC[38;2;255;200;100m'Developer mode'$ESC[0m."
Write-Host "  4. Drag and drop the $ESC[38;2;255;200;100m'extension'$ESC[0m folder into the browser (or click 'Load unpacked')."
Write-Host ""
Write-Host "  $ESC[38;2;150;255;150mOpen a new tab to see the magic. 🎉$ESC[0m"
Write-Host ""

Start-Sleep -Milliseconds 500
Invoke-Item $installDir
