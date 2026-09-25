$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$msysRoot = "C:\msys64\ucrt64"
$compiler = Join-Path $msysRoot "bin\g++.exe"
$includePath = Join-Path $msysRoot "include"
$projectIncludePath = Join-Path $projectRoot "include"
$libraryPath = Join-Path $msysRoot "lib"
$runtimePath = Join-Path $msysRoot "bin"
$buildDirectory = Join-Path $projectRoot "build"
$outputPath = Join-Path $buildDirectory "Phase1Farm.exe"

if (-not (Test-Path $compiler)) {
    throw "g++ was not found at $compiler. Install MSYS2 UCRT64 or update the compiler path in run_phase1.ps1."
}

if (-not (Test-Path (Join-Path $includePath "GLFW\glfw3.h"))) {
    throw "GLFW headers were not found under $includePath."
}

if (-not (Test-Path (Join-Path $libraryPath "libglfw3.a"))) {
    throw "GLFW library was not found under $libraryPath."
}

New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

$sourceFiles = @(
    "src\main.cpp",
    "src\Application.cpp",
    "src\Camera.cpp",
    "src\Input.cpp",
    "src\Scene.cpp",
    "src\Animation.cpp",
    "src\Lighting.cpp",
    "src\graphics\Primitives.cpp",
    "src\objects\Farmhouse.cpp",
    "src\objects\Vegetation.cpp",
    "src\objects\Tractor.cpp",
    "src\objects\windmill.cpp"
) | ForEach-Object { Join-Path $projectRoot $_ }

& $compiler `
    "-std=c++17" `
    "-I$includePath" `
    "-I$projectIncludePath" `
    $sourceFiles `
    "-L$libraryPath" `
    "-lglfw3" `
    "-lopengl32" `
    "-lgdi32" `
    "-luser32" `
    "-lshell32" `
    "-o" $outputPath

if ($LASTEXITCODE -ne 0) {
    throw "Phase 1 compilation failed with exit code $LASTEXITCODE."
}

$env:PATH = "$runtimePath;$env:PATH"
Write-Host "Launching $outputPath"
& $outputPath
exit $LASTEXITCODE