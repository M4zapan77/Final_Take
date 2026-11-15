param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6'
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$projectFile = Join-Path $projectRoot 'MyProject.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath (Join-Path $projectRoot 'Binaries\Win64\UnrealEditor-MyProject.dll'))) {
    throw 'Build Development Editor first with Scripts\Build.ps1.'
}
if (-not (Test-Path -LiteralPath $editor)) { throw 'UnrealEditor-Cmd.exe not found. Check -EngineRoot.' }
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
$runStartedAt = [DateTime]::UtcNow
$testArguments = @($projectFile, '-Unattended', '-NullRHI', '-NoSound', '-NoSplash', '-NoP4', '-NoLiveCoding',
    '-ExecCmds=Automation RunTests OneManStudio.', '-TestExit=Automation Test Queue Empty',
    "-ReportExportPath=$projectRoot\Saved\Automation\Movie", "-Log=$logDirectory\MovieTests.log", '-StdOut', '-FullStdOutLogOutput')
& $editor @testArguments | Tee-Object -FilePath (Join-Path $logDirectory 'MovieTests.log')
$editorExitCode = $LASTEXITCODE
if ($editorExitCode -ne 0) { exit $editorExitCode }
$reportPath = Join-Path $projectRoot 'Saved\Automation\Movie\index.json'
if (-not (Test-Path -LiteralPath $reportPath) -or (Get-Item -LiteralPath $reportPath).LastWriteTimeUtc -lt $runStartedAt) {
    throw 'A fresh automation report was not produced.'
}
$report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
$expectedPaths = @(
    'OneManStudio.Acting.RecordingData',
    'OneManStudio.Acting.RecorderSampling',
    'OneManStudio.Acting.SharedClockAndReplay',
    'OneManStudio.Acting.ReplayCollision',
    'OneManStudio.Movie.ConcurrentRecordingAndReplay',
    'OneManStudio.Movie.ThreeTakeLoop',
    'OneManStudio.Movie.CharacterMovementLifecycle',
    'OneManStudio.Movie.RestartAndCleanup',
    'OneManStudio.Movie.DefaultTimingAndFinishedRestart',
    'OneManStudio.ActingEvents.DataValidation',
    'OneManStudio.ActingEvents.CaptureAndGating',
    'OneManStudio.ActingEvents.PlaybackTimeline',
    'OneManStudio.ActingEvents.ThreeTakeMovieAndRestart',
    'OneManStudio.ActingEvents.AnimationPresentation',
    'OneManStudio.Wardrobe.CatalogAndFallback',
    'OneManStudio.Wardrobe.SnapshotIsolation',
    'OneManStudio.Wardrobe.ReadyAndTimeout',
    'OneManStudio.Wardrobe.ThreeTakeOutfits',
    'OneManStudio.Wardrobe.NativeWidget',
    'OneManStudio.Wardrobe.ExistingSkeleton',
    'OneManStudio.Objectives.DefinitionsAndValidation',
    'OneManStudio.Objectives.RobberRules',
    'OneManStudio.Objectives.AllRolesAndScores',
    'OneManStudio.Objectives.ProximityAtActionTime',
    'OneManStudio.Objectives.PropReplayAndReset',
    'OneManStudio.Objectives.ZeroAverageAndCap',
    'OneManStudio.Objectives.NativeHUD',
    'OneManStudio.Objectives.ZonesAndBadConfiguration',
    'OneManStudio.Ratings.ScoreThresholds',
    'OneManStudio.Ratings.AwardsAndTies',
    'OneManStudio.Ratings.ConditionalAwards',
    'OneManStudio.Premiere.TitleAndReplay',
    'OneManStudio.Premiere.ResultsButtons',
    'OneManStudio.Premiere.RestartAndCleanup',
    'OneManStudio.Premiere.ZeroScore',
    'OneManStudio.Premiere.Configuration',
    'OneManStudio.Studio.FrontEndAndIntroduction',
    'OneManStudio.Studio.TransitionsReplayAndMenu',
    'OneManStudio.Studio.RestartAndInputOwnership',
    'OneManStudio.Studio.UIAudioAndConfiguration',
    'OneManStudio.Startup.DefaultsAndValidation',
    'OneManStudio.Startup.MissingMediaFallback',
    'OneManStudio.Startup.SkipAndInputGuard',
    'OneManStudio.Startup.TravelSessionState',
    'OneManStudio.Customization.ProfileAndCatalog',
    'OneManStudio.Customization.FacialSnapshotIsolation',
    'OneManStudio.Customization.TwoHandsAndReset',
    'OneManStudio.Customization.UnifiedInterfaceAndGuard'
)
foreach ($expectedPath in $expectedPaths) {
    $result = @($report.tests | Where-Object { $_.fullTestPath -eq $expectedPath })
    if ($result.Count -ne 1 -or $result[0].state -ne 'Success') { throw "Test did not pass: $expectedPath. See $reportPath" }
}
if ($report.failed -gt 0 -or $report.notRun -gt 0 -or $report.inProcess -gt 0) { throw "Unsuccessful tests in $reportPath" }
Write-Output "Automation passed: $($report.succeeded + $report.succeededWithWarnings) tests. Report: $reportPath"
exit 0
