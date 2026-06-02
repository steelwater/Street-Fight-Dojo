param(
  [string] $Sketch = "arduboy\StreetFightDojo\StreetFightDojo.ino",
  [string] $Fqbn = "arduino:avr:leonardo",
  [string] $Warnings = "default"
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\arduboy-env.ps1"

$SketchPath = Resolve-Path (Join-Path $RepoRoot $Sketch)
$BuildPath = Join-Path $RepoRoot "build\arduboy"

Invoke-ArduboyCli compile --fqbn $Fqbn --warnings $Warnings --output-dir $BuildPath $SketchPath
