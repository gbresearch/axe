[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration,

    [string] $YadroDir = (Join-Path $PSScriptRoot '../../yadro')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repository = Split-Path -Parent $PSScriptRoot
$YadroDir = (Resolve-Path -LiteralPath $YadroDir).Path
if ((Split-Path -Leaf $YadroDir) -ne 'yadro' -or
    -not (Test-Path -LiteralPath (Join-Path $YadroDir 'util/gbtest.h'))) {
    throw 'YadroDir must point to a directory named yadro containing util/gbtest.h.'
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -products '*' -version '[18.0,19.0)' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) {
    throw 'Visual Studio 2026 with the v145 x64 C++ tools is required.'
}

Import-Module (Join-Path $installation 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation `
    -DevCmdArguments '-arch=x64 -host_arch=x64'

function Invoke-Checked {
    param([string] $Program, [string[]] $Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE."
    }
}

$outputDirectory = Join-Path $repository "build/windows-msvc/$Configuration"
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$previousCl = $env:CL
# AXE test sources include this checkout's ../include headers directly. Only
# the dependency needs an include root; prepend it for an explicit YadroDir.
$env:CL = '/I"' + (Split-Path -Parent $YadroDir) + '" ' + $previousCl
Push-Location $repository
try {
    Invoke-Checked 'git' @('-C', $YadroDir, 'rev-parse', 'HEAD')
    Invoke-Checked 'msbuild' @(
        'vs/axe_test.vcxproj', '/m', '/nologo', '/verbosity:minimal',
        "/p:Configuration=$Configuration", '/p:Platform=x64', '/p:PlatformToolset=v145',
        "/p:OutDir=$outputDirectory/", "/p:IntDir=$outputDirectory/obj/",
        "/bl:$outputDirectory/msbuild.binlog"
    )

    # The project builds the executable; running it is a separate required gate.
    Push-Location $outputDirectory
    try {
        Invoke-Checked (Join-Path $outputDirectory 'axe_test.exe') @()
    }
    finally {
        Pop-Location
    }

    # MSVC's latest mode is also the /std:c++latest mode used by axe_test.vcxproj.
    foreach ($standard in @('c++17', 'c++20', 'c++latest')) {
        $headerDirectory = Join-Path $outputDirectory "headers/$standard"
        New-Item -ItemType Directory -Force -Path $headerDirectory | Out-Null
        foreach ($header in Get-ChildItem -LiteralPath (Join-Path $repository 'include') -Filter '*.h' -File | Sort-Object Name) {
            $source = Join-Path $headerDirectory "$($header.Name).cpp"
            Set-Content -LiteralPath $source -Value "#include <$($header.Name)>" -Encoding utf8
            # Match the project's warning level; /W4 diagnoses its existing
            # intentional -1-to-size_t unbounded-repetition sentinel (C4245).
            Invoke-Checked 'cl' @(
                '/nologo', '/Zs', "/std:$standard", '/permissive-', '/EHsc', '/W3', '/WX',
                "/I$repository/include", $source
            )
        }
    }

    Invoke-Checked 'python' @('tools/check_docs.py', '--compiler', 'cl', '--std', 'latest')
    Invoke-Checked 'python' @('-m', 'unittest', 'discover', '-s', 'tools', '-p', 'test_check_docs.py', '-v')
}
finally {
    Pop-Location
    $env:CL = $previousCl
}
