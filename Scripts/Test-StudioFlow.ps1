param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [string]$PackageDirectory,
    [switch]$UseDefaultTimings,
    [switch]$EnableAudio,
    [switch]$UseSuppliedAudio,
    [ValidateRange(640,3840)][int]$ResolutionX = 1280,
    [ValidateRange(360,2160)][int]$ResolutionY = 720,
    [ValidatePattern('^[A-Za-z0-9_]*$')][string]$ReportSuffix = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$isPackage = -not [string]::IsNullOrWhiteSpace($PackageDirectory)
if ($isPackage) {
    $program = @((Join-Path $PackageDirectory 'Windows\MyProject\Binaries\Win64\MyProject.exe'),
        (Join-Path $PackageDirectory 'MyProject\Binaries\Win64\MyProject.exe')) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (-not $program) { throw 'Cooked Development executable not found in PackageDirectory.' }
    $arguments = @(); $runName = 'StudioPackaged'
} else {
    $program = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $arguments = @((Join-Path $projectRoot 'MyProject.uproject')); $runName = 'StudioPlay'
}
if (-not (Test-Path -LiteralPath $program)) { throw 'Game/Editor executable not found.' }
if ($UseDefaultTimings) { $runName += 'FullTiming' }
$runName += $ReportSuffix
$directory = Join-Path $projectRoot "Saved\Automation\$runName"
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $directory,$logDirectory | Out-Null
# Deliberately no map URL, GameMode override, ExecCmds, Movie.Test or StudioSkipMenu.
$arguments += @('-game','-StudioSkipStartupVideos','-StudioSmoke','-StudioScreenshots','-Unattended','-NoSplash','-NoP4','-NoLiveCoding',
    '-RenderOffscreen','-Windowed',"-ResX=$ResolutionX","-ResY=$ResolutionY",'-ForceRes',"-StudioReportDir=$directory",'-StdOut','-FullStdOutLogOutput',"-AbsLog=$logDirectory\$runName.engine.log")
if (-not $UseDefaultTimings) { $arguments += '-StudioFastTest' }
if ($UseSuppliedAudio) { $arguments += '-StudioSuppliedAudio'; $EnableAudio=$true }
elseif ($EnableAudio) { $arguments += '-StudioAudioProbe' } else { $arguments += '-NoSound' }
$started = [DateTime]::UtcNow
& $program @arguments | Tee-Object -FilePath (Join-Path $logDirectory "$runName.log")
$processExit = $LASTEXITCODE
$path = Join-Path $directory 'result.json'
if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTimeUtc -lt $started) { throw "Fresh studio report missing: $path" }
$result = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
foreach ($field in @('passed','menuPages','introduction','audioPreferences','initialDefaults10And30','wardrobeInputLocked',
    'keyboardRestartCleared','movementInput','replayPreserved','newMovieCleared','zeroMovieRated','returnMenuCleared','playAgain','quitButton')) {
    if (-not $result.$field) { throw "Studio check failed: $field. See $path" }
}
if ($processExit -ne 0 -or $result.wardrobes -ne 8 -or $result.readyStarts -ne 6 -or $result.timeoutStarts -ne 1 -or
    $result.titleCards -ne 3 -or $result.playbacks -ne 3 -or $result.transitionCards -ne 6 -or $result.firstMovieScore -ne 100 -or $result.stars -ne 5 -or
    ($UseDefaultTimings -and -not $result.fullTiming)) { throw "Complete studio loop failed: $path (exit $processExit)" }
if (@($result.directorCues).Count -ne 5 -or @($result.directorCues | Where-Object { $_ -lt 1 }).Count) { throw 'Some director cues were not exercised.' }
if ($EnableAudio -and ((-not $UseSuppliedAudio -and -not $result.syntheticAudioProbe) -or ($UseSuppliedAudio -and -not $result.suppliedAudioProbe) -or -not $result.musicPlayback -or -not $result.directorPlayback -or -not $result.actingSoundPlayback -or -not $result.audioCleanup)) { throw 'Audio playback/cleanup checks failed.' }
Write-Output "Studio game flow passed ($runName). Report: $path"
exit 0
