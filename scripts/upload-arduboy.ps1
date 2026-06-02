param(
  [Parameter(Mandatory = $true)]
  [string] $Port,

  [string] $Sketch = "arduboy\StreetFightDojo\StreetFightDojo.ino",
  [string] $Fqbn = "arduino:avr:leonardo"
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\arduboy-env.ps1"

$SketchPath = Resolve-Path (Join-Path $RepoRoot $Sketch)

Invoke-ArduboyCli upload --fqbn $Fqbn --port $Port $SketchPath
