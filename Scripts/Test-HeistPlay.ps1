param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [switch]$RenderScreenshots,
    [switch]$UseLevelSetup,
    [string]$PackageDirectory,
    [ValidatePattern('^/Game/[A-Za-z0-9_/]+$')][string]$Map,
    [ValidatePattern('^[A-Za-z0-9_]*$')][string]$ReportSuffix = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if ($PackageDirectory) {
    $editor = @((Join-Path $PackageDirectory 'Windows\MyProject\Binaries\Win64\MyProject.exe'),
        (Join-Path $PackageDirectory 'MyProject\Binaries\Win64\MyProject.exe')) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not (Test-Path -LiteralPath $editor)) { throw 'UnrealEditor-Cmd.exe not found.' }
$startedAt = [DateTime]::UtcNow
$arguments = @()
if (-not $PackageDirectory) { $arguments += Join-Path $projectRoot 'MyProject.uproject' }
# Keep the synthetic legacy fixture isolated from the bank's permanently placed zones.
# UseLevelSetup without Map deliberately tests the saved startup-map configuration.
if (-not $Map -and -not $UseLevelSetup) { $Map = '/Game/ThirdPerson/Lvl_ThirdPerson' }
if ($Map) { $arguments += "$Map`?game=/Script/MyProject.StudioGameMode" }
$runName = 'HeistPlay' + $(if ($PackageDirectory) { 'Packaged' }) + $ReportSuffix
$reportDirectory = Join-Path $projectRoot "Saved\Automation\$runName"
$arguments += @('-game', '-StudioSkipMenu', '-ObjectiveSmoke', '-Unattended',
    '-NoSound', '-NoSplash', '-NoP4', '-NoLiveCoding', '-RenderOffscreen', '-Windowed', '-ResX=1280', '-ResY=720', '-ForceRes',
    '-ExecCmds=sg.ShadowQuality 1,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,Movie.ObjectiveSmoke',
    "-HeistReportDir=$reportDirectory", '-StdOut', '-FullStdOutLogOutput')
if ($UseLevelSetup) { $arguments += '-UseLevelHeistSetup' }
if ($RenderScreenshots) { $arguments += '-ObjectiveSmokeScreenshots' } else { $arguments += '-NullRHI' }
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
& $editor @arguments | Tee-Object -FilePath (Join-Path $logDirectory "$runName.log")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$report = Join-Path $reportDirectory 'result.json'
if (-not (Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $startedAt) { throw 'Fresh heist Play report not produced.' }
$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if (-not $result.passed -or $result.preparations -ne 3 -or $result.finalMovieScore -ne 100) { throw "Heist Play test failed: $report" }
if ($UseLevelSetup -and (-not $result.usedLevelSetup -or -not $result.doorWalkVerified)) { throw "Bank level movement/objective check failed: $report" }
Write-Output "Heist Play test passed. Report: $report"
exit 0

