$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$ArduinoCli = Join-Path $RepoRoot ".tools\arduino-cli\arduino-cli.exe"
$ArduinoConfig = Join-Path $RepoRoot ".arduino\arduino-cli.yaml"
$DefaultFqbn = "arduino:avr:leonardo"

function Get-ArduboyCli {
  if (-not (Test-Path $ArduinoCli)) {
    throw "Arduino CLI not found at $ArduinoCli. Run scripts\setup-arduboy.ps1."
  }

  return $ArduinoCli
}

function Invoke-ArduboyCli {
  param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $Arguments
  )

  & (Get-ArduboyCli) --config-file $ArduinoConfig @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "arduino-cli failed with exit code $LASTEXITCODE"
  }
}
