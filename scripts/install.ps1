param(
  [string]$Version = $env:CLOT_VERSION,
  [string]$InstallDir = $env:CLOT_INSTALL_DIR,
  [string]$BinDir = $env:CLOT_BIN_DIR,
  [string]$Url = $env:CLOT_URL
)

$ErrorActionPreference = "Stop"
$Repo = "jclot/ClotLang"

if (-not $InstallDir -or $InstallDir.Trim().Length -eq 0) {
  $InstallDir = Join-Path $env:LOCALAPPDATA "Clot"
}
if (-not $BinDir -or $BinDir.Trim().Length -eq 0) {
  $BinDir = Join-Path $InstallDir "bin"
}

switch ($env:PROCESSOR_ARCHITECTURE) {
  "AMD64" { $Arch = "x86_64" }
  "ARM64" { $Arch = "arm64" }
  default { throw "Unsupported architecture: $($env:PROCESSOR_ARCHITECTURE)" }
}

$Asset = "clot-windows-$Arch.zip"
if (-not $Url -or $Url.Trim().Length -eq 0) {
  if ($Version -and $Version.Trim().Length -gt 0) {
    $Url = "https://github.com/$Repo/releases/download/$Version/$Asset"
  } else {
    $Url = "https://github.com/$Repo/releases/latest/download/$Asset"
  }
}

$TempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("clot-install-" + [System.Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $TempDir | Out-Null

try {
  $Archive = Join-Path $TempDir $Asset
  Invoke-WebRequest -Uri $Url -OutFile $Archive

  $ExtractDir = Join-Path $TempDir "extract"
  Expand-Archive -Path $Archive -DestinationPath $ExtractDir -Force

  $Bin = Get-ChildItem -Path $ExtractDir -Recurse -Filter "clot.exe" | Select-Object -First 1
  if (-not $Bin) {
    $Bin = Get-ChildItem -Path $ExtractDir -Recurse -Filter "clot" | Select-Object -First 1
  }
  if (-not $Bin) {
    throw "clot binary not found in the archive."
  }

  New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
  Copy-Item -Path $Bin.FullName -Destination (Join-Path $BinDir "clot.exe") -Force

  # Install the bundled standard library so `import clot.core.exceptions;` and
  # friends resolve from anywhere. The binary looks for it at <InstallDir>\lib\clot.
  $LibSrc = Get-ChildItem -Path $ExtractDir -Recurse -Directory |
    Where-Object { $_.FullName -match "[\\/]lib[\\/]clot$" } | Select-Object -First 1
  if ($LibSrc) {
    $LibDest = Join-Path $InstallDir "lib"
    New-Item -ItemType Directory -Path $LibDest -Force | Out-Null
    $ClotLibDest = Join-Path $LibDest "clot"
    if (Test-Path $ClotLibDest) { Remove-Item -Path $ClotLibDest -Recurse -Force }
    Copy-Item -Path $LibSrc.FullName -Destination $ClotLibDest -Recurse -Force
    Write-Host "Standard library: $ClotLibDest"
  } else {
    Write-Warning "Bundled standard library not found in the archive; 'import clot.*' may not resolve."
  }

  $PathValue = [Environment]::GetEnvironmentVariable("Path", "User")
  $PathParts = @()
  if ($PathValue) { $PathParts = $PathValue -split ";" }
  if (-not ($PathParts | Where-Object { $_ -eq $BinDir })) {
    if ($PathValue -and $PathValue.Trim().Length -gt 0) {
      $NewPath = "$PathValue;$BinDir"
    } else {
      $NewPath = $BinDir
    }
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "User")
    Write-Host "Added to user PATH: $BinDir"
  } else {
    Write-Host "Bin dir already on PATH: $BinDir"
  }

  Write-Host "Installed: $BinDir\clot.exe"
  Write-Host "Restart your terminal to use 'clot'."
} finally {
  Remove-Item -Path $TempDir -Recurse -Force -ErrorAction SilentlyContinue
}
