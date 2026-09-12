param([string]$Compiler = '')

$ErrorActionPreference = 'Stop'

$sketchDir = Split-Path -Parent $PSScriptRoot
$fakeDir = Join-Path $PSScriptRoot 'fakes'
if (-not $Compiler) {
  $compilerCommand = Get-Command 'clang++' -ErrorAction SilentlyContinue
  if ($compilerCommand) {
    $Compiler = $compilerCommand.Source
  } else {
    $localCompiler = Join-Path $env:USERPROFILE '.local\llvm-21.1.8\bin\clang++.exe'
    if (Test-Path -LiteralPath $localCompiler) { $Compiler = $localCompiler }
  }
}
if (-not $Compiler -or -not (Test-Path -LiteralPath $Compiler)) {
  throw 'clang++ was not found. Pass its path with -Compiler.'
}

$tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$testRunDir = Join-Path $tempBase ('hourglass-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRunDir | Out-Null
$reconnectOutput = Join-Path $testRunDir 'reconnect.exe'
$flipOutput = Join-Path $testRunDir 'flip-flow.exe'
$smoothingOutput = Join-Path $testRunDir 'orientation-smoothing.exe'
$timingOutput = Join-Path $testRunDir 'timing-responsiveness.exe'
$lifecycleOutput = Join-Path $testRunDir 'lifecycle-replay.exe'
$autoReverseOutput = Join-Path $testRunDir 'auto-reverse.exe'
$pauseResponsivenessOutput = Join-Path $testRunDir 'pause-responsiveness.exe'

function Build-Test($testSource, $output) {
  & $Compiler `
    -std=c++17 `
    -DARDUINO=10819 `
    -I $fakeDir `
    -I $sketchDir `
    -x c++ `
    (Join-Path $sketchDir 'hourglass.ino') `
    (Join-Path $sketchDir 'LedControl.cpp') `
    (Join-Path $sketchDir 'Delay.cpp') `
    (Join-Path $PSScriptRoot $testSource) `
    -o $output

  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

try {
  Build-Test 'reconnect_test.cpp' $reconnectOutput
  & $reconnectOutput
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Build-Test 'flip_flow_test.cpp' $flipOutput
  & $flipOutput
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Build-Test 'orientation_smoothing_test.cpp' $smoothingOutput
  & $smoothingOutput
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Build-Test 'timing_responsiveness_test.cpp' $timingOutput
  & $timingOutput
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Build-Test 'lifecycle_replay_test.cpp' $lifecycleOutput
  foreach ($seed in @(1, 17, 123, 997, 4093)) {
    & $lifecycleOutput $seed
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  }

  Build-Test 'auto_reverse_test.cpp' $autoReverseOutput
  & $autoReverseOutput
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Build-Test 'pause_responsiveness_test.cpp' $pauseResponsivenessOutput
  & $pauseResponsivenessOutput
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  exit 0
} finally {
  $resolvedRunDir = [IO.Path]::GetFullPath($testRunDir)
  if ($resolvedRunDir.StartsWith($tempBase) -and (Test-Path -LiteralPath $resolvedRunDir)) {
    Remove-Item -LiteralPath $resolvedRunDir -Recurse -Force
  }
}
