$ErrorActionPreference = "Continue"
$ProgressPreference = "SilentlyContinue"
$Host.UI.RawUI.WindowTitle = "Pratibimb Uninstaller"
$ESC = [char]27

function Write-Gradient {
    param([string]$text, [int]$r1, [int]$g1, [int]$b1, [int]$r2, [int]$g2, [int]$b2)
    $len = $text.Length
    if ($len -eq 0) { return }
    $out = ""
    for ($i = 0; $i < $len; $i++) {
        $r = [Math]::Round($r1 + ($r2 - $r1) * ($i / $len))
        $g = [Math]::Round($g1 + ($g2 - $g1) * ($i / $len))
        $b = [Math]::Round($b1 + ($b2 - $b1) * ($i / $len))
        $out += "$ESC[38;2;$r;$g;${b}m" + $text[$i]
    }
    Write-Host ($out + "$ESC[0m")
}

Clear-Host
Write-Host ""
Write-Gradient "    ██████╗ ██████╗  █████╗ ████████╗██╗██████╗ ██╗███╗   ███╗██████╗ " 255 100 100 255 150 100
Write-Gradient "    ██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██║██╔══██╗██║████╗ ████║██╔══██╗" 255 120 100 255 170 100
Write-Gradient "    ██████╔╝██████╔╝███████║   ██║   ██║██████╔╝██║██╔████╔██║██████╔╝" 255 140 100 255 190 100
Write-Gradient "    ██╔═══╝ ██╔══██╗██╔══██║   ██║   ██║██╔══██╗██║██║╚██╔╝██║██╔══██╗" 255 160 100 255 210 100
Write-Gradient "    ██║     ██║  ██║██║  ██║   ██║   ██║██████╔╝██║██║ ╚═╝ ██║██████╔╝" 255 180 100 255 230 100
Write-Gradient "    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝╚═════╝ ╚═╝╚═╝     ╚═╝╚═════╝ " 255 200 100 255 255 100
Write-Host ""
Write-Host "    $ESC[38;2;150;150;150mRemoving Pratibimb...$ESC[0m"
Write-Host ""

$installDir = "$env:LOCALAPPDATA\Pratibimb"

Write-Host "  $ESC[38;2;255;150;100m[1/2]$ESC[0m Removing registry entries"
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
    if (Test-Path $regPath) {
        Remove-Item -Path $regPath -Recurse -Force
        Write-Host "    $ESC[38;2;255;100;100m-$ESC[0m Removed from $b"
    }
}

Write-Host "  $ESC[38;2;255;150;100m[2/2]$ESC[0m Removing files from AppData"
if (Test-Path $installDir) {
    Remove-Item -Path $installDir -Recurse -Force
    Write-Host "    $ESC[38;2;255;100;100m-$ESC[0m Removed $installDir"
}

Write-Host ""
Write-Gradient "  ✨ Uninstallation Complete! ✨" 255 150 100 255 100 150
Write-Host ""
Write-Host "  $ESC[38;2;255;255;255mDon't forget to remove the extension from your browser!$ESC[0m"
Write-Host ""
