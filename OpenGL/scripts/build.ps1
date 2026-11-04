param([ValidateSet('Debug','Release')][string]$Config='Debug')
$ErrorActionPreference='Stop'
$collection=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
& (Join-Path $collection 'scripts\build.ps1') -Track opengl -Config $Config
