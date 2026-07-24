[CmdletBinding()]
param()

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

try {
    $root = Resolve-Path (Join-Path $PSScriptRoot "..")
    Set-Location $root

    $externalDir = Join-Path $root "external"
    $vcpkgDir = Join-Path $externalDir "vcpkg"
    $toolchain = Join-Path $vcpkgDir "scripts\buildsystems\vcpkg.cmake"
    $vcpkgExe = Join-Path $vcpkgDir "vcpkg.exe"
    $bootstrap = Join-Path $vcpkgDir "bootstrap-vcpkg.bat"

    if ((Test-Path $toolchain) -and (Test-Path $vcpkgExe)) {
        Write-Host "Repo-local vcpkg is ready: $vcpkgDir" -ForegroundColor Green
        exit 0
    }

    if (-not (Test-Path $externalDir)) {
        New-Item -ItemType Directory -Path $externalDir | Out-Null
    }

    if (-not (Test-Path $vcpkgDir)) {
        Invoke-CommandChecked "Clone vcpkg" "git" @(
            "clone",
            "https://github.com/microsoft/vcpkg.git",
            $vcpkgDir
        )
    }

    if (-not (Test-Path $bootstrap)) {
        throw "vcpkg exists but bootstrap script was not found at '$bootstrap'. Delete external\vcpkg and run SetupDependencies.bat again."
    }

    Invoke-CommandChecked "Bootstrap vcpkg" $bootstrap

    if (-not ((Test-Path $toolchain) -and (Test-Path $vcpkgExe))) {
        throw "vcpkg bootstrap finished, but required files were not found at '$toolchain' and '$vcpkgExe'."
    }

    Write-Host ""
    Write-Host "Dependencies are ready. CMake will install manifest packages from vcpkg.json during configure." -ForegroundColor Green
    exit 0
}
catch {
    Write-Host ""
    Write-Host "Dependency setup failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
