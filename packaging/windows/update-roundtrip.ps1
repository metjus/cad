# End-to-end proof of the update path (Phase 1 acceptance):
#   1. silent install of the older release with its Setup.exe
#   2. user data written outside the install folder
#   3. the installed app checks the feed, downloads the update (delta),
#      applies it and restarts itself into --update-test-verify
#   4. the restarted app reports the new version and the preserved settings
#   5. uninstall removes the program but leaves user data alone
param(
  [Parameter(Mandatory)] [string] $Setup,
  [Parameter(Mandatory)] [string] $Feed,
  [Parameter(Mandatory)] [string] $PackId,
  [Parameter(Mandatory)] [string] $Expected
)
$ErrorActionPreference = 'Stop'
$logs = Join-Path $pwd 'update-test-logs'
New-Item -ItemType Directory -Force $logs | Out-Null
$installRoot = Join-Path $env:LOCALAPPDATA $PackId
$dataDir = Join-Path $env:LOCALAPPDATA 'HybridCAD\HybridCAD'
$report = Join-Path $dataDir 'update-test.json'
$failures = 0
function Check($ok, $what) {
  if ($ok) { Write-Host "[PASS] $what" } else { Write-Host "[FAIL] $what"; $script:failures++ }
}
function Save-Logs {
  Copy-Item "$installRoot\*.log" $logs -ErrorAction SilentlyContinue
  Copy-Item "$dataDir\logs\*" $logs -ErrorAction SilentlyContinue
  Copy-Item $report $logs -ErrorAction SilentlyContinue
}

# 1. Install
Write-Host "Installing $Setup"
$p = Start-Process $Setup -ArgumentList '--silent' -PassThru -Wait
Check ($p.ExitCode -eq 0) "Setup.exe --silent exit code $($p.ExitCode)"
Start-Sleep -Seconds 5
Stop-Process -Name HybridCAD -Force -ErrorAction SilentlyContinue   # first-run launch, if any
$exe = Join-Path $installRoot 'current\HybridCAD.exe'
Check (Test-Path $exe) "installed to $exe"
Get-ChildItem $installRoot | Format-Table Name, Length
if (Test-Path "$installRoot\current\sq.version") { Get-Content "$installRoot\current\sq.version" }

# 2. User data that an update must not touch
New-Item -ItemType Directory -Force $dataDir | Out-Null
Set-Content (Join-Path $dataDir 'sentinel.txt') 'user data must survive updates'
Remove-Item $report -ErrorAction SilentlyContinue

# 3. Check, download and apply from inside the installed app
$p = Start-Process $exe -ArgumentList '--update-test', $Feed -PassThru -Wait
Check ($p.ExitCode -eq 0) "installed app found, downloaded and started the update (exit $($p.ExitCode))"

# 4. Wait for the restarted, updated app to report
for ($i = 0; $i -lt 180 -and -not (Test-Path $report); $i++) { Start-Sleep -Seconds 1 }
Save-Logs
Check (Test-Path $report) "updated app restarted and wrote its report"
if (Test-Path $report) {
  $r = Get-Content $report -Raw | ConvertFrom-Json
  $r | Format-List
  Check ($r.version -eq $Expected) "running version after restart is $($r.version) (expected $Expected)"
  Check ($r.marker -ne '') "settings written by the old version are visible to the new version"
  Check ($r.dataOutsideInstall) "user data directory is outside the install directory"
}
$log = Get-Content "$dataDir\logs\hybridcad.log" -Raw -ErrorAction SilentlyContinue
Check ($log -match 'delta update') "update was offered as a delta package"
Check (Test-Path (Join-Path $dataDir 'sentinel.txt')) "user data file survived the update"

# 5. Uninstall
$update = Join-Path $installRoot 'Update.exe'
if (Test-Path $update) {
  $p = Start-Process $update -ArgumentList '--uninstall', '--silent' -PassThru -Wait
  Write-Host "Update.exe --uninstall exit code $($p.ExitCode)"
  for ($i = 0; $i -lt 30 -and (Test-Path $exe); $i++) { Start-Sleep -Seconds 1 }
  Check (-not (Test-Path $exe)) "uninstall removed the program"
} else {
  Check $false "Update.exe present for uninstall"
}
Check (Test-Path (Join-Path $dataDir 'sentinel.txt')) "user data file survived uninstall"
Save-Logs
if ($failures -gt 0) { throw "$failures update round-trip check(s) failed" }
Write-Host "UPDATE ROUND TRIP PASSED"
