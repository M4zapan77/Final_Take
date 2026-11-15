param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [string]$ArchiveDirectory,
    [string[]]$Maps = @('/Game/OneManStudio/Maps/Lvl_Bank'),
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
    "-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=$unusedThreads -CookDir=/Game/OneManStudio/UI/StartupVideos -CookDir=/Game/OneManStudio/Characters/Base3/Customization -CookDir=/Game/OneManStudio/Cosmetics -CookDir=/Game/OneManStudio/Environment -CookDir=/Game/OneManStudio/Audio"
$arguments = @('-NoCompileUAT',"-ScriptsForProject=$projectRoot\MyProject.uproject",'BuildCookRun',
    "-project=$projectRoot\MyProject.uproject",'-nop4','-unattended','-utf8output','-target=MyProject','-platform=Win64',
    '-clientconfig=Development','-build','-cook','-stage','-pak','-iostore','-archive',"-archivedirectory=$ArchiveDirectory",
    '-nocompileeditor','-skipbuildeditor','-nodebuginfo','-prereqs','-ubtargs=-NoUBA',
    "-AdditionalCookerOptions=$cookOptions",('-map=' + ($Maps -join '+')))
& $uat @arguments | Tee-Object -FilePath (Join-Path $logDirectory 'Package-Windows.log')
$packageExit = $LASTEXITCODE
if ($packageExit -ne 0) { exit $packageExit }
$runtime = Join-Path $ArchiveDirectory 'Windows\MyProject\Binaries\Win64\MyProject.exe'
if (-not (Test-Path -LiteralPath $runtime)) { throw 'UAT did not produce the expected packaged executable.' }
$launcher = @'
@echo off
cd /d "%~dp0"
"MyProject\Binaries\Win64\MyProject.exe" "@ENTRY_MAP@?game=/Script/MyProject.StudioGameMode"
'@
$launcher = $launcher.Replace('@ENTRY_MAP@',$Maps[0])
Set-Content -LiteralPath (Join-Path $ArchiveDirectory 'Windows\Play-ONE-MAN-STUDIO.bat') -Value $launcher -Encoding ascii
Write-Output "Packaged Windows Development game: $runtime"
Write-Output "Native entry point: $($Maps[0])?game=/Script/MyProject.StudioGameMode"
exit 0
