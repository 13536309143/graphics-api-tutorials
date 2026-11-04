param([ValidateSet('all','launcher','opengl','vulkan-native','vulkan-nvpro','dx12')][string]$Track='all',[ValidateSet('Debug','Release')][string]$Config='Debug',[switch]$Test)
$ErrorActionPreference='Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
  $tracks=if($Track -eq 'all'){@('launcher','opengl','vulkan-native','vulkan-nvpro','dx12')}else{@($Track)}
  foreach($item in $tracks){
    cmake --preset $item
    if($LASTEXITCODE -ne 0){throw "Configure failed: $item"}
    $preset="$item-$($Config.ToLowerInvariant())"
    cmake --build --preset $preset --parallel 4
    if($LASTEXITCODE -ne 0){throw "Build failed: $item"}
    if($Test -and $item -ne 'launcher'){
      ctest --preset $preset
      if($LASTEXITCODE -ne 0){throw "GPU tests failed: $item"}
    }
  }
}finally{Pop-Location}
