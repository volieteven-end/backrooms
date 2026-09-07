[CmdletBinding()]
param(
 [string]$ServerExe,
 [switch]$EditorRuntime,
 [string]$EngineRoot = 'D:\Unreal5.8\UE_5.8',
 [ValidateRange(1024,65535)][int]$GamePort=7777,
 [ValidateRange(1024,65535)][int]$BeaconPort=15000,
 [string]$StateFile
)
# Resolve script-relative defaults after parameter binding (Windows PowerShell 5.1 -File).
$ErrorActionPreference='Stop'
if ([string]::IsNullOrWhiteSpace($StateFile)) {
 $StateFile=Join-Path $PSScriptRoot '..\..\Saved\Multiplayer\server-process.json'
}
$project=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\backrooms.uproject'))
$state=[IO.Path]::GetFullPath($StateFile)
if (Test-Path -LiteralPath $state) {
 $old=Get-Content -LiteralPath $state -Raw | ConvertFrom-Json
 if (Get-Process -Id $old.ProcessId -ErrorAction SilentlyContinue) { throw 'Tracked process is still present. Check or stop it before starting a replacement.' }
}
$root=Split-Path -Parent $state;New-Item -ItemType Directory -Force -Path $root | Out-Null
$log=Join-Path $root ('Server-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.log')
if ($GamePort -eq $BeaconPort) { throw 'GamePort and BeaconPort must differ.' }
foreach ($port in @($GamePort,$BeaconPort)) {
 if (Get-NetUDPEndpoint -LocalPort $port -ErrorAction SilentlyContinue) { throw "UDP port $port is already bound." }
}
if ($EditorRuntime) { $exe=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$argsList=@("`"$project`"",'/Game/UI/Menu/Maps/L_Lobby','-server') }
else {
 if (!$ServerExe) { throw 'Pass ServerExe pointing to the packaged Server executable, or select EditorRuntime explicitly for development.' }
 $exe=$ServerExe;$argsList=@('/Game/UI/Menu/Maps/L_Lobby')
}
$exe=(Resolve-Path -LiteralPath $exe).Path
$argsList+=@("-port=$GamePort",'-NullRHI','-unattended','-nosound','-nop4',"`"-abslog=$log`"", "-BRGamePort=$GamePort","-BRBeaconPort=$BeaconPort")
$p=Start-Process -FilePath $exe -ArgumentList $argsList -WorkingDirectory (Split-Path -Parent $exe) -PassThru -WindowStyle Hidden
$process=Get-CimInstance Win32_Process -Filter "ProcessId=$($p.Id)"
[pscustomobject]@{ProcessId=$p.Id;Executable=$exe;Created=$process.CreationDate.ToUniversalTime().ToString('o');Log=$log;GamePort=$GamePort;BeaconPort=$BeaconPort;EditorRuntime=[bool]$EditorRuntime} | ConvertTo-Json | Set-Content -LiteralPath $state -Encoding utf8
$deadline=(Get-Date).AddSeconds(60)
do {
 if ($p.HasExited) { throw "Server exited with $($p.ExitCode). Inspect $log" }
 if ((Test-Path -LiteralPath $log) -and (Select-String -LiteralPath $log -Pattern "BR_BEACON result=LISTENING port=$BeaconPort" -Quiet)) {
  Write-Output "SERVER_START result=PASS pid=$($p.Id) game_udp=$GamePort beacon_udp=$BeaconPort editor_runtime=$EditorRuntime log=$log";return
 }
 Start-Sleep -Milliseconds 500
} while ((Get-Date) -lt $deadline)
throw "Server did not report its Beacon listener within 60 seconds. Tracked PID $($p.Id); inspect $log and use Stop-Server.ps1."
