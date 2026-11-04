param([string]$Lesson='02_triangle',[ValidateSet('Debug','Release')][string]$Config='Debug')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$names=Get-ChildItem (Join-Path $root 'lessons') -Directory | Select-Object -ExpandProperty Name
if($Lesson -notin $names){throw "课程不存在，可选：$($names -join ', ')"}
$exe=Join-Path $root "build\bin\$Config\$Lesson.exe"
if(!(Test-Path $exe)){throw '请先运行 scripts\build.ps1'}
& $exe
if($LASTEXITCODE -ne 0){throw "程序退出码：$LASTEXITCODE"}
