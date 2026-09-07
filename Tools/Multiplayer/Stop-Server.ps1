[CmdletBinding()]
param([string]$StateFile)
# Resolve the default in script scope, not in the parameter initializer.
$ErrorActionPreference='Stop'
if ([string]::IsNullOrWhiteSpace($StateFile)) {
 $StateFile=Join-Path $PSScriptRoot '..\..\Saved\Multiplayer\server-process.json'
}
$state=(Resolve-Path -LiteralPath $StateFile).Path
$s=Get-Content -LiteralPath $state -Raw | ConvertFrom-Json
$p=Get-CimInstance Win32_Process -Filter "ProcessId=$($s.ProcessId)"
if (!$p) { Write-Output 'SERVER_STOP result=ALREADY_STOPPED';return }
$exe=(Resolve-Path -LiteralPath $s.Executable).Path
if (![string]::Equals($p.ExecutablePath,$exe,[StringComparison]::OrdinalIgnoreCase) -or $p.CreationDate.ToUniversalTime().Ticks -ne ([datetime]$s.Created).ToUniversalTime().Ticks) { throw 'PID identity differs from the recorded server; no process changed.' }
if ($p.CommandLine -notlike ('*'+$s.Log+'*')) { throw 'Server log marker differs; no process changed.' }
# Keep the verified process handle while stopping; PID lookup can still see an exiting process.
$tracked=Get-Process -Id $s.ProcessId -ErrorAction Stop
Stop-Process -InputObject $tracked
if (!$tracked.WaitForExit(60000)) { throw 'Server did not finish exiting within 60 seconds.' }
Write-Output "SERVER_STOP result=PASS pid=$($s.ProcessId)"
