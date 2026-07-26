[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$RemoveVcpkg,
    [switch]$DepsOnly,

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
