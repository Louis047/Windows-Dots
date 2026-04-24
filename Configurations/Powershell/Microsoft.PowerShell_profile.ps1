fastfetch
$ProgressPreference = 'SilentlyContinue'

if (Get-Command oh-my-posh -ErrorAction SilentlyContinue) {
	oh-my-posh init pwsh --config "$env:POSH_THEMES_PATH\robbyrussell.omp.json" | Invoke-Expression *> $null
}

Set-Alias -Name ff -Value fastfetch -Force

function micro { & micro.exe @args; clear }

if (Get-Alias ls -ErrorAction SilentlyContinue) {
	Remove-Item Alias:ls -Force
}

function ls {
    eza --group-directories-first --color=auto --icons=never @args
}

function ll {
    eza -l --group-directories-first --color=auto --icons=never @args
}

function la {
    eza -a --group-directories-first --color=auto --icons=never @args
}

function rr {
    Remove-Item -Recurse @args
}

function rmf {
    Remove-Item -Force @args
}

function rmrf {
    Remove-Item -Recurse -Force @args
}

function ~ { Set-Location $HOME }
function .. { Set-Location .. }
function .... { Set-Location ..\.. }
function ...... { Set-Location ..\..\.. }
function ........ { Set-Location ..\..\..\.. }