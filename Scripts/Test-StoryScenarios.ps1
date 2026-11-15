param(
    [string]$EngineRoot='C:\Program Files\Epic Games\UE_5.6',
    [ValidateSet('Police','Civilian','Failure')][string[]]$Scenarios=@('Police','Civilian','Failure'),
    [string]$PackageDirectory,
    [string]$ReportPrefix='FinalUIEvents',
    [switch]$LanguageCheck
)
$ErrorActionPreference='Stop'
$projectRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$program=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if($PackageDirectory)
{
    $program=Join-Path $PackageDirectory 'Windows\MyProject\Binaries\Win64\MyProject.exe'
    if(-not(Test-Path -LiteralPath $program)){throw 'This native harness requires a Development package. Shipping deliberately excludes it.'}
}
foreach($scenario in $Scenarios)
{
    $directory=Join-Path $projectRoot "Saved\Automation\$ReportPrefix$scenario"
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
    $entry=if($PackageDirectory){'/Game/OneManStudio/Maps/Lvl_Bank?game=/Script/MyProject.StudioGameMode'}else{Join-Path $projectRoot 'MyProject.uproject'}
    $arguments=@($entry,'-game','-StoryFrontEndCheck',
        '-FinalTakeSceneSmoke','-StoryUICosmeticsCheck',"-StoryScenario=$scenario","-StoryReportDir=$directory",
        '-NoSplash','-NoP4','-NoLiveCoding','-Unattended','-RenderOffscreen','-Windowed',
        '-ResX=1280','-ResY=720','-ForceRes','-stdout','-FullStdOutLogOutput')
    if($LanguageCheck){$arguments+=@('-StoryLanguageCheck','-language=es')}
    $started=[DateTime]::UtcNow
    & $program @arguments | Tee-Object -FilePath (Join-Path $projectRoot "Saved\Logs\$ReportPrefix$scenario.log")
    $code=$LASTEXITCODE
    $report=Join-Path $directory 'result.json'
    if(-not(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $started){throw "Fresh report missing: $report"}
    $result=Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if($code -ne 0 -or -not $result.passed -or $result.turns -ne 6 -or $result.wardrobes -ne 3 -or $result.moviePlaybacks -ne 2){throw "Story regression failed: $report"}
    Write-Output "Story scenario passed: $scenario. $report"
}
exit 0
