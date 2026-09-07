[CmdletBinding()]
param([ValidateSet('Baseline','Modified','Rollback')][string]$Variant='Modified',[string]$EngineRoot='D:\Unreal5.8\UE_5.8')
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$work=Join-Path $projectRoot 'work\menu_multiplayer_20260906'
if($Variant -eq 'Modified') { $testRoot=$projectRoot;$expected=4 }
else { $testRoot=Join-Path $work 'rollback_sandbox\backrooms';$expected=1 }
# Baseline and Rollback run the verified original-source fixture; Modified runs the live changed project.
$project=Join-Path $testRoot 'backrooms.uproject'
$log=Join-Path $work ("FRAMEWORK_"+$Variant.ToUpperInvariant()+'.log')
$exe=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $exe $project /Engine/Maps/Entry -NullRHI -unattended -nop4 -nosound -NoSplash '-ExecCmds=Automation RunTests Backrooms' '-TestExit=Automation Test Queue Empty' "-abslog=$log" -stdout *> ($log+'.stdout')
$code=$LASTEXITCODE
$text=Get-Content -LiteralPath $log -Raw
$passed=[regex]::Matches($text,'Test Completed\. Result=\{Success\}').Count
$ok=$code -eq 0 -and $passed -eq $expected -and $text -notmatch 'Test Completed\. Result=\{Fail'
$label=$Variant.ToUpperInvariant()
$result=if($ok){'PASS'}else{'FAIL'}
Write-Output "$label result=$result tests=$passed expected=$expected engine_exit=$code"
[pscustomobject]@{variant=$Variant;project=$project;command=@($exe,$project,'/Engine/Maps/Entry','-NullRHI','-unattended','-nop4','-nosound','-NoSplash','-ExecCmds=Automation RunTests Backrooms','-TestExit=Automation Test Queue Empty',"-abslog=$log",'-stdout');log=$log;engine_exit=$code;tests=$passed;expected=$expected;result=$result} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath ($log+'.json') -Encoding utf8
if(!$ok){exit 1}
