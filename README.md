# Windows-Dots

> [!NOTE]
> Make sure to go through the readme entirely for a complete setup. You are open to choose according to your liking too.

### Tools Used:
- [Yet Another Status Bar](https://github.com/amnweb/yasb) - a status bar with customizations, built using Python.
  
- [GlazeWM](https://github.com/glzr-io/glazewm) - i3-like window tiling manager, built using Rust.

- [GlazeWM Autotiler](https://github.com/Dutch-Raptor/GAT-GWM) - Implements a missing feature to GlazeWM: `Tiling Layouts`. Soon to be deprecated as the functionality is about to be merged in the next update.
  
- [Komorebi](https://github.com/LGUG2Z/komorebi) - Another Tiling Manager made using Rust, with goodies like animations and better transparency support. Sure a hell to customize this.

- [AltSnap](https://github.com/RamonUnch/AltSnap) - Resizing window with `Alt` Key, with ease.
  
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


### Setup To Follow:

#### Windhawk
Some useful mods I found for my Windows workflow
- Disable rounded corners in Windows 11 - I hate rounded corners with a square monitors so yes 
- Windows 11 Notification Center Styler - Use this to change border to square for both `Action Center`, `Notification Center` and `Flyouts`
- Windows 11 Start Menu Styler - Again, to remove square borders (don't use it anymore)
- Windows 11 Taskbar Styler - As the name says, to make it compact
- Taskbar Height and Icon Size - For the above
- Center Titlebar
- Dark Mode Context Menus
- Explorer Font Changer - Very useful to change fonts system-wide (I prefer `Segoe UI Variable Display`)
- No Focus Rectangle

#### YASB
1. To use wallpaper widget, save them in this location `C:\Users\$USER\Pictures\Wallpapers\Home Screen`
2. Most of the widgets are customized for you, and YASB has the ability to choose your own widgets on runtime
3. Make sure to change your location in the weather widget

#### Task Scheduler
Made changes from the base `AltDrag` config to be optimal for both desktop and laptop users, `tacky-borders` included

#### Thide
Open Powershell and type this command
```
thide enable-autostart
```
This adds thide to startup to hide taskbar permanently everytime, runs in tray

#### Windhawk
For square corners, copy the file content from `Windhawk` directory and paste it in Advanced Settings --> Mod Settings in `Windows 11 Notification Center Styler` mod

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

#### GlazeWM Autotiler
To install this, you have to install `rustup` and `cargo` then use the below command
```
cargo install --git https://github.com/Dutch-Raptor/GAT-GWM.git --features=no_console
```
Have already done the required configurations for it in GlazeWM