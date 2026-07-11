$ErrorActionPreference = "Stop"

$ESC = [char]27
$GREEN = "$ESC[38;2;50;255;100m"
$BLUE = "$ESC[38;2;100;150;255m"
$NC = "$ESC[0m"

function Show-Progress {
    param([string]$Message)
    Write-Host -NoNewline "`r    $ESC[38;2;150;150;150m⟳$ESC[0m $Message..."
}

Write-Host ""
Write-Host "  $ESC[38;2;150;200;255mPratibimb Updater$ESC[0m"
Write-Host "  $ESC[38;2;100;100;100m=================================$ESC[0m"
Write-Host ""

$installDir = "$env:LOCALAPPDATA\Pratibimb"
$extDir = "$installDir\extension"

if (-not (Test-Path $installDir)) {
    Write-Host "  $ESC[38;2;255;100;100mError:$ESC[0m Pratibimb is not installed! Run install.ps1 first."
    exit 1
}

$releaseApiUrl = "https://api.github.com/repos/SunTzv/Pratibimb/releases/latest"

Write-Host "  $BLUE[1/2]$NC Downloading extension package"
Show-Progress "Fetching latest stable release"
$releaseInfo = Invoke-RestMethod -Uri $releaseApiUrl
$zipUrl = $releaseInfo.zipball_url
Invoke-WebRequest -Uri $zipUrl -OutFile "$installDir\release.zip" -UseBasicParsing
Write-Host "`r    $GREEN✓$NC Fetched latest stable release       "

Write-Host "  $BLUE[2/2]$NC Updating files"
Show-Progress "Extracting files"
Expand-Archive -LiteralPath "$installDir\release.zip" -DestinationPath "$installDir\temp" -Force
Remove-Item "$installDir\release.zip" -Force

$extractedDir = (Get-ChildItem -Path "$installDir\temp" -Directory | Select-Object -First 1).FullName
Copy-Item -Path "$extractedDir\extension\*" -Destination $extDir -Recurse -Force
Remove-Item "$installDir\temp" -Recurse -Force
Write-Host "`r    $GREEN✓$NC Files updated successfully        "

Write-Host ""
Write-Host "  ✨ Update Complete! ✨"
Write-Host "  Go to chrome://extensions and click the 'Update' or reload button for Pratibimb."
Write-Host ""
