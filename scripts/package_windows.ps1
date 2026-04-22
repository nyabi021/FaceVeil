param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$OpenCvRoot = $env:OpenCV_DIR,
    [string]$OnnxRuntimeRoot = $env:ONNXRUNTIME_ROOT,
    [string]$Generator = "Ninja",
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildDir = Join-Path $RootDir "build-windows"
$DistDir = Join-Path $RootDir "dist/windows/FaceVeil"
$ExePath = Join-Path $BuildDir "FaceVeil.exe"

function Resolve-QtRootPath {
    param([string]$Path)

    if (-not $Path) {
        if ($env:QTDIR) {
            $Path = $env:QTDIR
        } elseif ($env:Qt6_DIR) {
            $Path = $env:Qt6_DIR
        }
    }

    if (-not $Path) {
        return $null
    }

    $resolved = Resolve-Path $Path
    if ((Split-Path $resolved -Leaf) -eq "Qt6") {
        $resolved = Resolve-Path (Join-Path $resolved "../../..")
    }

    return $resolved
}

$QtRoot = Resolve-QtRootPath $QtRoot

if (-not $QtRoot) {
    throw "QtRoot was not provided. Pass -QtRoot C:\Qt\6.x.x\msvc2022_64 or set QT_ROOT, QTDIR, or Qt6_DIR."
}
if (-not $OpenCvRoot) {
    throw "OpenCvRoot was not provided. Pass -OpenCvRoot C:\opencv\build or set OpenCV_DIR."
}
if (-not $OnnxRuntimeRoot) {
    throw "OnnxRuntimeRoot was not provided. Pass -OnnxRuntimeRoot C:\onnxruntime-win-x64 or set ONNXRUNTIME_ROOT."
}

$OpenCvRoot = Resolve-Path $OpenCvRoot
$OnnxRuntimeRoot = Resolve-Path $OnnxRuntimeRoot

cmake -S $RootDir -B $BuildDir `
    -G $Generator `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DCMAKE_PREFIX_PATH="$QtRoot;$OpenCvRoot" `
    -DONNXRUNTIME_ROOT=$OnnxRuntimeRoot
cmake --build $BuildDir --config $BuildType

if (-not (Test-Path $ExePath)) {
    $ExePath = Join-Path $BuildDir "$BuildType/FaceVeil.exe"
}
if (-not (Test-Path $ExePath)) {
    throw "FaceVeil.exe was not found after build."
}

if (Test-Path $DistDir) {
    Remove-Item $DistDir -Recurse -Force
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

Copy-Item $ExePath $DistDir

$windeployqt = Join-Path $QtRoot "bin/windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt.exe was not found under $QtRoot\bin"
}
& $windeployqt --release --compiler-runtime (Join-Path $DistDir "FaceVeil.exe")

$onnxDll = Join-Path $OnnxRuntimeRoot "lib/onnxruntime.dll"
if (-not (Test-Path $onnxDll)) {
    $onnxDll = Join-Path $OnnxRuntimeRoot "bin/onnxruntime.dll"
}
if (-not (Test-Path $onnxDll)) {
    throw "onnxruntime.dll was not found under $OnnxRuntimeRoot\lib or $OnnxRuntimeRoot\bin"
}
Copy-Item $onnxDll $DistDir -Force

$opencvDlls = Get-ChildItem -Path $OpenCvRoot -Recurse -Filter "opencv_world*.dll" -ErrorAction SilentlyContinue
if (-not $opencvDlls) {
    $opencvDlls = Get-ChildItem -Path $OpenCvRoot -Recurse -Filter "opencv_*.dll" -ErrorAction SilentlyContinue
}
foreach ($dll in $opencvDlls) {
    Copy-Item $dll.FullName $DistDir -Force
}

$modelsDir = Join-Path $DistDir "models"
New-Item -ItemType Directory -Path $modelsDir | Out-Null
Copy-Item (Join-Path $RootDir "models/*.onnx") $modelsDir -Force

Write-Host "Packaged app: $DistDir"
Write-Host "Run with: $DistDir\FaceVeil.exe"
