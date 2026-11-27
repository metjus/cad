# Stages an installed HybridCAD build (exe + runtime DLLs + Qt platform
# plugin) into a folder that `vpk pack` turns into an installer/update.
param(
  [Parameter(Mandatory)] [string] $BuildDir,
  [Parameter(Mandatory)] [string] $StageDir,
  [Parameter(Mandatory)] [string] $Prefix   # conda Library prefix with OCCT/Qt
)
$ErrorActionPreference = 'Stop'
if (Test-Path $StageDir) { Remove-Item -Recurse -Force $StageDir }
cmake --install $BuildDir --prefix $StageDir
if ($LASTEXITCODE -ne 0) { throw "cmake --install failed" }
New-Item -ItemType Directory -Force "$StageDir/platforms" | Out-Null
foreach ($p in @("$Prefix/lib/qt6/plugins/platforms/qwindows.dll", "$Prefix/plugins/platforms/qwindows.dll")) {
  if (Test-Path $p) { Copy-Item $p "$StageDir/platforms/"; break }
}
if (-not (Test-Path "$StageDir/platforms/qwindows.dll")) { throw "qwindows.dll not found" }
if (-not (Test-Path "$StageDir/HybridCAD.exe")) { throw "HybridCAD.exe missing from stage" }
if (-not (Get-ChildItem "$StageDir/velopack_libc*.dll" -ErrorAction SilentlyContinue)) { Write-Warning "Velopack DLL not staged" }
$size = (Get-ChildItem $StageDir -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB
"Staged {0} ({1:N1} MB)" -f $StageDir, $size
