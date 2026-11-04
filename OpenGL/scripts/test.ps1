param([ValidateSet('Debug','Release')][string]$Config='Debug')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
ctest --test-dir (Join-Path $root 'build') -C $Config --output-on-failure
if($LASTEXITCODE -ne 0){throw 'Smoke tests failed'}
