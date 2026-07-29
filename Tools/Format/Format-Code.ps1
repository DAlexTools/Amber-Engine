[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [switch]$Check,
    [switch]$List,
    [switch]$IncludeThirdParty,

    [string]$ClangFormatPath = "",

    [string[]]$Roots = @(
        "Engine",
        "Samples",
        "Projects",
        "Tests"
    ),

    [string[]]$Extensions = @(
        ".c",
        ".cc",
        ".cpp",
        ".cxx",
        ".h",
        ".hh",
        ".hpp",
        ".hxx",
        ".inl",
        ".ipp"
    )
)

$ErrorActionPreference = "Stop"

function Get-RepoRoot {
    return (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).ProviderPath
}

function Get-RelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $normalizedBase = [System.IO.Path]::GetFullPath($BasePath).TrimEnd('\', '/')
    $normalizedPath = [System.IO.Path]::GetFullPath($Path)
    if ($normalizedPath.StartsWith($normalizedBase, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $normalizedPath.Substring($normalizedBase.Length).TrimStart('\', '/')
    }

    return $normalizedPath
}

function Resolve-ClangFormat {
    param(
        [string]$RequestedPath
    )

    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        if (Test-Path -LiteralPath $RequestedPath) {
            return (Resolve-Path -LiteralPath $RequestedPath).ProviderPath
        }

        return $RequestedPath
    }

    $command = Get-Command "clang-format" -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        $command = Get-Command "clang-format.exe" -ErrorAction SilentlyContinue
    }

    if ($null -ne $command) {
        return $command.Source
    }

    $standardInstallPaths = @(
        "C:\Program Files\LLVM\bin\clang-format.exe",
        "C:\Program Files (x86)\LLVM\bin\clang-format.exe"
    )
    foreach ($path in $standardInstallPaths) {
        if (Test-Path -LiteralPath $path) {
            return $path
        }
    }

    throw "clang-format was not found in PATH. Install LLVM/clang-format or pass -ClangFormatPath <path>."
}

function New-StringSet {
    param(
        [string[]]$Values
    )

    $set = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($value in $Values) {
        [void]$set.Add($value)
    }
    return $set
}

function Test-IsExcludedPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot,

        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.HashSet[string]]$ExcludedNames
    )

    $relative = Get-RelativePath $RepoRoot $Path
    $parts = $relative -split '[\\/]'
    foreach ($part in $parts) {
        if ($ExcludedNames.Contains($part)) {
            return $true
        }
    }

    return $false
}

$repoRoot = Get-RepoRoot
Set-Location $repoRoot

$extensionSet = New-StringSet $Extensions
$excludedNames = @(
    ".git",
    ".vs",
    "Builds",
    "build",
    "out",
    "bin",
    "obj",
    "Debug",
    "Release",
    "RelWithDebInfo",
    "MinSizeRel",
    "x64",
    "Win32",
    "Dependencies",
    "vcpkg_installed",
    "Content"
)

if (-not $IncludeThirdParty) {
    $excludedNames += "ThirdParty"
}

$excludedSet = New-StringSet $excludedNames
$files = New-Object System.Collections.Generic.List[System.IO.FileInfo]

foreach ($rootName in $Roots) {
    $rootPath = if ([System.IO.Path]::IsPathRooted($rootName)) { $rootName } else { Join-Path $repoRoot $rootName }
    if (-not (Test-Path -LiteralPath $rootPath)) {
        Write-Verbose "Skipping missing root: $rootName"
        continue
    }

    Get-ChildItem -LiteralPath $rootPath -Recurse -File |
        Where-Object {
            $extensionSet.Contains($_.Extension) -and
            -not (Test-IsExcludedPath $repoRoot $_.FullName $excludedSet)
        } |
        ForEach-Object {
            $files.Add($_)
        }
}

$files = $files | Sort-Object FullName -Unique

if ($files.Count -eq 0) {
    Write-Host "No C/C++ files found."
    exit 0
}

if ($List) {
    foreach ($file in $files) {
        Write-Host (Get-RelativePath $repoRoot $file.FullName)
    }
    Write-Host ""
    Write-Host "Found $($files.Count) file(s)."
    exit 0
}

$clangFormat = Resolve-ClangFormat $ClangFormatPath
$formatArgs = @("--style=file")
if ($Check) {
    $formatArgs += @("--dry-run", "--Werror")
}
else {
    $formatArgs += "-i"
}

Write-Host "clang-format: $clangFormat"
Write-Host "Mode: $(if ($Check) { 'check' } else { 'format' })"
Write-Host "Files: $($files.Count)"

$failed = 0
foreach ($file in $files) {
    $relative = Get-RelativePath $repoRoot $file.FullName
    $operation = if ($Check) { "Check formatting" } else { "Format" }
    if (-not $PSCmdlet.ShouldProcess($relative, $operation)) {
        continue
    }

    & $clangFormat @formatArgs $file.FullName
    if ($LASTEXITCODE -ne 0) {
        ++$failed
        Write-Host "clang-format failed: $relative" -ForegroundColor Red
    }
}

if ($failed -gt 0) {
    throw "clang-format failed for $failed file(s)."
}

Write-Host ""
if ($Check) {
    Write-Host "Formatting check passed." -ForegroundColor Green
}
else {
    Write-Host "Formatting completed." -ForegroundColor Green
}
