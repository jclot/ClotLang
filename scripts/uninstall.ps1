param(
  [string]$InstallDir = $env:CLOT_INSTALL_DIR,
  [string]$BinDir = $env:CLOT_BIN_DIR
)

$ErrorActionPreference = "Stop"

if (-not $InstallDir -or $InstallDir.Trim().Length -eq 0) {
  $InstallDir = Join-Path $env:LOCALAPPDATA "Clot"
}
if (-not $BinDir -or $BinDir.Trim().Length -eq 0) {
  $BinDir = Join-Path $InstallDir "bin"
}

$Target = Join-Path $BinDir "clot.exe"
if (Test-Path $Target) {
  Remove-Item -Path $Target -Force
  Write-Host "Removed: $Target"
} else {
  Write-Host "Not found: $Target"
}

$PathValue = [Environment]::GetEnvironmentVariable("Path", "User")
if ($PathValue) {
  $Parts = $PathValue -split ";"
  $NewParts = $Parts | Where-Object { $_ -and $_ -ne $BinDir }
  if ($NewParts.Count -ne $Parts.Count) {
    $NewPath = ($NewParts -join ";")
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "User")
    Write-Host "Removed from user PATH: $BinDir"
  }
}

if (Test-Path $BinDir) {
  $Remaining = Get-ChildItem -Path $BinDir -ErrorAction SilentlyContinue
  if (-not $Remaining) {
    Remove-Item -Path $BinDir -Force
  }
}

Write-Host "Uninstall complete. Restart your terminal if needed."
