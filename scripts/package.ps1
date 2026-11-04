param([ValidateSet('opengl','vulkan-native','vulkan-nvpro','dx12')][string]$Track='vulkan-native')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$folders=@{'opengl'='OpenGL';'vulkan-native'='Vulkan\native';'vulkan-nvpro'='Vulkan\nvpro';'dx12'='DirectX12'}
$folder=Join-Path $root $folders[$Track]
$bin=Join-Path $folder 'build\bin\Release'
if(!(Test-Path $bin)){throw 'Build the Release preset first.'}
$dist=Join-Path $root 'dist'
New-Item -ItemType Directory -Force $dist | Out-Null
Compress-Archive -Path (Join-Path $bin '*') -DestinationPath (Join-Path $dist "$Track-windows-x64.zip") -Force
Write-Host (Join-Path $dist "$Track-windows-x64.zip")
