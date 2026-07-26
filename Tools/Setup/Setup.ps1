[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$RemoveVcpkg,
    [switch]$DepsOnly,
    [switch]$RegisterProjectFiles,

    [ValidateSet("Editor", "NoEditor", "Core")]
    [string]$Mode = "Editor",

    [ValidateSet("Samples", "Editor", "Game", "Tests", "PhysicsLab", "Platformer", "Platformer2", "ContainerSandbox", "Core", "All")]
    [string]$Target = "Samples",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Debug",

    [switch]$RunTests,
    [switch]$RunSmoke,
    [switch]$NoConfigure,
    [switch]$ConfigureOnly,
    [switch]$NoAutoSetupDeps
)

$ErrorActionPreference = "Stop"

function Invoke-ToolScript {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ScriptPath,

        [AllowEmptyCollection()]
        [string[]]$Arguments = @()
    )

    & powershell -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Arguments
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function Set-DefaultRegistryValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SubKey,

        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $key = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey($SubKey)
    if ($null -eq $key) {
        throw "Failed to open registry key HKCU:\$SubKey."
    }

    try {
        $key.SetValue("", $Value, [Microsoft.Win32.RegistryValueKind]::String)
    }
    finally {
        $key.Dispose()
    }
}

function Register-AmberProjectFiles {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$BuildConfiguration
    )

    $editorExe = Join-Path $Root "Builds\Editor\Engine\Editor\Shell\$BuildConfiguration\AmberEditor.exe"
    if (-not (Test-Path $editorExe)) {
        Invoke-ToolScript (Join-Path $PSScriptRoot "Build.ps1") @(
            "-Mode", "Editor",
            "-Target", "Editor",
            "-Configuration", $BuildConfiguration
        )
    }

    if (-not (Test-Path $editorExe)) {
        throw "AmberEditor.exe was not found at '$editorExe'."
    }

    $extensionKey = "Software\Classes\.amberproject"
    $projectKey = "Software\Classes\AmberEngine.Project"
    $commandKey = "Software\Classes\AmberEngine.Project\shell\open\command"
    $iconKey = "Software\Classes\AmberEngine.Project\DefaultIcon"
    $openCommand = "`"$editorExe`" `"%1`""

    Set-DefaultRegistryValue $extensionKey "AmberEngine.Project"
    Set-DefaultRegistryValue $projectKey "Amber Project"
    Set-DefaultRegistryValue $iconKey "`"$editorExe`",0"
    Set-DefaultRegistryValue $commandKey $openCommand

    Write-Host ""
    Write-Host ".amberproject files are registered for this Windows user." -ForegroundColor Green
    Write-Host "Open command: $openCommand"
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $root

if ($Clean) {
    $cleanArgs = @()
    if ($RemoveVcpkg) {
        $cleanArgs += "-RemoveVcpkg"
    }

    Invoke-ToolScript (Join-Path $PSScriptRoot "Clean.ps1") $cleanArgs
    exit 0
}

if ($DepsOnly) {
    Invoke-ToolScript (Join-Path $PSScriptRoot "SetupDependencies.ps1")
    exit 0
}

if ($RegisterProjectFiles) {
    Register-AmberProjectFiles $root $Configuration
    exit 0
}

$buildArgs = @(
    "-Mode", $Mode,
    "-Target", $Target,
    "-Configuration", $Configuration
)

if ($RunTests) {
    $buildArgs += "-RunTests"
}
if ($RunSmoke) {
    $buildArgs += "-RunSmoke"
}
if ($NoConfigure) {
    $buildArgs += "-NoConfigure"
}
if ($ConfigureOnly) {
    $buildArgs += "-ConfigureOnly"
}
if ($NoAutoSetupDeps) {
    $buildArgs += "-NoAutoSetupDeps"
}

Invoke-ToolScript (Join-Path $PSScriptRoot "Build.ps1") $buildArgs
