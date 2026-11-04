param([ValidateSet('all','opengl','vulkan-native','vulkan-nvpro','dx12')][string]$Track='all')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cache=Join-Path $root '.cache\downloads'
New-Item -ItemType Directory -Force $cache | Out-Null
$packages=@(
  @('glfw','https://codeload.github.com/glfw/glfw/zip/refs/tags/3.4','a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a','glfw-3.4','OpenGL\third_party\glfw','CMakeLists.txt'),
  @('glm','https://codeload.github.com/g-truc/glm/zip/1.0.1','09c5716296787e1f7fcb87b1cbdbf26814ec1288ed6259ccd30d5d9795809fa5','glm-1.0.1','OpenGL\third_party\glm','glm\glm.hpp')
)
if($Track -in @('all','opengl','vulkan-native','dx12')){
  $packages+=,@('assimp','https://codeload.github.com/assimp/assimp/zip/refs/tags/v5.4.3','795c29716f4ac123b403e53b677e9f32a8605c4a7b2d9904bfaae3f4053b506d','assimp-5.4.3','OpenGL\third_party\assimp','CMakeLists.txt')
}
if($Track -in @('all','vulkan-nvpro')){
  $packages+=,@('vulkan-headers','https://codeload.github.com/KhronosGroup/Vulkan-Headers/zip/refs/tags/vulkan-sdk-1.4.341.0','1a0ea8ce1f1f08bcf9a7e4962646ba6b2a09b40108ca516fc6a006e45c77cb65','Vulkan-Headers-vulkan-sdk-1.4.341.0','Vulkan\nvpro\external\Vulkan-Headers','include\vulkan\vulkan.h')
  $packages+=,@('vulkan-utils','https://codeload.github.com/KhronosGroup/Vulkan-Utility-Libraries/zip/refs/tags/vulkan-sdk-1.4.341.0','61de60ed4c2a6c10b1c7091a472f1fdc4fbab5031f8fe1f85d4a72e49552b779','Vulkan-Utility-Libraries-vulkan-sdk-1.4.341.0','Vulkan\nvpro\external\Vulkan-Utility-Libraries','include\vulkan\vk_enum_string_helper.h')
}
foreach($package in $packages){
  $name,$url,$hash,$archiveRoot,$relative,$marker=$package
  $destination=Join-Path $root $relative
  if(Test-Path (Join-Path $destination $marker)){Write-Host "Using source cache: $name";continue}
  $zip=Join-Path $cache "$name.zip"
  if(!(Test-Path $zip) -or (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash -ne $hash){
    Write-Host "Downloading $name"
    Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $zip -TimeoutSec 120
  }
  if((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash -ne $hash){throw "SHA256 mismatch: $name"}
  $extract=Join-Path $cache "extract-$name"
  Expand-Archive -LiteralPath $zip -DestinationPath $extract -Force
  $unpacked=Join-Path $extract $archiveRoot
  New-Item -ItemType Directory -Force $destination | Out-Null
  Copy-Item -Path (Join-Path $unpacked '*') -Destination $destination -Recurse -Force
  Write-Host "Prepared verified source cache: $name"
}
