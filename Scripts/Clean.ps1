[CmdletBinding()]
param(
    [switch]$RemoveVcpkg
)

$ErrorActionPreference = "Stop"

$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$Root = $Root.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
$RootPrefix = $Root + [System.IO.Path]::DirectorySeparatorChar

function Resolve-CleanTarget {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $Root $RelativePath))
    if ($fullPath -eq $Root -or -not $fullPath.StartsWith($RootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean outside repository root: $RelativePath"
    }

    return $fullPath
}

$targets = [System.Collections.Generic.List[string]]::new()

@(
    ".vs",
    "build",
    "build-cmake",
    "build-cmake-vcpkg",
    "build-cmake-vcpkg-no-editor",
    "out",
    "bin",
    "obj",
    "x64",
    "Win32",
    "Debug",
    "Release",
    "RelWithDebInfo",
    "MinSizeRel",
    "vcpkg_installed",
    "CMakeUserPresets.json",
    "log.txt",
    "imgui.ini",
    "entry_trace.txt"
) | ForEach-Object { $targets.Add($_) }

Get-ChildItem -LiteralPath $Root -Force -Directory -ErrorAction Stop |
    Where-Object { $_.Name -like "build-*" } |
    ForEach-Object { $targets.Add($_.Name) }

if ($RemoveVcpkg) {
    $targets.Add("external\vcpkg")
}

$uniqueTargets = $targets | Sort-Object -Unique
$foundCount = 0
$removed = [System.Collections.Generic.List[string]]::new()
$failed = [System.Collections.Generic.List[object]]::new()

foreach ($target in $uniqueTargets) {
    $fullPath = Resolve-CleanTarget -RelativePath $target
    if (-not (Test-Path -LiteralPath $fullPath)) {
        continue
    }

    $foundCount++
    try {
        Remove-Item -LiteralPath $fullPath -Recurse -Force -ErrorAction Stop
        $removed.Add($target)
    } catch {
        $failed.Add([pscustomobject]@{
            Path = $target
            Error = $_.Exception.Message
        })
    }
}

if ($removed.Count -gt 0) {
    Write-Host "Removed generated/local files:"
    $removed | ForEach-Object { Write-Host "  $_" }
} elseif ($foundCount -eq 0) {
    Write-Host "No generated/local files found."
} else {
    Write-Host "No generated/local files were removed."
}

if ($failed.Count -gt 0) {
    Write-Warning "Some files could not be removed. Close Visual Studio/CMake processes that use this repository and run Clean.bat again."
    $failed | Format-Table -AutoSize
    exit 1
}

exit 0
