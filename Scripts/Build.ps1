param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.6',
    [ValidateSet('Development', 'DebugGame')]
    [string]$Configuration = 'Development',
    [ValidateSet('Editor', 'Game')]
    [string]$Target = 'Editor',
    [switch]$GenerateProjectFiles
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$projectFile = Join-Path $projectRoot 'MyProject.uproject'
$buildTool = Join-Path $EngineRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
$dotnetRoot = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet'
$dotnet = Get-ChildItem -LiteralPath $dotnetRoot -Directory |
    Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName 'win-x64\dotnet.exe' } |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1
if (-not $dotnet -or -not (Test-Path -LiteralPath $buildTool)) {
    throw 'UnrealBuildTool or the bundled .NET SDK was not found. Pass -EngineRoot with the UE 5.6 installation path.'
}

$logDirectory = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
$buildTempDirectory = Join-Path $projectRoot 'Saved\BuildTemp'
New-Item -ItemType Directory -Force -Path $buildTempDirectory | Out-Null
# Keep compiler/linker temporary files accessible across desktop sandbox sessions.
$env:TEMP = $buildTempDirectory
$env:TMP = $buildTempDirectory
$env:DOTNET_CLI_HOME = Join-Path $projectRoot 'Saved\DotNetHome'
$env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = '1'
$env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
if ($GenerateProjectFiles) {
    $buildArguments = @('-ProjectFiles', "-Project=$projectFile", '-Game', '-Engine', "-Log=$logDirectory\GenerateProjectFiles.log")
} else {
    $targetName = if ($Target -eq 'Editor') { 'MyProjectEditor' } else { 'MyProject' }
    $buildArguments = @($targetName, 'Win64', $Configuration, "-Project=$projectFile",
        '-WaitMutex', '-NoHotReloadFromIDE', '-NoUBA', '-NoUBTMakefiles', "-Log=$logDirectory\Build-$targetName.log")
}

# Use the engine's bundled tools directly; do not rebuild or modify the engine installation.
Push-Location -LiteralPath (Join-Path $EngineRoot 'Engine\Source')
try {
    & $dotnet $buildTool @buildArguments
    $buildExitCode = $LASTEXITCODE
} finally {
    Pop-Location
}
exit $buildExitCode
