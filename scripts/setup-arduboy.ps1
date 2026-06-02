$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$ToolRoot = Join-Path $RepoRoot ".tools\arduino-cli"
$ArduinoCli = Join-Path $ToolRoot "arduino-cli.exe"
$ArduinoConfig = Join-Path $RepoRoot ".arduino\arduino-cli.yaml"
$ArduinoCliVersion = "1.5.0"
$ArchiveName = "arduino-cli_$ArduinoCliVersion`_Windows_64bit.zip"
$ArchiveUrl = "https://github.com/arduino/arduino-cli/releases/download/v$ArduinoCliVersion/$ArchiveName"
$ArchivePath = Join-Path $env:TEMP $ArchiveName

New-Item -ItemType Directory -Force -Path $ToolRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $ArduinoConfig -Parent) | Out-Null

if (-not (Test-Path $ArduinoCli)) {
  Invoke-WebRequest -Uri $ArchiveUrl -OutFile $ArchivePath
  Expand-Archive -LiteralPath $ArchivePath -DestinationPath $ToolRoot -Force
}

if (-not (Test-Path $ArduinoConfig)) {
  & $ArduinoCli config init --dest-file $ArduinoConfig --overwrite
}

& $ArduinoCli --config-file $ArduinoConfig config set directories.user ".\arduino-sketchbook"
& $ArduinoCli --config-file $ArduinoConfig config set directories.data ".\.arduino\data"
& $ArduinoCli --config-file $ArduinoConfig config set directories.downloads ".\.arduino\downloads"
& $ArduinoCli --config-file $ArduinoConfig core update-index
& $ArduinoCli --config-file $ArduinoConfig core install arduino:avr
& $ArduinoCli --config-file $ArduinoConfig lib install Arduboy2 ArduboyTones

& "$PSScriptRoot\test-arduboy.ps1"
