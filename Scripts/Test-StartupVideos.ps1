param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [string]$PackageDirectory,
    [ValidateSet('Auto','SkipStudio','SkipGame','Failures')][string]$Scenario = 'Auto',
    [ValidateRange(640,3840)][int]$ResolutionX = 1280,
    [ValidateRange(360,2160)][int]$ResolutionY = 800
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$arguments = @()
if ($PackageDirectory) {
    $program = @((Join-Path $PackageDirectory 'Windows\MyProject\Binaries\Win64\MyProject.exe'),
        (Join-Path $PackageDirectory 'MyProject\Binaries\Win64\MyProject.exe'))|Where-Object {Test-Path -LiteralPath $_}|Select-Object -First 1
    $name = 'StartupPackaged' + $Scenario
} else {
    $program = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $arguments += Join-Path $projectRoot 'MyProject.uproject'; $name = 'StartupPlay' + $Scenario
}
if (-not $program -or -not (Test-Path -LiteralPath $program)) { throw 'Editor/game executable missing.' }
$directory=Join-Path $projectRoot "Saved\Automation\$name"; $logDirectory=Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $directory,$logDirectory|Out-Null
$arguments += @('-game','-StartupVideoSmoke',"-VideoScenario=$Scenario","-VideoReportDir=$directory",'-Unattended','-NoSplash','-NoP4','-NoLiveCoding',
    '-RenderOffscreen','-Windowed',"-ResX=$ResolutionX","-ResY=$ResolutionY",'-ForceRes','-StdOut','-FullStdOutLogOutput',"-AbsLog=$logDirectory\$name.engine.log")
$started=[DateTime]::UtcNow
& $program @arguments | Tee-Object -FilePath (Join-Path $logDirectory "$name.log")
$processExit=$LASTEXITCODE; $path=Join-Path $directory 'result.json'
if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTimeUtc -lt $started) { throw "Fresh video report missing: $path" }
$result=Get-Content -LiteralPath $path -Raw|ConvertFrom-Json
foreach($field in @('passed','fadeBlack','levelLoaded','mediaStopped','gameplayInput')) { if(-not $result.$field){throw "Video check failed: $field. $path"} }
if($processExit -ne 0){throw "Video process failed: $processExit. $path"}
if($Scenario -eq 'Failures') { if(-not $result.missingAndFailedMedia){throw 'Missing/decoder failure handling failed.'} }
else {
    if(-not $result.hubPlayback -or -not $result.hubLoops -or -not $result.optionsContinuous){throw 'Continuous HUB playback/options failed.'}
    if($Scenario -eq 'Auto' -and (-not $result.studioPlayback -or -not $result.gamePlayback)){throw 'Automatic video order failed.'}
    if($Scenario -eq 'SkipStudio' -and (-not $result.studioPlayback -or $result.gamePlayback -or -not $result.skipConsumed)){throw 'Studio skip failed.'}
    if($Scenario -eq 'SkipGame' -and (-not $result.studioPlayback -or -not $result.gamePlayback -or -not $result.skipConsumed)){throw 'Game-intro skip failed.'}
}
Write-Output "Startup videos passed ($name), native pointer input and real level travel. Report: $path"
exit 0
