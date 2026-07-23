[CmdletBinding()]
param(
    [ValidateSet("Editor", "NoEditor", "Core")]
    [string]$Mode = "Editor",

    [ValidateSet("Samples", "Game", "Tests", "PhysicsLab", "Platformer", "ContainerSandbox", "Core", "All")]
    [string]$Target = "Samples",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Debug",

    [switch]$RunTests,
    [switch]$RunSmoke,
    [switch]$NoConfigure,
    [switch]$ConfigureOnly
)

$ErrorActionPreference = "Stop"

function Invoke-CommandChecked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Title,

        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    Write-Host ""
    Write-Host "==> $Title" -ForegroundColor Cyan
    Write-Host "$FilePath $($Arguments -join ' ')" -ForegroundColor DarkGray

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Title failed with exit code $LASTEXITCODE."
    }
}

function Get-ConfigurePreset {
    param([string]$BuildMode)

    switch ($BuildMode) {
        "Core" { return "core" }
        "NoEditor" { return "full-local-vcpkg-no-editor" }
        default { return "full-local-vcpkg" }
    }
}

function Get-BuildDirectory {
    param(
        [string]$BuildMode,
        [string]$Root
    )

    switch ($BuildMode) {
        "Core" { return Join-Path $Root "build-cmake" }
        "NoEditor" { return Join-Path $Root "build-cmake-vcpkg-no-editor" }
        default { return Join-Path $Root "build-cmake-vcpkg" }
    }
}

function Get-Targets {
    param(
        [string]$BuildMode,
        [string]$BuildTarget
    )

    $sampleTargets = @(
        "GameEngineApp",
        "PlatformerApp",
        "ContainerSandboxApp",
        "PhysicsLabApp",
        "AngryApp",
        "RagdollApp",
        "ChainApp",
        "SoftBodyApp",
        "FabricSimulationApp"
    )
    $testTargets = @("PhysicsUnitTests", "EngineUnitTests")
    $coreTargets = @("Physics", "EnginePhysicsBridgeCheck", "PhysicsCollisionFilteringCheck")

    if ($BuildMode -eq "Core") {
        return $coreTargets
    }

    switch ($BuildTarget) {
        "Game" { return @("GameEngineApp") }
        "Tests" { return $testTargets }
        "PhysicsLab" { return @("PhysicsLabApp") }
        "Platformer" { return @("PlatformerApp") }
        "ContainerSandbox" { return @("ContainerSandboxApp") }
        "Core" { return $coreTargets }
        "All" { return $sampleTargets + $testTargets }
        default { return $sampleTargets }
    }
}

function Invoke-SmokeChecks {
    param(
        [string]$BuildMode,
        [string]$BuildTarget,
        [string]$BuildDirectory,
        [string]$Configuration
    )

    $sampleBin = Join-Path $BuildDirectory "Samples\$Configuration"
    $physicsSmoke = Join-Path $BuildDirectory "Engine\Runtime\Physics\$Configuration\PhysicsCollisionFilteringCheck.exe"

    $oldVideo = $env:SDL_VIDEODRIVER
    $oldAudio = $env:SDL_AUDIODRIVER
    $env:SDL_VIDEODRIVER = "dummy"
    $env:SDL_AUDIODRIVER = "dummy"

    try {
        if ($BuildMode -eq "Core" -or $BuildTarget -eq "Core" -or $BuildTarget -eq "All") {
            if (Test-Path $physicsSmoke) {
                Invoke-CommandChecked "Physics collision filtering smoke" $physicsSmoke @()
            }
        }

        if ($BuildMode -eq "Core") {
            return
        }

        if ($BuildTarget -in @("Samples", "All", "PhysicsLab")) {
            Invoke-CommandChecked "PhysicsLab smoke" (Join-Path $sampleBin "PhysicsLabApp.exe") @("--smoke-test")
            Invoke-CommandChecked "PhysicsLab UI smoke" (Join-Path $sampleBin "PhysicsLabApp.exe") @("--ui-smoke-test")
        }

        if ($BuildTarget -in @("Samples", "All", "Platformer")) {
            Invoke-CommandChecked "Platformer smoke" (Join-Path $sampleBin "PlatformerApp.exe") @("--smoke-test")
        }

        if ($BuildTarget -in @("Samples", "All", "ContainerSandbox")) {
            Invoke-CommandChecked "ContainerSandbox smoke" (Join-Path $sampleBin "ContainerSandboxApp.exe") @("--smoke-test")
        }

        if ($BuildTarget -in @("Samples", "All", "Game")) {
            Invoke-CommandChecked "GameEngine level 1 smoke" (Join-Path $sampleBin "GameEngineApp.exe") @("--smoke-test", "--level", "1")
        }
    }
    finally {
        $env:SDL_VIDEODRIVER = $oldVideo
        $env:SDL_AUDIODRIVER = $oldAudio
    }
}

try {
    $root = Resolve-Path (Join-Path $PSScriptRoot "..")
    Set-Location $root

    if ($Mode -ne "Core") {
        $localVcpkgToolchain = Join-Path $root "external\vcpkg\scripts\buildsystems\vcpkg.cmake"
        if (-not (Test-Path $localVcpkgToolchain)) {
            throw "Repo-local vcpkg was not found at '$localVcpkgToolchain'. Run external\vcpkg\bootstrap-vcpkg.bat or use the core mode."
        }
    }

    $configurePreset = Get-ConfigurePreset $Mode
    $buildDirectory = Get-BuildDirectory $Mode $root
    $targets = Get-Targets $Mode $Target

    Write-Host "AmberEngine build" -ForegroundColor Green
    Write-Host "Root: $root"
    Write-Host "Mode: $Mode"
    Write-Host "Target: $Target"
    Write-Host "Configuration: $Configuration"

    if (-not $NoConfigure) {
        Invoke-CommandChecked "Configure $configurePreset" "cmake" @("--preset", $configurePreset)
    }

    if ($ConfigureOnly) {
        Write-Host ""
        Write-Host "Configure completed." -ForegroundColor Green
        exit 0
    }

    $buildArgs = @("--build", $buildDirectory, "--config", $Configuration, "--target") + $targets
    Invoke-CommandChecked "Build $($targets -join ', ')" "cmake" $buildArgs

    if ($RunTests -or $Target -eq "Tests") {
        if ($Mode -eq "Core") {
            Write-Host ""
            Write-Host "Unit tests are not configured in Core mode. Use -Mode Editor or -Mode NoEditor." -ForegroundColor Yellow
        }
        else {
            $testPreset = if ($Mode -eq "NoEditor") { "unit-tests-local-no-editor" } else { "unit-tests-local" }
            Invoke-CommandChecked "Run unit tests" "ctest" @("--preset", $testPreset)
        }
    }

    if ($RunSmoke) {
        Invoke-SmokeChecks $Mode $Target $buildDirectory $Configuration
    }

    Write-Host ""
    Write-Host "Build completed successfully." -ForegroundColor Green
    exit 0
}
catch {
    Write-Host ""
    Write-Host "Build failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
