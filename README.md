# Windows-Dots

> [!NOTE]
> 1. Make sure to go through the readme entirely for a complete setup. You are open to choose according to your liking too.
> 2. `$USER` - Your User Name
> 3. If `$PROFILE` doesn't exist, use this command to create a profile for powershell `New-Item -Path $PROFILE -Type File -Force`
> 4. If the above command doesn't work, allow Powershell to sign remote scripts `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope LocalMachine` 
> 5. Make sure to create `.env` for YASB in its default director as some widgets like `weather` rely on it. Documentation [here](https://github.com/amnweb/yasb/wiki/Configuration#environment-variables-support)


### Tools Used:
- [AltSnap](https://github.com/RamonUnch/AltSnap) - Resizing window with `Alt` Key, with ease
  
- [ExplorerPatcher](https://github.com/valinet/ExplorerPatcher) - A good customization tool for Windows 10/11 with extensible features

- [Fastfetch](https://github.com/fastfetch-cli/fastfetch) - A good CLI tool to fetch information about your system.

- [Flow Launcher](https://github.com/Flow-Launcher/Flow.Launcher) - Powetoys Run, but way better

- [GlazeWM](https://github.com/glzr-io/glazewm) - i3-like window tiling manager, built using Rust

- [GlazeWM Autotiler](https://github.com/Dutch-Raptor/GAT-GWM) - Implements automatic tiling to GlazeWM
  
- [MacType](https://github.com/snowie2000/mactype) - If you love MacOS and want better font rendering, this software is a heaven. Recently brought back from dead. Really recommend this. (even though their website is dead, good to see this project active in github)

- [Nilesoft Shell](https://github.com/moudey/Shell) - A great context menu customization tool with extensive features, allowing you to add your own context menu options with the help of scripts

- [Smooth-Scroll](https://www.smoothscroll.net/win/) - A paid software with trial period, improves smooth scrolling in Windows and MacOS

- [Tacky-Borders](https://github.com/lukeyou05/tacky-borders) - A cool Windows Border Customization without any compromise in resource usage

- [Thide](https://github.com/amnweb/thide) - Another goated tool by YASB dev, a lightweight tool to hide taskbar permanently

- [WindHawk](https://github.com/ramensoftware/windhawk) - Windows Customization made easy. Every user should try this

- [Yet Another Status Bar](https://github.com/amnweb/yasb) - A status bar with customizations and extensible features, built using Python

---
### Preview:

![PIC 1](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-1.png)

![PIC 2](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-2.png)

![PIC 3](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-3.png)

![PIC 4](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-4.png)

![PIC 5](https://github.com/Louis047/Windows-Dots/blob/main/Images/Updated-Dots-5.png)

---
### Rice Setup Guide

#### Discord QuickCSS
Paste the content from `no_rounded.css` to any Discord client that has QuickCSS support


---
#### ExplorerPatcher
Import the `reg` file from the directory to the application to use simple alt+tab behavior 


---
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

To apply the theme, navigate to `FlowLauncher` directory where it contains the necessary steps documented as images (STILL NEED TO REFINE IT TO SYNCHRONIZE IT WITH WINDOWS THEME MODE)


---
#### GlazeWM Autotiler
To install this, you have to install `rustup` and `cargo` then use the below command
```
cargo install --git https://github.com/Dutch-Raptor/GAT-GWM.git --features=no_console
```
Have already done the required configurations for it in GlazeWM


---
#### Nilesoft Shell
Paste the config into the app directory. Make sure to open them as admin. 


---
#### Oh My Posh
Use `robbyrussel` theme for minimalism

Before that, create a directory for `$POSH_THEMES_PATH`
```
mkdir $env:POSH_THEMES_PATH -Force
```

Now to apply it, add the following command to `$PROFILE`
```powershell
if (Get-Command oh-my-posh -ErrorAction SilentlyContinue) {
	oh-my-posh init pwsh --config "$env:POSH_THEMES_PATH\robbyrussell.omp.json" | Invoke-Expression
}
```

> [!NOTE]
> I recommend to install `oh-my-posh` with `scoop` package manager as the configuration is provided according to that. You can test with other type of installation and let me know of any issues!


---
#### Task Scheduler
Made changes from the base `AltDrag` config to be optimal for both desktop and laptop users, `tacky-borders` included


---
#### Thide
Open Powershell and type this command
```
thide enable-autostart
```
This adds thide to startup to hide taskbar permanently everytime, runs in tray


---
#### VSCode
1. Install `Custom CSS and JS Loader` extension for VSCode
2. Open Command Palette `Ctrl + Shift + P` then type: Open User Settings (JSON)
3. Paste the below JSON snippet and save it
```json
"vscode_custom_css.imports": [
	        "file:///${env:USERPROFILE}/Documents/VSCode-CSS/no_rounded.css"
]
```
4. Create a directory named `VSCode-CSS` in the provided path in the command below or you can just create it manually via file explorer
```
mkdir -p '$env:USERPROFILE/Documents/VSCode-CSS'
```
5. Paste the `no_rounded.css` from `CSS-Snippets` to the created directory
6. Open Command Palette again and type: Enable Custom CSS and JS Loader then restart VS Code once the prompt shows successful (ignore any warning/errors)
7. You will get a prompt saying 'VSCode is corrupted' ignore it and hide it permanently.


---
#### Windhawk
Some useful mods I found for my Windows workflow
- "Sign in with a passkey" Blocker - add `zen.exe` to process exclusion list
- Alt + Tab per monitor
- Block Win + V - I prefer Flow Launcher clipboard
- Center Titlebar
- Control Panel Color Fix
- CTRL+SHIFT+C Quotes remover - really handy
- Customize Windows Notification Placement
- Dark Mode Context Menus
- Dark Mode for Notepad
- Disable rounded corners in Windows 11 - I hate rounded corners with a square monitors so yes 
- Disable Virtual Desktop Transition Animation
- Disable Voice Access Hotkey
- Disable Windows Ink Modifier Tooltips
- Explorer breadcrumbs Middle-Click New Tab
- Explorer Font Changer - Very useful to change fonts system-wide(almost). I prefer `Inter Display`
- F1 Blocker
- Fix dark mode ListViews
- Fix white flashes in explorer
- Message Box Fix
- No Focus Rectangle
- No Properties Icon
- No Run Icon
- Old Auto Colorization
- Primary Taskbar on Second Monitor
- RegEdit Auto Trim Whitespace
- Select filename extension on double F2
- Show All Apps by Default in Start Menu
- Smart Copy and Paste
- Start Menu open location - Monitor 0
- Start Menu Size - Width: 450 Height: 650
- Taskbar Height and Icon Size - For the above
- Taskbar on top
- Taskbar Tray System Icon Tweaks - Toggle everything on
- Turn off change file extension warning
- UXTheme Hook (For Implementing Windows 10 theme for 11. Theme Link: [10ThemesFor11](https://github.com/SandTechStuff/10ThemeFor11))
- Windows 11 Custom Title Bar Colors
- Windows 11 File Explorer Styler - To remove rounded borders (still buggy and yet to fix)
- Windows 11 Notification Center Styler - Use this to change border to square for and `Action Center` and `Toast Notifications`
- Windows 11 Start Menu Styler - To remove rounded borders for it as well 
- Windows 11 Taskbar Styler - To make taskbar transparent and configure border for `Flyouts`

For square corners in other windows elements (Action Center, Flyouts, Start Menu), copy the respective file content from `Windhawk` directory and paste it in Advanced Settings --> Mod Settings in the following mods
- Action Center - Windows 11 Notification Center Styler
- Volume and Brightness Flyouts - Windows 11 Taskbar Styler
- Start Menu and other elements - Windows 11 Start Menu Styler
- File explorer - Windows 11 File Explorer Styler

Changes to be made from some mods
- Customize Windows Notification Placement - Use the following settings
  - Monitor: 0
  - Horizontal placement: Right
  - Distance from right/left: -8
  - Vertical placement: Top
  - Distance from bottom/top: -22
  - Notification appearance animation: Automatic
- Smart Copy and Paste - Toggle the following options enabled
  - Remove Tracking Parameters
  - Auto-trim whitespace
- "Sign in with a passkey" Blocker - add `zen.exe` to process exclusion list
- Taskbar Height and Icon Size - Set taskbar height and all other elements to `-1`. Then use `thide` to completely hide it. This is to ensure taskbar isn't visible anymore at any time.
- Taskbar Tray System Icon Tweaks - Hide everything
- Windows 11 Custom Title Bar Colors - For this make sure to add the following process in exclusion list in Advanced tab
  - notepad.exe
  - Taskmgr.exe
- Windows 11 File Explorer Styler - Set Translucent background effect to `None` to utilize Windows 11 Custom Title Bar Colors mod color.



---
#### Windows Terminal
In Settings, go to Rendering section and change Graphics API to `Direct2D` to fix font rendering issues


---
#### YASB
1. To use wallpaper widget, save them in this location `$env:USERPROFILE/Pictures/Wallpapers/Home Screen` 
2. Most of the widgets are customized for you, and YASB has the ability to choose your own widgets on runtime via the context menu in bar
3. For `clock` widget, the country code uses `YASB_CAL_COUNTRY_CODE` variable. Make sure to set a country code in your `.env` file. Check docs [here](https://docs.yasb.dev/latest/configuration)


---
#### Zen Browser
Some changes in settings required for a good tiling experience

1. Type `about:config` in the search bar and change the following configurations
  - `zen.theme.content-element-separation` : 0 (You may prefer to keep this default if you love padding)
  - `zen.view.experimental-no-window-controls` : true 
  - `zen.theme.essentials-favicon-bg` : false (for proper tab bar animations in essentials)
  - `zen.theme.border-radius` : 0 
  - `extensions.webextensions.restrictedDomains` : 	accounts-static.cdn.mozilla.net,addons.cdn.mozilla.net,api.accounts.firefox.com,content.cdn.mozilla.net,discovery.addons.mozilla.org,oauth.accounts.firefox.com,profile.accounts.firefox.com,sync.services.mozilla.com (to ensure stylus includes mozilla related websites as well, except for other restricted services)
  - `widget.prefer_windows_on_current_virtual_desktop` : false (to fix url links opening in new window when using windows with tiling window managers)
2. Follow the guide here to utilize `userChrome.css` and `userContent.css`: [Live Editing Zen Theme](https://docs.zen-browser.app/guides/live-editing#step-4-edit-the-userchromecss-file)
3. Paste the zen browser snippets in the `chrome` directory of your current profile
    - `<profile>\chrome\userChrome.css`
    - `<profile>\chrome\userContent.css`

> [!NOTE]
> `userChome.css` - For browser user-interface customization
> `userContent.css` - For customizing inner elements of the browser
> To open the current profile folder, do the following steps
> 1. Open `about:profiles`
> 2. Open the profile folder which is currently active
> 3. Follow the above third step

The above steps are purely for customizing zen browser manually. I have worked on a Sine Mod called [`Square UI`](https://github.com/Louis047/Square-UI) for eliminating rounded corners in both the browser and the websites rendered without the need of extensions, and have all the configurations combined and automated. Also has `Custom uiFont` mod integrated.

Recommended Mods
- Square UI (Highlhy recommended if you don't want to add the customizations manually)
- Custom uiFont
- Left Side Glance
- No Sidebar Scrollbar
- Private Mode Highlighting
- Remove Tab X - set for pinned tabs only

