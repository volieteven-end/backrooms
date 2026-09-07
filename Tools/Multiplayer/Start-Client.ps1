[CmdletBinding()]
param(
 [string]$ClientExe,
 [switch]$EditorRuntime,
 [string]$EngineRoot='D:\Unreal5.8\UE_5.8',
 [ValidatePattern('^[a-zA-Z0-9.:-]+$')][string]$ServerHost='127.0.0.1',
 [ValidateRange(1024,65535)][int]$GamePort=7777,
 [ValidateRange(1024,65535)][int]$BeaconPort=15000
)
$ErrorActionPreference='Stop'
if ($EditorRuntime) {
 $exe=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
 $project=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\backrooms.uproject'))
 $argsList=@("`"$project`"",'/Game/UI/Menu/Maps/L_MainMenu','-game')
} else {
 if (!$ClientExe) { throw 'Pass ClientExe, or select EditorRuntime for development.' }
 $exe=$ClientExe;$argsList=@('/Game/UI/Menu/Maps/L_MainMenu')
}
$exe=(Resolve-Path -LiteralPath $exe).Path
$argsList+=@("-BRServerHost=$ServerHost","-BRGamePort=$GamePort","-BRBeaconPort=$BeaconPort",'-windowed','-ResX=1920','-ResY=1080')
# Game UI is interactive. Hidden startup prevents an extra console; the game opens its own viewport.
$p=Start-Process -FilePath $exe -ArgumentList $argsList -WorkingDirectory (Split-Path -Parent $exe) -PassThru -WindowStyle Hidden
Write-Output "CLIENT_START pid=$($p.Id) server=$ServerHost game_udp=$GamePort beacon_udp=$BeaconPort"
