param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.6')
$ErrorActionPreference='Stop'
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$python=Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\Python3\Win64\python.exe'
$env:PYTHONIOENCODING='utf-8'
& $editor (Join-Path $project 'MyProject.uproject') -run=GatherText "-Config=$project\Config\Localization\FinalTake_Gather.ini" -Unattended -NoP4 -NullRHI -NoSound -NoSplash -stdout -FullStdOutLogOutput "-AbsLog=$project\Saved\Logs\LocalizationGather.log"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python (Join-Path $PSScriptRoot 'Fill-Localization.py')
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $editor (Join-Path $project 'MyProject.uproject') -run=GatherText "-Config=$project\Config\Localization\FinalTake_Compile.ini" -Unattended -NoP4 -NullRHI -NoSound -NoSplash -stdout -FullStdOutLogOutput "-AbsLog=$project\Saved\Logs\LocalizationCompile.log"
exit $LASTEXITCODE
