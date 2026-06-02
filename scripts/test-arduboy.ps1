$ErrorActionPreference = "Stop"
. "$PSScriptRoot\arduboy-env.ps1"

$Sketches = Get-ChildItem -Path (Join-Path $RepoRoot "arduboy") -Recurse -Filter "*.ino"

if ($Sketches.Count -eq 0) {
  throw "No Arduboy sketches found under arduboy\."
}

foreach ($Sketch in $Sketches) {
  $SketchDirName = Split-Path (Split-Path $Sketch.FullName -Parent) -Leaf
  $SketchName = [System.IO.Path]::GetFileNameWithoutExtension($Sketch.Name)

  if ($SketchDirName -ne $SketchName) {
    throw "Arduino sketch folder must match .ino filename: $($Sketch.FullName)"
  }

  & "$PSScriptRoot\build-arduboy.ps1" -Sketch (Resolve-Path -Relative $Sketch.FullName)
}
