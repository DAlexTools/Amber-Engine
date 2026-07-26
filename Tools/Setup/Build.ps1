[CmdletBinding()]
param(
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

function Invoke-CommandChecked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Title,

        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [AllowEmptyCollection()]
        [string[]]$Arguments = @()
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
    param(
        [string]$BuildMode,
        [bool]$UseSystemVcpkg
    )

    switch ($BuildMode) {
        "Core" { return "core" }
        "NoEditor" { return $(if ($UseSystemVcpkg) { "full-vcpkg-no-editor" } else { "full-local-vcpkg-no-editor" }) }
        default { return $(if ($UseSystemVcpkg) { "full-vcpkg" } else { "full-local-vcpkg" }) }
    }
}

function Resolve-VcpkgToolchain {
    param(
        [string]$Root,
        [bool]$AllowAutoSetup
    )

    $localToolchain = Join-Path $Root "Dependencies\vcpkg\scripts\buildsystems\vcpkg.cmake"
    $localVcpkgExe = Join-Path $Root "Dependencies\vcpkg\vcpkg.exe"
    if ((Test-Path $localToolchain) -and (Test-Path $localVcpkgExe)) {
        return [pscustomobject]@{
            UseSystemVcpkg = $false
            Toolchain = $localToolchain
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
        $systemToolchain = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
        $systemVcpkgExe = Join-Path $env:VCPKG_ROOT "vcpkg.exe"
        if ((Test-Path $systemToolchain) -and (Test-Path $systemVcpkgExe)) {
            return [pscustomobject]@{
                UseSystemVcpkg = $true
                Toolchain = $systemToolchain
            }
        }
    }

    if ($AllowAutoSetup) {
        $setupScript = Join-Path $PSScriptRoot "SetupDependencies.ps1"
        Invoke-CommandChecked "Setup dependencies" "powershell" @(
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            $setupScript
        )

        if (Test-Path $localToolchain) {
            return [pscustomobject]@{
                UseSystemVcpkg = $false
                Toolchain = $localToolchain
            }
        }
    }

    throw "vcpkg was not found. Run .\Setup.bat, set VCPKG_ROOT to an existing vcpkg checkout, or build with .\Setup.bat -Mode Core -Target Core."
}

function Get-BuildDirectory {
    param(
        [string]$BuildMode,
        [string]$Root
    )

    switch ($BuildMode) {
        "Core" { return Join-Path $Root "Builds\Core" }
        "NoEditor" { return Join-Path $Root "Builds\NoEditor" }
        default { return Join-Path $Root "Builds\Editor" }
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
        "Platformer2App",
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
    $editorTargets = if ($BuildMode -eq "Editor") { @("AmberEditor") } else { @() }

    if ($BuildMode -eq "Core") {
        return $coreTargets
    }

    switch ($BuildTarget) {
        "Game" { return @("GameEngineApp") }
        "Editor" { return $editorTargets }
        "Tests" { return $testTargets }
        "PhysicsLab" { return @("PhysicsLabApp") }
        "Platformer" { return @("PlatformerApp") }
        "Platformer2" { return @("Platformer2App") }
        "ContainerSandbox" { return @("ContainerSandboxApp") }
        "Core" { return $coreTargets }
        "All" { return $sampleTargets + $editorTargets + $testTargets }
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
    $platformerBin = Join-Path $BuildDirectory "Projects\Platformer\$Configuration"
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
            Invoke-CommandChecked "Platformer smoke" (Join-Path $platformerBin "PlatformerApp.exe") @("--smoke-test")
        }

        if ($BuildTarget -in @("Samples", "All", "Platformer2")) {
            Invoke-CommandChecked "Platformer2 smoke" (Join-Path $sampleBin "Platformer2App.exe") @("--smoke-test")
        }

        if ($BuildTarget -in @("Samples", "All", "ContainerSandbox")) {
            Invoke-CommandChecked "ContainerSandbox smoke" (Join-Path $sampleBin "ContainerSandboxApp.exe") @("--smoke-test")
        }

        if ($BuildTarget -in @("Samples", "All", "Game")) {
            Invoke-CommandChecked "GameEngine level 1 smoke" (Join-Path $sampleBin "GameEngineApp.exe") @("--smoke-test", "--level", "1")
        }

        if ($BuildMode -eq "Editor" -and $BuildTarget -in @("Editor", "All")) {
            $editorSmoke = Join-Path $BuildDirectory "Engine\Editor\Shell\$Configuration\AmberEditor.exe"
            $platformerProject = Join-Path (Get-Location) "Projects\Platformer\Platformer.amberproject"
            Invoke-CommandChecked "AmberEditor smoke" $editorSmoke @("--smoke-test", "--project", $platformerProject)
        }
    }
    finally {
        $env:SDL_VIDEODRIVER = $oldVideo
        $env:SDL_AUDIODRIVER = $oldAudio
    }
}

try {
    $root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
    Set-Location $root

    $vcpkg = $null
    if ($Mode -ne "Core") {
        $vcpkg = Resolve-VcpkgToolchain $root (-not $NoAutoSetupDeps)
    }

    $configurePreset = Get-ConfigurePreset $Mode ($null -ne $vcpkg -and $vcpkg.UseSystemVcpkg)
    $buildDirectory = Get-BuildDirectory $Mode $root
    $targets = Get-Targets $Mode $Target
    if (-not $targets -or $targets.Count -eq 0) {
        throw "Target '$Target' is not available in mode '$Mode'."
    }

    Write-Host "AmberEngine build" -ForegroundColor Green
    Write-Host "Root: $root"
    Write-Host "Mode: $Mode"
    Write-Host "Target: $Target"
    Write-Host "Configuration: $Configuration"
    if ($null -ne $vcpkg) {
        Write-Host "vcpkg: $($vcpkg.Toolchain)"
    }

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
