param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6'
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$projectFile = Join-Path $projectRoot 'MyProject.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$gameModule = Join-Path $projectRoot 'Binaries\Win64\UnrealEditor-MyProject.dll'
if (-not (Test-Path -LiteralPath $gameModule)) {
    throw 'Build the Development Editor target first with Scripts\Build.ps1. No gameplay tests have run.'
}
if (-not (Test-Path -LiteralPath $editor)) {
    throw 'UnrealEditor-Cmd.exe was not found. Pass -EngineRoot with the UE 5.6 installation path.'
}
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
$testArguments = @($projectFile, '-Unattended', '-NullRHI', '-NoSound', '-NoSplash', '-NoP4', '-NoLiveCoding',
    '-ExecCmds=Automation RunTests OneManStudio.Acting', '-TestExit=Automation Test Queue Empty',
    "-ReportExportPath=$projectRoot\Saved\Automation\Recording", "-Log=$logDirectory\RecordingTests.log",
    '-StdOut', '-FullStdOutLogOutput')
$runStartedAt = [DateTime]::UtcNow
& $editor @testArguments
$editorExitCode = $LASTEXITCODE
if ($editorExitCode -ne 0) { exit $editorExitCode }

# TestExit can return zero even when individual automation tests failed. Validate
# the newly exported report so failed or missing tests cannot produce a false pass.
$reportPath = Join-Path $projectRoot 'Saved\Automation\Recording\index.json'
if (-not (Test-Path -LiteralPath $reportPath) -or (Get-Item -LiteralPath $reportPath).LastWriteTimeUtc -lt $runStartedAt) {
    throw 'Unreal did not export a fresh automation report. No successful test run can be confirmed.'
}
$report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
$expectedTests = @('RecordingData', 'RecorderSampling', 'SharedClockAndReplay', 'ReplayCollision')
foreach ($testName in $expectedTests) {
    $testResult = @($report.tests | Where-Object { $_.fullTestPath -eq "OneManStudio.Acting.$testName" })
    if ($testResult.Count -ne 1 -or $testResult[0].state -ne 'Success') {
        throw "Automation test did not pass: OneManStudio.Acting.$testName. See $reportPath"
    }
}
if ($report.failed -gt 0 -or $report.notRun -gt 0 -or $report.inProcess -gt 0) {
    throw "Automation report contains unsuccessful tests. See $reportPath"
}
Write-Output "Automation passed: $($report.succeeded + $report.succeededWithWarnings) tests. Report: $reportPath"
exit 0
