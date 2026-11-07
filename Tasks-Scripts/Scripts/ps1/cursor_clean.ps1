#Requires -RunAsAdministrator
[CmdletBinding(SupportsShouldProcess)] param()
$sysKey = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Control Panel\Cursors\Schemes'
$usrKey = 'HKCU:\Control Panel\Cursors\Schemes'
$base = "$env:windir\Cursors"
if (!(Test-Path $usrKey)) { return }

function Get-Files($k, $n) {
  $k.GetValue($n) -split ',' | % { [Environment]::ExpandEnvironmentVariables($_.Trim()) } | ? { $_ -match '\.(cur|ani)$' }
}

# built-in = everything registered under HKLM
$sys = Get-Item $sysKey; $usr = Get-Item $usrKey
$sysNames = @($sys.GetValueNames() | ? { $_ })
$keep = $sysNames | % { Get-Files $sys $_ }

# custom = HKCU schemes not present in HKLM; only .cur/.ani files not used by built-ins
$names = @($usr.GetValueNames() | ? { $_ -and $_ -notin $sysNames })
$kill = @($names | % { Get-Files $usr $_ } | sort -Unique | ? { $_ -notin $keep })

# reset only active roles pointing to removed files
$cur = 'HKCU:\Control Panel\Cursors'; $act = Get-ItemProperty $cur
$hit = 'Arrow','Help','AppStarting','Wait','Crosshair','IBeam','NWPen','No','SizeNS','SizeWE','SizeNWSE','SizeNESW','SizeAll','UpArrow','Hand','Pin','Person' |
  ? { $act.$_ -and ([Environment]::ExpandEnvironmentVariables($act.$_) -in $kill) }
if ($hit) { $hit | % { Set-ItemProperty $cur $_ '' }; Set-ItemProperty $cur '(default)' '' }

# delete files
$kill | % { Remove-Item -LiteralPath $_ -Force -ErrorAction SilentlyContinue }

# delete pack subfolders under Windows\Cursors (skip if a built-in file lives inside)
$kill | ? { $_ -like "$base\*\*" } | % { Join-Path $base $_.Substring($base.Length + 1).Split('\')[0] } | sort -Unique |
  ? { $d = $_; !($keep | ? { $_ -like "$d\*" }) } |
  % { Remove-Item -LiteralPath $_ -Recurse -Force -ErrorAction SilentlyContinue }

# delete custom scheme entries
$names | % { Remove-ItemProperty -Path $usrKey -Name $_ -ErrorAction SilentlyContinue }

# refresh cursors
if (!$WhatIfPreference) {
  Add-Type -Name W -Namespace N -MemberDefinition '[DllImport("user32.dll")] public static extern bool SystemParametersInfo(uint a,uint b,IntPtr c,uint d);'
  [N.W]::SystemParametersInfo(0x57,0,[IntPtr]::Zero,0) | Out-Null
}
