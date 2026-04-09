Import-Module Terminal-Icons
fastfetch

if (Get-Command oh-my-posh -ErrorAction SilentlyContinue) {
	oh-my-posh init pwsh --config "$env:POSH_THEMES_PATH\robbyrussell.omp.json" | Invoke-Expression
}

$Env:KOMOREBI_CONFIG_HOME = "$Env:USERPROFILE\.config\komorebi"