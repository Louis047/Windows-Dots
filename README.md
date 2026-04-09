# Windows-Dots

> [!NOTE]
> 1. Make sure to go through the readme entirely for a complete setup. You are open to choose according to your liking too.
> 2. `$USER` - Your User Name
> 3. If `$PROFILE` doesn't exist, use this command to create a profile for powershell `New-Item -Path $PROFILE -Type File -Force`
> 4. If the above command doesn't work, allow Powershell to sign remote scripts `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope LocalMachine` 
> 5. Make sure to create `.env` for YASB in its default director as some widgets rely on it for privacy and compatibility reasons. Documentation [here](https://github.com/amnweb/yasb/wiki/Configuration#environment-variables-support)


## Tools Used:
- [AltSnap](https://github.com/RamonUnch/AltSnap) - Resizing window with `Alt` Key, with ease
  
- [ExplorerPatcher](https://github.com/valinet/ExplorerPatcher) - A good customization tool for Windows 10/11 with extensible features

- [Everything](https://www.voidtools.com/) - Replacement for Windows Search Indexing. Fastest and Lightest. Using Alpha version. 

- [Fastfetch](https://github.com/fastfetch-cli/fastfetch) - A good CLI tool to fetch information about your system.

- [Flow Launcher](https://github.com/Flow-Launcher/Flow.Launcher) - Powetoys Run, but way better with plugins support

- [GlazeWM](https://github.com/glzr-io/glazewm) - i3-like window tiling manager, built using Rust

- [GAT-GWM](https://github.com/Dutch-Raptor/GAT-GWM) - Auto-tiler support for GlazeWM
  
- [MacType](https://github.com/snowie2000/mactype) - If you love MacOS and want better font rendering, this software is a heaven. Recently brought back from dead. Really recommend this. (even though their website is dead, good to see this project active in github)

- [Nilesoft Shell](https://github.com/moudey/Shell) - A great context menu customization tool with extensive features, allowing you to add your own context menu options with the help of scripts

- [SmoothScroll](https://www.smoothscroll.net/win/) - A paid software with trial period, improves smooth scrolling in Windows and MacOS

- [Tacky-Borders](https://github.com/lukeyou05/tacky-borders) - A cool Windows Border Customization without any compromise in resource usage

- [Thide](https://github.com/amnweb/thide) - Another goated tool by YASB dev, a lightweight tool to hide taskbar permanently

- [Windhawk](https://github.com/ramensoftware/windhawk) - Windows Customization made easy. Every user should try this

- [Yet Another Status Bar](https://github.com/amnweb/yasb) - A status bar with customizations and extensible features, built using Python

---
## Preview:

![PIC 1](https://github.com/Louis047/Windows-Dots/blob/main/Assets/Updated-Dots-1.png)

![PIC 2](https://github.com/Louis047/Windows-Dots/blob/main/Assets/Updated-Dots-2.png)

![PIC 3](https://github.com/Louis047/Windows-Dots/blob/main/Assets/Updated-Dots-3.png)

![PIC 4](https://github.com/Louis047/Windows-Dots/blob/main/Assets/Updated-Dots-4.png)

![PIC 5](https://github.com/Louis047/Windows-Dots/blob/main/Assets/Updated-Dots-5.png)

---
## Rice Setup Guide

TO-DO: Add Automation Script `install.ps1` with interactive TUI, not sure if it should be run as admin. Some cannot be done automatically as it requires manual intervention.

If you want to have more control over installing, follow the guide provided below. Almost everything is covered.

---
#### Discord
Paste the content from `no_rounded.css` from `Backups` to any Discord client that has QuickCSS support, `Vesktop` / `Equibop` recommended

---
#### ExplorerPatcher
Import the `reg` file from `Configurations\ExplorerPatcher` directory to the application to use simple alt+tab behavior 

---
#### Flow Launcher

Recommended Plugins
- AnyVideo Downloader
- Clipboard+
- Colors
- CurrencyPP
- DayNight Toggle
- Emoji+
- Env
- Google Meet Creator
- Lorem Ipsum Generator
- Nicknames Generator
- Speed Test
- StringUtils
- Temp Cleaner - If using `scoop` add this location to the Temp Folders list `C:\Users\$USER\scoop\cache`
- Uninstaller+
- Win Hotkey
- Windows Services Manager
- Windows Terminal Profiles

Changes to be made to the plugins manually:
- AnyVideo Downloader
  - Download Path: `%USERPROFILE%\Videos\Downloads`
- Clipboard+
  - Action Keyword: cp
  - List max records: 100
  - Uncheck Windows Clipboard-related options
  - Check Encrypt data in the database
  - Keep text: 1 hour
  - Keep Assets: 1 hour
  - Keep files: 1 hour 
- Emoji+
  - Action Keyword: :
- Explorer
  - General Settings
    - Uncheck 1st and 3rd option
    - Index Search Engine: Everything
    - Content Search Engine: Everything
    - Directory Recursive Search Engine: Everything
    - Maximum Results: 50
  - Everything Setting
    - Check both the options
- Plugins Manager
  - Check all options
- Program 
  - For optimal search with correct results, follow the options below
    - Index Sources: Tick `UWP Apps`, `Start Menu`, `Registry`
    - Options: Tick `Hide App Path`, `Hide Uninstallers`, `Hide Duplicated Apps`
- Shell
  - Check only Always run as administrator option
- Temp Cleaner
  - Add the following directories, separated by `;`
    - `%LOCALAPPDATA%\Temp`
    - `C:\Windows\Prefetch`
    - `%USERPROFILE%\scoop\cache` (if using scoop)
    - [MORE TO BE ADDED]
- Web Searches 
  - Change action keyword for google from `*` to `gg`
  - Max Suggestions: 5
  - Check Use Search Query Autocomplete
  - Autocomplete Data from: Google
- Win Hotkey
  - Maximum press time to trigger flow (ms): Leave Blank

To apply the theme, navigate to `Configurations\FlowLauncher` directory where it contains the necessary steps documented as Assets

---
#### GlazeWM Autotiler
To install this, you have to install `rustup` and `cargo` then use the below command
```
cargo install --git https://github.com/Dutch-Raptor/GAT-GWM.git --features=no_console
```
Configuration uses this already so works out-of-the-box. Make sure to install this first before using GlazeWM.

---
#### Nilesoft Shell
- Copy `theme.nss` and `shell.nss`
- Paste in the directory where the app is present. Usually in `C:\Program Files\Nilesoft Shell`

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
#### Tasks-Scripts
Made changes from the base `AltDrag` config to be optimal for both desktop and laptop users, made for the below list:
- AltSnap
- Block-WinTab
- GlazeWM
- SmoothScroll (Both Cleanup and Launch)
- Tacky-Borders

All run with the highest privileges to ensure the modifications cover all apps by default. 

Save the Scripts in the following directory: `Documents\VBS-Scripts`

---
#### Thide
Open Powershell and type this command
```
thide enable-autostart
```
This adds thide to startup to hide taskbar permanently everytime, runs in tray

---
#### Windhawk
Some useful mods I found for my Windows workflow
- "Sign in with a passkey" Blocker - add `zen.exe` to process exclusion list
- Alt + Tab per monitor
- Auto Custom Titlebar Colors
- Better Dialogs
- Better file sizes in Explorer details
- Block Start Menu and Hosts
- Classic Explorer navigation bar (Only for Windows 11)
- Context Menu Preloader
- CTRL+SHIFT+C Quotes remover - really handy
- Customize Windows Notification Placement
- Dark Mode for Notepad
- Dark Paint
- Disable rounded corners in Windows 11 (Only for Windows 11)
- Disable Windows Ink Modifier Tooltips
- Disable Windows Shortcuts (Local Mod)
- F1 Blocker
- Fix Basic Caption Text
- Fix Darkmode ListViews
- Hide Home,Gallery & OneDrive in Explorer
- Legacy Shell Message Boxes
- Message Box Fix
- No Focus Rectangle
- No Properties Icon
- No Run Icon
- Notepad Multi-Select
- Old Auto Colorization
- RegEdit Auto Trim Whitespace
- Resource Redirect
- Select filename extension on double F2
- Shell Animation Disabler
- Show All Apps by Default in Start Menu (Only for Windows 11)
- Shrink Address Bar Height
- Smart Copy and Paste
- Smooth Scroll for Win32
- Start Menu open location 
- Start Menu Size 
- Taskbar Height and Icon Size
- Taskbar on top (Only for Windows 11)
- Taskbar Tray System Icon Tweaks (Only for Windows 11)
- Turn off change file extension warning
- UXTheme Hook (For Implementing Windows 10 theme for 11. Theme Link: [10ThemesFor11](https://github.com/SandTechStuff/10ThemeFor11)) (Only for Windows 11)
- VSCode Tweaker
- Windhawk UI Tweaker (Fork of VSCode Tweaker)
- Windows 11 Old Task Manager (Only for Windows 11)
- Windows 11 Notification Center Styler (Only for Windows 11)
- Windows 11 Start Menu Styler (Only for Windows 11)) 
- Windows 11 Taskbar Styler (Only for Windows 11)
- Windows 7 Command Bar
- Word PDF Lossless Export

For square corners in other windows elements (Action Center, Flyouts, Start Menu), copy the respective file content from `Configurations\Windhawk\mod-configs` directory. Paste them in respective mods in Settings set as Textual mode for easy config application. 

Below are the mods associated with the configs
- Action Center, Toast Notifications and Notification Center - Windows 11 Notification Center Styler
- Volume and Brightness Flyouts - Windows 11 Taskbar Styler
- Start Menu and Lock Screen - Windows 11 Start Menu Styler
- File explorer - Windows 11 File Explorer Styler

Changes to be made for the mods manually
- Alt+Tab per monitor
  - Display Location: Where cursor is located
  - Windows to show: Windows from the monitor wheere cursor is located
- Auto Custom Titlebar Colors
  - Add the following processes to custom exclusion list
    - notepad.exe (If using UWP version)
    - Taskmgr.exe (If using UWP version)
    - zen.exe
- Better Dialogs
  - Replace Message Boxes with Task Dialogs: On
  - Use modern folder picker dialog: On 
- Better file sizes in Explorer details
  - Show folder sizes: Enabled via `Everything` integration (Install Everything Alpha for best performance)
  - Mix files and folders when sorting by size: On
  - Use MB/GB for large files: On
- Block Start menu and Hosts
  - All On
- Classic Explorer navigation bar
  - Explorer style: Classic ribbon UI (no tabs)
- Customize Windows Notification Placement
  - Monitor: 0
  - Horizontal placement: Right
  - Distance from right/left: 4
  - Vertical placement: Top
  - Distance from bottom/top: -14
  - Notification appearance animation: Automatic
- Fix Basic Caption Text
  - Small window icons: On
- Message Box Fix
  - Message box style: Windows 7-10 1703
- No Properties Icon
  - Remove window icon from all property sheets: On
- Old Auto Colorization
  - Auto colorization mode: Windows 10 1507-1803
- Resource Redirect
  - Icon theme: Paper (by niivu)
- Shell Animation Disabler
  - All On
- Smart Copy and Paste
  - Remove Tracking Parameters
  - Auto-trim whitespace
- Smooth Scroll for Win32
  - V-Sync: On
- Start menu open location
  - Monitor: 0
- Start Menu Size
  - Start menu width: 450
  - Start menu height: 650
- "Sign in with a passkey" Blocker 
  - add `zen.exe` to process exclusion list
- Taskbar Height and Icon Size 
  - Set taskbar height and all other elements to `-1`. Then use `thide` to completely hide it. This is to ensure taskbar isn't visible anymore at any time.
- Taskbar Tray System Icon Tweaks 
  - All On
- VSCode Tweaker
  - Add `Antigravity.exe` to custom process inclusion list
  - Code Snippets
    - Snippet Type: Javascript
    - Snippet Source: Inline code
    - Snippet Code: Inside `Configurations\VSCode` directory, paste it in the input box
- Windhawk UI Tweaker
  - Fork VSCode Tweaker and do the following to the code
    - Replace `Code.exe` with `VSCodium.exe`
    - Code Snippets
      - Snippet Type: Electron main.js
      - Snippet Source: Inline code
      - Snippet Code: Inside `Configurations\VSCode` directory, paste it in the input box
  - Forking VSCode Tweaker Mod is necessary as from my testing, adding `VSCodium.exe` to VSCode Tweaker shows a warning everytime but works. It's annoying, this is the better way.

---
#### Windows Terminal
1. In Settings, go to Rendering section and change Graphics API to `Direct2D` to fix font rendering issues
2. Use `Dark+` color scheme for best colors
3. For `micro`
   - Open Command Bar with `Ctrl+e`
   - Type `set colorscheme geany`
   - Restart

---
#### YASB
1. To use wallpaper gallery, save them here: `$env:USERPROFILE/Pictures/Wallpapers/Home Screen` 
2. For `clock` widget, the country code uses `YASB_CAL_COUNTRY_CODE` variable. Make sure to set a country code in your `.env` file. Check docs [here](https://docs.yasb.dev/latest/configuration)

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
3. Paste the zen browser snippets from `Configurations\ZenBrowser\Hard Backup` directory to your current profile (You can find you current active profile from `about:profiles`)
    - `<profile>\chrome\userChrome.css`
    - `<profile>\chrome\userContent.css`

> [!NOTE]
> `userChome.css` - For browser user-interface customization
> 
> `userContent.css` - For customizing inner elements of the browser
> 
> To open the current profile folder, do the following steps
> 1. Open `about:profiles`
> 2. Open the profile folder which is currently active
> 3. Follow the above third step

The above steps are purely for customizing the browser manually. I have worked on a sine mod called [`Square UI`](https://github.com/Louis047/Square-UI) for eliminating rounded corners in both the browser and the websites rendered without the need of extensions, and have all the configurations combined and automated. Also has `Custom uiFont` mod integrated.

You can install Sine Mods [here](https://github.com/CosmoCreeper/Sine/releases/latest)

Recommended Mods

If using Sine Mods
- Square UI (Highly recommended if you don't want to add the customizations manually)

If not using Sine Mods
- Custom uiFont (If configured manually)
- Left Side Glance
- No Sidebar Scrollbar
- Remove Tab X - set for pinned tabs only
