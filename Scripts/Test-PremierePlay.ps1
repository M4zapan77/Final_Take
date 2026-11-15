param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [string]$PackageDirectory,
    [ValidatePattern('^/Game/[A-Za-z0-9_/]+$')][string]$Map = '/Game/ThirdPerson/Lvl_ThirdPerson',
    [ValidatePattern('^[A-Za-z0-9_]*$')][string]$ReportSuffix = '',
    [switch]$RenderScreenshots
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$mapUrl = "$Map`?game=/Script/MyProject.StudioGameMode"
$isPackage = -not [string]::IsNullOrWhiteSpace($PackageDirectory)
if ($isPackage) {
    $candidates = @((Join-Path $PackageDirectory 'Windows\MyProject\Binaries\Win64\MyProject.exe'),
        (Join-Path $PackageDirectory 'MyProject\Binaries\Win64\MyProject.exe'))
    $program = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (-not $program) { throw 'Packaged Development MyProject.exe not found in PackageDirectory.' }
    $arguments = @($mapUrl)
    $runName = 'PremierePackaged'
} else {
    $program = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $arguments = @((Join-Path $projectRoot 'MyProject.uproject'), $mapUrl)
    $runName = 'PremierePlay'
}
if (-not (Test-Path -LiteralPath $program)) { throw "Game/Editor executable missing: $program" }
$runName += $ReportSuffix
$reportDirectory = Join-Path $projectRoot "Saved\Automation\$runName"
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $reportDirectory,$logDirectory | Out-Null
$arguments += @('-game','-StudioSkipMenu','-PremiereSmoke','-Unattended','-NoSound','-NoSplash','-NoP4','-NoLiveCoding',
    '-RenderOffscreen','-Windowed','-ResX=1280','-ResY=720','-ForceRes',"-PremiereReportDir=$reportDirectory",
    '-ExecCmds=sg.ShadowQuality 1,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,Movie.PremiereSmoke',
    '-StdOut','-FullStdOutLogOutput',"-AbsLog=$logDirectory\$runName.engine.log")
if ($RenderScreenshots) { $arguments += '-PremiereSmokeScreenshots' } else { $arguments += '-NullRHI' }
$startedAt = [DateTime]::UtcNow
& $program @arguments | Tee-Object -FilePath (Join-Path $logDirectory "$runName.log")
$processExit = $LASTEXITCODE
$report = Join-Path $reportDirectory 'result.json'
if (-not (Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $startedAt) { throw "Fresh premiere report missing: $report" }
$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if ($processExit -ne 0 -or -not $result.passed -or $result.preparations -ne 6 -or $result.titleCards -ne 3 -or $result.playbacks -ne 3 -or
    $result.firstMovieScore -ne 75 -or $result.stars -ne 4 -or -not $result.replayPreserved -or -not $result.newMovieCleared -or
    -not $result.zeroMovieRated -or -not $result.nativeGameModeStarted -or -not $result.movementInputVerified) { throw "Premiere integration failed: $report" }
Write-Output "Premiere integration passed ($runName). Report: $report"
exit 0
