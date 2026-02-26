# Windows-Dots

> [!NOTE]
> 1. Make sure to go through the readme entirely for a complete setup. You are open to choose according to your liking too.
> 2. `$USER` - Your User Name
> 3. If `$PROFILE` doesn't exist, use this command to create a profile for powershell `New-Item -Path $PROFILE -Type File -Force`
> 4. If the above command doesn't work, allow Powershell to sign remote scripts `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope LocalMachine` 
> 5. Make sure to create `.env` for YASB in its default director as some widgets like `weather` rely on it. Documentation [here](https://github.com/amnweb/yasb/wiki/Configuration#environment-variables-support)

### Tools Used:
- [Yet Another Status Bar](https://github.com/amnweb/yasb) - A status bar with customizations, built using Python.
  
- [GlazeWM](https://github.com/glzr-io/glazewm) - i3-like window tiling manager, built using Rust.

- [GlazeWM Autotiler](https://github.com/Dutch-Raptor/GAT-GWM) - Implements a missing feature to GlazeWM: `Tiling Layouts`. Soon to be deprecated as the functionality is about to be merged in the next update.
  
- [Komorebi](https://github.com/LGUG2Z/komorebi) - Another Tiling Manager made using Rust, with goodies like animations and better transparency support. Sure a hell to customize this.

- [AltSnap](https://github.com/RamonUnch/AltSnap) - Resizing window with `Alt` Key, with ease.

- [Nilesoft Shell](https://github.com/moudey/Shell) - A great context menu customization tool with extensive features, allowing you to add your own context menu options with the help of scripts
  
- [Flow Launcher](https://github.com/Flow-Launcher/Flow.Launcher) - Powetoys Run, but better. (Microsoft Ruined it with their new trash: `Command Palette`)
  
- [WindHawk](https://github.com/ramensoftware/windhawk) - Windows Customization made easy. Every user should try this.
  
- [Smooth-Scroll](https://www.smoothscroll.net/win/) - System-Level implementation for buttery-smooth scrolling. Sad that it's paid but has a free trial upto 21 days. I do appreciate the developer's work. Might as well support him by buying a license with my first salary.

- [Fastfetch](https://github.com/fastfetch-cli/fastfetch) - A good CLI tool to fetch information about your system.

- [MacType](https://github.com/snowie2000/mactype) - If you love MacOS and want better font rendering, this software is a heaven. Recently brought back from dead. Really recommend this. (even though their website is dead, good to see this project active in github)

- [Tacky-Borders](https://github.com/lukeyou05/tacky-borders) - A cool Windows Border Customization without any compromise in resource usage. An Accidental discovery. Komorebi has this integrated.

- [Thide](https://github.com/amnweb/thide) - Another goated tool by YASB dev, a lightweight tool to hide taskbar permanently

### Preview:
GlazeWM Setup

![PIC 1](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-1.png)

![PIC 2](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-2.png)

![PIC 3](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-3.png)

Komorebi Setup

![PIC 4](https://github.com/Louis047/Windows-Dots/blob/main/Images/Screenshot%202025-04-26%20222210.png)

![PIC 5](https://github.com/Louis047/Windows-Dots/blob/main/Images/Screenshot%202025-04-26%20222339.png)


### Rice Setup Guide

#### Discord QuickCSS
Paste the content from `no_rounded.css` to any Discord client that has QuickCSS support

#### ExplorerPatcher
Import the `reg` file from the directory to the application to use simple alt+tab behavior 


#### Flow Launcher
For an optimal search, make changes to the following plugins
- Web Searches - Change action keyword for google from `*` to `gg`
- Program - Index Sources: Tick `UWP Apps`, `Start Menu`, `Registry`; Options: Tick `Hide App Path`, `Hide Uninstallers`, `Hide Duplicated Apps`

Recommended Plugins
- AnyVideo Downloader
- Clipboard+
- Emoji+
- Temp Cleaner - If using `scoop` add this location to the Temp Folders list `C:\Users\$USER\scoop\cache`
- Windows Terminal Profiles 

To apply the theme, navigate to `FlowLauncher` directory where it contains the necessary steps documented as images.

#### GlazeWM Autotiler
To install this, you have to install `rustup` and `cargo` then use the below command
```
cargo install --git https://github.com/Dutch-Raptor/GAT-GWM.git --features=no_console
```
Have already done the required configurations for it in GlazeWM


#### Nilesoft Shell
Paste the config into the app directory. Make sure to open them as admin. 


#### Oh My Posh
Use `robbyrussel` theme for minimalism

Before that, create a directory for `$POSH_THEMES_PATH`
```
mkdir $env:POSH_THEMES_PATH -Force
```

Now to apply it, add the following command to `$PROFILE`
```
if (Get-Command oh-my-posh -ErrorAction SilentlyContinue) {
	oh-my-posh init pwsh --config "$env:POSH_THEMES_PATH\robbyrussell.omp.json" | Invoke-Expression
}
```

> [!NOTE]
> I recommend to install `oh-my-posh` with `scoop` package manager as the configuration is provided according to that. You can test with other type of installation and let me know of any issues!


#### Task Scheduler
Made changes from the base `AltDrag` config to be optimal for both desktop and laptop users, `tacky-borders` included


#### Thide
Open Powershell and type this command
```
thide enable-autostart
```
This adds thide to startup to hide taskbar permanently everytime, runs in tray


#### Windhawk
Some useful mods I found for my Windows workflow
- Disable rounded corners in Windows 11 - I hate rounded corners with a square monitors so yes 
- Windows 11 Notification Center Styler - Use this to change border to square for and `Action Center` and `Toast Notifications`
- Windows 11 Start Menu Styler - To remove square borders for it as well 
- Windows 11 Taskbar Styler - To make taskbar transparent and configure border for `Flyouts`
- Taskbar Height and Icon Size - For the above
- Center Titlebar
- Dark Mode Context Menus
- Explorer Font Changer - Very useful to change fonts system-wide(almost). I prefer `Segoe UI Variable Display`
- No Focus Rectangle
- Taskbar Tray System Icon Tweaks

For square corners in other windows elements (Action Center, Flyouts, Start Menu), copy the respective file content from `Windhawk` directory and paste it in Advanced Settings --> Mod Settings in the following mods
- Action Center - Windows 11 Notification Center Styler
- Volume and Brightness Flyouts - Windows 11 Taskbar Styler
- Start Menu and other elements - Windows 11 Start Menu Styler

Changes to be made from some mods
- Taskbar Height and Icon Size - Set taskbar height and all other elements to `-1`
- Taskbar Tray System Icon Tweaks - Hide everything


#### Windows Terminal
In Settings, go to Rendering section and change Graphics API to `Direct2D` to fix font rendering issues


#### YASB
1. To use wallpaper widget, save them in this location `C:\Users\$USER\Pictures\Wallpapers\Home Screen` (path with double backslashes)
2. Most of the widgets are customized for you, and YASB has the ability to choose your own widgets on runtime


#### Zen Browser
Some changes in settings required for a good tiling experience

Type `about:config` in the search bar and change the following configurations
1. `zen.theme.content-element-separation` : 0 (You may prefer to keep this default if you love padding)
2. `zen.view.experimental-no-window-controls` : true 
3. Install Sine Mods from [here](https://github.com/CosmoCreeper/Sine) (required for extras, make sure to install pre-release version)

Recommended Mods
- Disable Rounded Corners
- No Sidebar Scrollbar 
- No-Gaps
- Zen Custom URL Bar (for sqaure corners)
- Deta Loading Bar (for good loading bar)
- Tidy Downloads
- Floating Statusbar and Findbar
- Zen Drop Link to Split
- New Icons
- Context Menu Icons
- Custom Swipe Animation & Color
