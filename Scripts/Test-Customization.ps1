param(
    [string]$EngineRoot='C:\Program Files\Epic Games\UE_5.6',
    [string]$PackageDirectory,
    [ValidateSet('Survey','Motion','Full')][string]$Scenario='Survey',
    [int]$ResolutionX=1280,
    [int]$ResolutionY=800
)
$ErrorActionPreference='Stop'
if($Scenario -eq 'Full')
{
    $parameters=@{EngineRoot=$EngineRoot;EnableAudio=$true;ResolutionX=$ResolutionX;ResolutionY=$ResolutionY;ReportSuffix='UnifiedCustomization'}
    if($PackageDirectory){$parameters.PackageDirectory=$PackageDirectory}
    & (Join-Path $PSScriptRoot 'Test-StudioFlow.ps1') @parameters
    exit $LASTEXITCODE
}
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if($PackageDirectory){$program=Join-Path $PackageDirectory 'Windows\MyProject\Binaries\Win64\MyProject.exe';$argsList=@();$prefix='CustomizationPackaged'}
else{$program=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$argsList=@((Join-Path $project 'MyProject.uproject'));$prefix='CustomizationPlay'}
$name=$prefix+$Scenario;$directory=Join-Path $project "Saved\Automation\$name";$logs=Join-Path $project 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $directory,$logs|Out-Null
$argsList+=@('-game','-StudioSkipMenu',"-Customization$Scenario","-CustomizationReportDir=$directory",'-Unattended','-NoSplash','-NoP4','-NoLiveCoding','-RenderOffscreen','-Windowed',"-ResX=$ResolutionX","-ResY=$ResolutionY",'-ForceRes','-StdOut','-FullStdOutLogOutput',"-AbsLog=$logs\$name.engine.log")
$argsList+='-NoSound'
$started=[DateTime]::UtcNow
& $program @argsList|Tee-Object -FilePath (Join-Path $logs "$name.log")
$processExit=$LASTEXITCODE;$path=Join-Path $directory 'result.json'
if(-not(Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTimeUtc -lt $started){throw "Fresh customization report missing: $path"}
$result=Get-Content -LiteralPath $path -Raw|ConvertFrom-Json
if($processExit -ne 0 -or -not$result.passed){throw "Customization failed: $path"}
Write-Output "Customization passed ($name). Report: $path"
exit 0
