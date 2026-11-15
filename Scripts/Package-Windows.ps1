param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [string]$ArchiveDirectory,
    [string[]]$Maps = @('/Game/OneManStudio/Maps/Lvl_Bank','/Game/OneManStudio/Maps/Lvl_Cinema'),
    [ValidateSet('Development','Shipping')][string]$Configuration='Shipping',
    [ValidateRange(1,8)][int]$ShaderWorkers = 2
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if ($Maps.Count -lt 1) { throw 'Specify at least one map to cook.' }
foreach ($map in $Maps) { if ($map -notmatch '^/Game/[A-Za-z0-9_/]+$') { throw "Use a map package path without an extension: $map" } }
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $projectRoot 'Saved\Packages\FinalCorrections' }
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path -LiteralPath $uat)) { throw 'RunUAT.bat not found.' }
if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) { throw 'Save and close Unreal Editor before packaging.' }
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
$tempDirectory = Join-Path $projectRoot 'Saved\BuildTemp'
New-Item -ItemType Directory -Force -Path $logDirectory,$tempDirectory,$ArchiveDirectory | Out-Null
$env:TEMP = $tempDirectory; $env:TMP = $tempDirectory
$env:DOTNET_CLI_HOME = Join-Path $projectRoot 'Saved\DotNetHome'
$env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = '1'; $env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
$unusedThreads = [Math]::Max(0,[Environment]::ProcessorCount - $ShaderWorkers)
$cookOptions = '-ini:Engine:[DevOptions.Shaders]:ShaderCompilerCoreCountThreshold=128 ' +
    "-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=$unusedThreads"
# The dynamic UI theme is listed in DefaultGame.ini's DirectoriesToAlwaysCook;
# existing gameplay catalogs remain cooked through their asset references.
# Virtual /Game paths passed as repeated CookDir
# command-line directories were silently omitted by the installed cooker.
$arguments = @('-NoCompileUAT',"-ScriptsForProject=$projectRoot\MyProject.uproject",'BuildCookRun',
    "-project=$projectRoot\MyProject.uproject",'-nop4','-unattended','-utf8output','-target=MyProject','-platform=Win64',
    "-clientconfig=$Configuration",'-build','-cook','-stage','-pak','-iostore','-compressed','-archive',"-archivedirectory=$ArchiveDirectory",
    '-nocompileeditor','-skipbuildeditor','-nodebuginfo','-prereqs','-ubtargs=-NoUBA',
    "-AdditionalCookerOptions=$cookOptions",('-map=' + ($Maps -join '+')))
& $uat @arguments | Tee-Object -FilePath (Join-Path $logDirectory 'Package-Windows.log')
$packageExit = $LASTEXITCODE
if ($packageExit -ne 0) { exit $packageExit }
$runtimeName=if($Configuration -eq 'Shipping'){'MyProject-Win64-Shipping.exe'}else{'MyProject.exe'}
$runtime = Join-Path $ArchiveDirectory "Windows\MyProject\Binaries\Win64\$runtimeName"
if (-not (Test-Path -LiteralPath $runtime)) { throw 'UAT did not produce the expected packaged executable.' }
$manifestPath=Join-Path $ArchiveDirectory 'Windows\Manifest_UFSFiles_Win64.txt'
if(-not(Test-Path -LiteralPath $manifestPath)){throw 'UFS staging manifest is missing.'}
$manifest=[IO.File]::ReadAllText($manifestPath).Replace('\','/')
$contentRoot=Join-Path $projectRoot 'Content'
$uiPackages=@(Get-ChildItem -LiteralPath (Join-Path $contentRoot 'OneManStudio\UI') -Recurse -File -Filter '*.uasset')
$missing=@()
foreach($asset in $uiPackages)
{
    $relative='MyProject/Content/'+$asset.FullName.Substring($contentRoot.Length+1).Replace('\','/')
    if(-not $manifest.Contains($relative)){$missing+=$relative}
}
if($missing.Count){throw ('UI assets omitted from cooked package: '+($missing -join ', '))}
@{configuration=$Configuration;uiPackages=$uiPackages.Count;missing=$missing;manifest='Windows/Manifest_UFSFiles_Win64.txt'} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ArchiveDirectory 'CookedUIValidation.json') -Encoding utf8
Write-Output "Verified $($uiPackages.Count) UI packages in the cooked staging manifest."
$launcher = @'
@echo off
cd /d "%~dp0"
"MyProject\Binaries\Win64\@RUNTIME@" "@ENTRY_MAP@?game=/Script/MyProject.StudioGameMode"
'@
$launcher = $launcher.Replace('@ENTRY_MAP@',$Maps[0]).Replace('@RUNTIME@',$runtimeName)
Set-Content -LiteralPath (Join-Path $ArchiveDirectory 'Windows\Play-FINAL-TAKE.bat') -Value $launcher -Encoding ascii
$fontLicense=Join-Path $projectRoot 'SourceArt\FinalUI20261115\Font\OFL.txt'
if(Test-Path -LiteralPath $fontLicense)
{
    $licenseDirectory=Join-Path $ArchiveDirectory 'Windows\Licenses';New-Item -ItemType Directory -Force -Path $licenseDirectory | Out-Null
    Copy-Item -LiteralPath $fontLicense -Destination (Join-Path $licenseDirectory 'Bungee-OFL.txt')
}
Write-Output "Packaged Windows $Configuration game: $runtime"
Write-Output "Native entry point: $($Maps[0])?game=/Script/MyProject.StudioGameMode"
exit 0
