# Сборка дистрибутива: ERTrainer\..\dist\ERTrainer.exe (без файлового лога)
$ErrorActionPreference = "Stop"
$cmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root "build"
$dist = Join-Path (Split-Path -Parent $root) "dist"

& $cmake -S $root -B $build -DERTRAINER_DIST=ON
& $cmake --build $build --config Release

$built = Join-Path $build "bin\Release\ERTrainer129.exe"
if (-not (Test-Path $built)) {
    throw "Не найден: $built"
}

New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item -Force $built (Join-Path $dist "ERTrainer.exe")
Write-Host "Готово: $(Join-Path $dist 'ERTrainer.exe')"
