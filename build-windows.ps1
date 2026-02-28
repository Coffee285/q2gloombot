<#
.SYNOPSIS
    Idempotent Windows 11 build script for q2gloombot.

.DESCRIPTION
    Builds q2gloombot from source on a fresh Windows 11 machine.
    Detects installed tools, optionally installs missing prerequisites
    via winget or Chocolatey, then builds with CMake and runs the test suite.

.PARAMETER NonInteractive
    Suppress all prompts; auto-accept installs where possible.

.PARAMETER NoInstall
    Skip installing prerequisites; fail with guidance if tools are missing.

.PARAMETER SkipTests
    Skip running the test suite after a successful build.

.PARAMETER Clean
    Remove previous build artifacts before building.

.PARAMETER ArtifactZip
    Create a .zip of ./dist after a successful build (default: $true).

.PARAMETER Verbose
    Enable extra diagnostic logging.

.EXAMPLE
    .\build-windows.ps1
    .\build-windows.ps1 -NonInteractive -SkipTests
    .\build-windows.ps1 -Clean -ArtifactZip:$false
#>

[CmdletBinding()]
param(
    [switch]$NonInteractive,
    [switch]$NoInstall,
    [switch]$SkipTests,
    [switch]$Clean,
    [bool]$ArtifactZip = $true,
    [switch]$VerboseOutput
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Logging helpers
# ---------------------------------------------------------------------------
$LogFile = Join-Path $PSScriptRoot 'build-windows.log'

function Write-Log {
    param([string]$Message, [string]$Level = 'INFO')
    $ts  = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
    $line = "[$ts] [$Level] $Message"
    Add-Content -Path $LogFile -Value $line
    switch ($Level) {
        'ERROR' { Write-Host $line -ForegroundColor Red }
        'WARN'  { Write-Host $line -ForegroundColor Yellow }
        'DEBUG' { if ($VerboseOutput) { Write-Host $line -ForegroundColor Cyan } }
        default { Write-Host $line }
    }
}

function Fail {
    param([string]$Message)
    Write-Log $Message 'ERROR'
    exit 1
}

# ---------------------------------------------------------------------------
# Initialise log
# ---------------------------------------------------------------------------
'' | Set-Content -Path $LogFile   # truncate/create
Write-Log "=== q2gloombot Windows build started ==="
Write-Log "Script: $PSCommandPath"
Write-Log "Parameters: NonInteractive=$NonInteractive  NoInstall=$NoInstall  SkipTests=$SkipTests  Clean=$Clean  ArtifactZip=$ArtifactZip"

# ---------------------------------------------------------------------------
# Detect whether we are running as Administrator
# ---------------------------------------------------------------------------
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
Write-Log "Running as Administrator: $isAdmin"

# ---------------------------------------------------------------------------
# Load .env / .env.example if present
# ---------------------------------------------------------------------------
foreach ($envFile in @('.env', '.env.example')) {
    $envPath = Join-Path $PSScriptRoot $envFile
    if (Test-Path $envPath) {
        Write-Log "Loading environment variables from $envFile"
        Get-Content $envPath | Where-Object { $_ -match '^\s*[^#]\S+=\S' } | ForEach-Object {
            $parts = $_ -split '=', 2
            $varName  = $parts[0].Trim()
            $varValue = if ($parts.Count -gt 1) { $parts[1].Trim() } else { '' }
            if (-not [System.Environment]::GetEnvironmentVariable($varName)) {
                [System.Environment]::SetEnvironmentVariable($varName, $varValue, 'Process')
                Write-Log "  Set $varName" 'DEBUG'
            }
        }
        break
    }
}

# ---------------------------------------------------------------------------
# Project-type detection
# ---------------------------------------------------------------------------
$repoRoot = $PSScriptRoot
Write-Log "Repository root: $repoRoot"

$hasCMake   = Test-Path (Join-Path $repoRoot 'CMakeLists.txt')
$hasNode    = Test-Path (Join-Path $repoRoot 'package.json')
$hasPython  = (Test-Path (Join-Path $repoRoot 'requirements.txt')) -or
              (Test-Path (Join-Path $repoRoot 'pyproject.toml')) -or
              (Test-Path (Join-Path $repoRoot 'setup.py'))
$hasDotnet  = (Get-ChildItem -Path $repoRoot -Filter '*.csproj' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1) -ne $null
$hasGo      = Test-Path (Join-Path $repoRoot 'go.mod')
$hasDocker  = Test-Path (Join-Path $repoRoot 'Dockerfile')
$hasMake    = Test-Path (Join-Path $repoRoot 'Makefile')

Write-Log "Detected project types — CMake:$hasCMake  Node:$hasNode  Python:$hasPython  .NET:$hasDotnet  Go:$hasGo  Docker:$hasDocker  Make:$hasMake"

if (-not ($hasCMake -or $hasNode -or $hasPython -or $hasDotnet -or $hasGo)) {
    Fail "Could not detect a supported project type. Please add a recognised build descriptor (CMakeLists.txt, package.json, etc.)."
}

# ---------------------------------------------------------------------------
# Helper: run a command and stream output to log
# ---------------------------------------------------------------------------
function Invoke-BuildCommand {
    param(
        [string]   $Description,
        [string]   $Command,
        [string[]] $Arguments
    )
    Write-Log "Running: $Command $($Arguments -join ' ')"
    $proc = Start-Process -FilePath $Command -ArgumentList $Arguments `
                          -NoNewWindow -PassThru -Wait `
                          -RedirectStandardOutput "$env:TEMP\bw_stdout.txt" `
                          -RedirectStandardError  "$env:TEMP\bw_stderr.txt"
    $stdout = Get-Content "$env:TEMP\bw_stdout.txt" -ErrorAction SilentlyContinue
    $stderr = Get-Content "$env:TEMP\bw_stderr.txt" -ErrorAction SilentlyContinue
    if ($stdout) { $stdout | ForEach-Object { Write-Log "  stdout: $_" 'DEBUG' }; Add-Content $LogFile ($stdout -join "`n") }
    if ($stderr) { $stderr | ForEach-Object { Write-Log "  stderr: $_" 'DEBUG' }; Add-Content $LogFile ($stderr -join "`n") }
    if ($proc.ExitCode -ne 0) {
        Fail "$Description failed (exit $($proc.ExitCode))."
    }
}

# ---------------------------------------------------------------------------
# Helper: try to find an executable on PATH
# ---------------------------------------------------------------------------
function Find-Tool {
    param([string]$Name)
    return (Get-Command $Name -ErrorAction SilentlyContinue) -ne $null
}

# ---------------------------------------------------------------------------
# Helper: install a package via winget (admin not required for user scope)
# ---------------------------------------------------------------------------
function Install-ViaWinget {
    param([string]$PackageId, [string]$DisplayName)
    if (-not (Find-Tool 'winget')) {
        Write-Log "winget not available — cannot auto-install $DisplayName" 'WARN'
        return $false
    }
    Write-Log "Installing $DisplayName via winget..."
    $acceptArgs = if ($NonInteractive) { @('--accept-package-agreements', '--accept-source-agreements') } else { @() }
    & winget install --id $PackageId --silent @acceptArgs 2>&1 | Tee-Object -Append -FilePath $LogFile | Write-Log 'DEBUG'
    return ($LASTEXITCODE -eq 0)
}

# ---------------------------------------------------------------------------
# Helper: install a package via Chocolatey
# ---------------------------------------------------------------------------
function Install-ViaChoco {
    param([string]$Package, [string]$DisplayName)
    if (-not (Find-Tool 'choco')) {
        Write-Log "Chocolatey not available — cannot auto-install $DisplayName" 'WARN'
        return $false
    }
    Write-Log "Installing $DisplayName via Chocolatey..."
    $yesFlag = if ($NonInteractive) { '-y' } else { '' }
    & choco install $Package $yesFlag 2>&1 | Tee-Object -Append -FilePath $LogFile | Write-Log 'DEBUG'
    return ($LASTEXITCODE -eq 0)
}

# ---------------------------------------------------------------------------
# Ensure a tool is present; attempt install if allowed
# ---------------------------------------------------------------------------
function Require-Tool {
    param(
        [string]$Executable,
        [string]$DisplayName,
        [string]$WingetId,
        [string]$ChocoPackage
    )
    if (Find-Tool $Executable) {
        Write-Log "$DisplayName found: $(Get-Command $Executable | Select-Object -ExpandProperty Source)"
        return
    }
    if ($NoInstall) {
        Fail "$DisplayName ($Executable) is not installed. Re-run without -NoInstall to allow automatic installation, or install it manually."
    }
    Write-Log "$DisplayName not found — attempting automatic installation..." 'WARN'
    $installed = $false
    if ($WingetId)    { $installed = Install-ViaWinget $WingetId $DisplayName }
    if (-not $installed -and $ChocoPackage) { $installed = Install-ViaChoco $ChocoPackage $DisplayName }
    if (-not $installed) {
        Fail "Could not install $DisplayName automatically. Please install it manually and re-run this script.`nInstaller page: https://cmake.org/download/ (CMake), https://git-scm.com/ (Git), etc."
    }
    # Refresh PATH so the newly installed tool is visible
    $env:Path = [System.Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' +
                [System.Environment]::GetEnvironmentVariable('Path', 'User')
    if (-not (Find-Tool $Executable)) {
        Fail "$DisplayName was installed but '$Executable' is still not on PATH. Open a new terminal and re-run."
    }
}

# ---------------------------------------------------------------------------
# Output / artifact directories
# ---------------------------------------------------------------------------
$distDir = Join-Path $repoRoot 'dist'
if ($Clean -and (Test-Path $distDir)) {
    Write-Log "Clean requested — removing $distDir"
    Remove-Item $distDir -Recurse -Force
}
New-Item -ItemType Directory -Path $distDir -Force | Out-Null

# ---------------------------------------------------------------------------
# ===  CMake (C/C++) build  ===
# ---------------------------------------------------------------------------
if ($hasCMake) {
    Write-Log "--- CMake build ---"
    Require-Tool 'cmake' 'CMake' 'Kitware.CMake' 'cmake'

    # Ensure a C compiler is available; install MinGW-w64 if nothing is found
    $hasCC = (Find-Tool 'cl') -or (Find-Tool 'gcc') -or (Find-Tool 'clang')
    if (-not $hasCC) {
        if ($NoInstall) {
            Fail "No C compiler found (cl/gcc/clang). Install Visual Studio Build Tools (https://aka.ms/vs/buildtools) or MinGW-w64 (https://winlibs.com/) and re-run."
        }
        Write-Log "No C compiler found — installing MinGW-w64 (GCC for Windows)..." 'WARN'
        $installed = Install-ViaWinget 'Winlibs.MinGW' 'MinGW-w64 (GCC)'
        if (-not $installed) { $installed = Install-ViaChoco 'mingw' 'MinGW-w64 (GCC)' }
        if (-not $installed) {
            Fail "Could not install a C compiler automatically. Please install Visual Studio Build Tools (https://aka.ms/vs/buildtools) or MinGW-w64 (https://winlibs.com/) and re-run this script."
        }
        # Refresh PATH so the newly installed compiler is visible
        $env:Path = [System.Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' +
                    [System.Environment]::GetEnvironmentVariable('Path', 'User') + ';' +
                    $env:Path
        if (-not ((Find-Tool 'cl') -or (Find-Tool 'gcc') -or (Find-Tool 'clang'))) {
            Fail "MinGW-w64 was installed but no C compiler (cl/gcc/clang) is still on PATH. Open a new terminal and re-run this script."
        }
    }

    # Prefer MSVC (Visual Studio 2022) if available; fall back to Ninja or MinGW Makefiles
    $generator = $null
    if (Find-Tool 'cl') {
        $generator = 'Visual Studio 17 2022'
        Write-Log "Using generator: $generator (MSVC)"
    } elseif (Find-Tool 'ninja') {
        $generator = 'Ninja'
        Write-Log "Using generator: Ninja"
    } elseif (Find-Tool 'gcc') {
        $generator = 'MinGW Makefiles'
        Write-Log "Using generator: MinGW Makefiles (GCC)"
    } else {
        Write-Log "No preferred generator detected; letting CMake choose the default." 'WARN'
    }

    $buildDir  = Join-Path $repoRoot 'build'
    $cmakeOutDir = Join-Path $distDir 'cmake'
    New-Item -ItemType Directory -Path $buildDir   -Force | Out-Null
    New-Item -ItemType Directory -Path $cmakeOutDir -Force | Out-Null

    # Configure
    $configArgs = @('-S', $repoRoot, '-B', $buildDir, '-DCMAKE_BUILD_TYPE=Release')
    if ($generator) { $configArgs += @('-G', $generator) }
    Invoke-BuildCommand 'CMake configure' 'cmake' $configArgs

    # Build (--parallel is generator-agnostic and supported by CMake 3.12+)
    Invoke-BuildCommand 'CMake build' 'cmake' @('--build', $buildDir, '--config', 'Release', '--parallel', '4')

    # Copy artifacts to dist/cmake
    $dllPaths = @(
        (Join-Path $buildDir 'Release\gamex86.dll'),
        (Join-Path $buildDir 'gamex86.dll'),
        (Join-Path $buildDir 'Release\gamei386.so'),
        (Join-Path $buildDir 'gamei386.so')
    )
    foreach ($p in $dllPaths) {
        if (Test-Path $p) {
            Copy-Item $p $cmakeOutDir -Force
            Write-Log "Artifact: $p -> $cmakeOutDir"
        }
    }

    # Tests
    if (-not $SkipTests) {
        Write-Log "Running CMake tests (bot_test)..."
        $testBinPaths = @(
            (Join-Path $buildDir 'Release\bot_test.exe'),
            (Join-Path $buildDir 'bot_test.exe')
        )
        $testBin = $testBinPaths | Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($testBin) {
            Invoke-BuildCommand 'bot_test' $testBin @()
            Write-Log "CMake tests passed."
        } else {
            # Try ctest as a fallback
            if (Find-Tool 'ctest') {
                Invoke-BuildCommand 'ctest' 'ctest' @('--test-dir', $buildDir, '--build-config', 'Release', '--output-on-failure')
                Write-Log "CTest passed."
            } else {
                Write-Log "Test binary not found; skipping CMake test step." 'WARN'
            }
        }
    }
}

# ---------------------------------------------------------------------------
# ===  Node.js build  ===
# ---------------------------------------------------------------------------
if ($hasNode) {
    Write-Log "--- Node.js build ---"
    Require-Tool 'node' 'Node.js' 'OpenJS.NodeJS' 'nodejs'

    $pkgJson = Get-Content (Join-Path $repoRoot 'package.json') -Raw | ConvertFrom-Json

    # Detect package manager from lockfiles
    $pm = 'npm'
    if (Test-Path (Join-Path $repoRoot 'pnpm-lock.yaml')) { $pm = 'pnpm' }
    elseif (Test-Path (Join-Path $repoRoot 'yarn.lock'))  { $pm = 'yarn' }
    Write-Log "Node package manager: $pm"

    $installCmd = switch ($pm) {
        'pnpm' { @('install', '--frozen-lockfile') }
        'yarn' { @('install', '--frozen-lockfile') }
        default { @('ci') }
    }
    Invoke-BuildCommand "Node install ($pm)" $pm $installCmd

    if ($pkgJson.scripts -and $pkgJson.scripts.build) {
        Invoke-BuildCommand 'Node build' $pm @('run', 'build')
    }

    # Copy output to dist/node
    $nodeDistDir = Join-Path $distDir 'node'
    foreach ($candidate in @('dist', 'build', 'out', 'lib')) {
        $src = Join-Path $repoRoot $candidate
        if ((Test-Path $src) -and ($src -ne $distDir)) {
            Copy-Item $src $nodeDistDir -Recurse -Force
            Write-Log "Node artifacts: $src -> $nodeDistDir"
            break
        }
    }

    if (-not $SkipTests -and $pkgJson.scripts -and $pkgJson.scripts.test) {
        Invoke-BuildCommand 'Node tests' $pm @('test')
        Write-Log "Node tests passed."
    }
}

# ---------------------------------------------------------------------------
# ===  Python build  ===
# ---------------------------------------------------------------------------
if ($hasPython) {
    Write-Log "--- Python build ---"
    Require-Tool 'python' 'Python 3' 'Python.Python.3.11' 'python'

    $venvDir = Join-Path $repoRoot '.venv'
    if (-not (Test-Path $venvDir)) {
        Invoke-BuildCommand 'Python venv create' 'python' @('-m', 'venv', $venvDir)
    }
    $pip = Join-Path $venvDir 'Scripts\pip.exe'

    Invoke-BuildCommand 'pip upgrade' $pip @('install', '--upgrade', 'pip', 'wheel', 'build')

    if (Test-Path (Join-Path $repoRoot 'requirements.txt')) {
        Invoke-BuildCommand 'pip install requirements' $pip @('install', '-r', 'requirements.txt')
    } elseif (Test-Path (Join-Path $repoRoot 'pyproject.toml')) {
        Invoke-BuildCommand 'pip install project' $pip @('install', '.')
    } elseif (Test-Path (Join-Path $repoRoot 'setup.py')) {
        Invoke-BuildCommand 'pip install setup.py' $pip @('install', '.')
    }

    $pyDistDir = Join-Path $distDir 'python'
    New-Item -ItemType Directory -Path $pyDistDir -Force | Out-Null

    $pyBuildToml = Get-Content (Join-Path $repoRoot 'pyproject.toml') -ErrorAction SilentlyContinue
    if ($pyBuildToml -match 'build-system') {
        $python = Join-Path $venvDir 'Scripts\python.exe'
        Invoke-BuildCommand 'Python build wheel' $python @('-m', 'build', '--outdir', $pyDistDir)
        Write-Log "Python wheel/sdist artifacts in $pyDistDir"
    }

    if (-not $SkipTests) {
        $pytest = Join-Path $venvDir 'Scripts\pytest.exe'
        if (Test-Path $pytest) {
            $testDir = Join-Path $repoRoot 'tests'
            if (-not (Test-Path $testDir)) { $testDir = Join-Path $repoRoot 'test' }
            if (Test-Path $testDir) {
                Invoke-BuildCommand 'pytest' $pytest @($testDir, '-v')
                Write-Log "Python tests passed."
            }
        }
    }
}

# ---------------------------------------------------------------------------
# ===  .NET build  ===
# ---------------------------------------------------------------------------
if ($hasDotnet) {
    Write-Log "--- .NET build ---"
    Require-Tool 'dotnet' '.NET SDK' 'Microsoft.DotNet.SDK.8' 'dotnet-sdk'

    $dotnetOutDir = Join-Path $distDir 'dotnet'
    New-Item -ItemType Directory -Path $dotnetOutDir -Force | Out-Null

    Invoke-BuildCommand 'dotnet restore' 'dotnet' @('restore')
    Invoke-BuildCommand 'dotnet build'   'dotnet' @('build', '-c', 'Release', '--no-restore')
    Invoke-BuildCommand 'dotnet publish' 'dotnet' @('publish', '-c', 'Release', '-o', $dotnetOutDir, '--no-restore')

    if (-not $SkipTests) {
        Invoke-BuildCommand 'dotnet test' 'dotnet' @('test', '-c', 'Release', '--no-build', '--logger', 'console;verbosity=normal')
        Write-Log ".NET tests passed."
    }
}

# ---------------------------------------------------------------------------
# ===  Go build  ===
# ---------------------------------------------------------------------------
if ($hasGo) {
    Write-Log "--- Go build ---"
    Require-Tool 'go' 'Go' 'GoLang.Go' 'golang'

    $goOutDir = Join-Path $distDir 'go'
    New-Item -ItemType Directory -Path $goOutDir -Force | Out-Null

    Invoke-BuildCommand 'go mod download' 'go' @('mod', 'download')
    Invoke-BuildCommand 'go build' 'go' @('build', '-o', "$goOutDir\", './...')

    if (-not $SkipTests) {
        Invoke-BuildCommand 'go test' 'go' @('test', './...', '-v')
        Write-Log "Go tests passed."
    }
}

# ---------------------------------------------------------------------------
# ===  Docker build  ===
# ---------------------------------------------------------------------------
if ($hasDocker) {
    Write-Log "--- Docker build ---"
    if (Find-Tool 'docker') {
        $imageTag = "q2gloombot:build-$(Get-Date -Format 'yyyyMMddHHmmss')"
        Invoke-BuildCommand 'docker build' 'docker' @('build', '-t', $imageTag, $repoRoot)
        Write-Log "Docker image built: $imageTag"
    } else {
        Write-Log "Docker not installed; skipping Docker build." 'WARN'
    }
}

# ---------------------------------------------------------------------------
# ===  Artifact zip  ===
# ---------------------------------------------------------------------------
if ($ArtifactZip) {
    $zipPath = Join-Path $repoRoot "dist-$(Get-Date -Format 'yyyyMMdd-HHmmss').zip"
    Write-Log "Creating artifact zip: $zipPath"
    Compress-Archive -Path "$distDir\*" -DestinationPath $zipPath -Force
    Write-Log "Artifact zip created: $zipPath"
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Write-Log "=== Build complete ==="
Write-Log "Artifacts: $distDir"
if ($ArtifactZip) { Write-Log "Zip: $zipPath" }
Write-Log "Log: $LogFile"
Write-Host ""
Write-Host "Build succeeded." -ForegroundColor Green
Write-Host "Artifacts : $distDir"
Write-Host "Build log : $LogFile"
exit 0
