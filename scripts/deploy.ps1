param([switch]$Build,[switch]$PrepareCache)
$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
  git submodule update --init --recursive
  if($LASTEXITCODE -ne 0){throw 'Dependency checkout failed'}
  cmake --version
  if($LASTEXITCODE -ne 0){throw 'Install CMake 3.24+'}
  if(!$env:VULKAN_SDK){Write-Host 'VULKAN_SDK is unset. Install Vulkan SDK and reopen this terminal before building Vulkan.'}
  if($PrepareCache){& .\scripts\prepare-dependencies.ps1 -Track all}
  if($Build){& .\scripts\build.ps1 -Track all -Test}
}finally{Pop-Location}
