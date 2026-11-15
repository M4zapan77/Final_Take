param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [switch]$RenderScreenshots
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editor)) { throw 'UnrealEditor-Cmd.exe not found.' }
$startedAt = [DateTime]::UtcNow
$arguments = @((Join-Path $projectRoot 'MyProject.uproject'), '-game', '-StudioSkipMenu', '-WardrobeSmoke', '-Unattended',
    '-NoSound', '-NoSplash', '-NoP4', '-NoLiveCoding', '-RenderOffscreen', '-Windowed', '-ResX=1280', '-ResY=720', '-ForceRes',
    '-ExecCmds=sg.ShadowQuality 1,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,Movie.WardrobeSmoke', '-StdOut', '-FullStdOutLogOutput')
if ($RenderScreenshots) { $arguments += '-WardrobeSmokeScreenshots' } else { $arguments += '-NullRHI' }
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
& $editor @arguments | Tee-Object -FilePath (Join-Path $logDirectory 'WardrobePlay.log')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$report = Join-Path $projectRoot 'Saved\Automation\WardrobePlay\result.json'
if (-not (Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $startedAt) { throw 'Fresh wardrobe Play report not produced.' }
$result = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if (-not $result.passed -or $result.wardrobeScreens -ne 3 -or $result.readyStarts -ne 2 -or $result.timeoutStarts -ne 1) { throw "Wardrobe Play test failed: $report" }
Write-Output "Wardrobe Play test passed. Report: $report"
exit 0
