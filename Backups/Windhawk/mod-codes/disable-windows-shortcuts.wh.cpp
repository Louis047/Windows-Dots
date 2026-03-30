// ==WindhawkMod==
// @id              disable-windows-shortcuts
// @name            Disable Windows Shortcuts
// @description     Block individual built-in Windows shortcut keys, each with its own toggle.
// @version         1.1.0
// @author          Lone
// @github          https://github.com/Louis047
// @include         explorer.exe
// @compilerOptions -lcomctl32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Disable Windows Shortcuts

Block any combination of Windows built-in global shortcut keys.
Each shortcut has its own toggle in the **Settings** tab.

## Supported Shortcuts

| Setting | Shortcut | Action |
|---------|----------|--------|
| Win+A | Action Center | Notification / quick-settings panel |
| Win+B | Focus Taskbar | Focus the notification area |
| Win+C | Copilot / Chat | Open Copilot (Windows 11) |
| Win+D | Show Desktop | Toggle desktop visibility |
| Win+E | File Explorer | Open File Explorer |
| Win+F | Feedback Hub | Open Feedback Hub |
| Win+G | Game Bar | Open Xbox Game Bar |
| Win+H | Voice Typing | Open voice typing |
| Win+I | Settings | Open Windows Settings |
| Win+K | Cast / Connect | Open wireless display panel |
| Win+M | Minimize All | Minimize all open windows |
| Win+N | Notifications | Open notification center (Windows 11) |
| Win+O | Orientation Lock | Lock screen orientation (tablets) |
| Win+P | Project | Open projection / display mode picker |
| Win+Q | Search | Open Windows Search (alias) |
| Win+R | Run Dialog | Open the Run dialog |
| Win+S | Search | Open Windows Search |
| Win+T | Cycle Taskbar | Cycle focus through taskbar apps |
| Win+U | Accessibility | Open Accessibility settings |
| Win+V | Clipboard History | Open clipboard history |
| Win+W | Widgets | Open Widgets board (Windows 11) |
| Win+X | Quick Link Menu | Open power-user context menu |
| Win+Z | Snap Layouts | Open snap layout picker (Windows 11) |
| Win+Tab | Task View | Open virtual desktop / task view |
| Win+Space | Input Language | Switch input language / keyboard layout |
| Win+Enter | Narrator | Launch Narrator |
| Win+Home | Minimize Others | Minimize all windows except active |
| Win+Pause | System Properties | Open System Properties |
| Win+. | Emoji Panel | Open emoji / symbol picker |
| Win+Up | Snap Up | Maximize / snap window up |
| Win+Down | Snap Down | Restore or snap window down |
| Win+Left | Snap Left | Snap window to left half |
| Win+Right | Snap Right | Snap window to right half |
| Win+PrtScn | Screenshot | Save screenshot to Pictures |
| Win+Shift+S | Snipping Tool | Open snipping / screenshot tool |
| Win+Shift+Left | Move Left Monitor | Move window to left monitor |
| Win+Shift+Right | Move Right Monitor | Move window to right monitor |
| Win+Shift+Up | Stretch Vertical | Maximize window vertically |
| Win+Shift+Down | Restore Vertical | Restore vertical size |
| Win+Shift+M | Restore All | Restore minimized windows |
| Win+Ctrl+D | New Virtual Desktop | Create a new virtual desktop |
| Win+Ctrl+Left | Previous Desktop | Switch to previous virtual desktop |
| Win+Ctrl+Right | Next Desktop | Switch to next virtual desktop |
| Win+Ctrl+F4 | Close Desktop | Close current virtual desktop |
| Win+Ctrl+Enter | Narrator (Alt) | Alternative Narrator launch |
| Win+Ctrl+C | Color Filters | Toggle color filters |
| Win+Ctrl+Q | Quick Assist | Open Quick Assist |
| Win+Alt+D | Date / Time | Show date and time flyout |
| Win+Alt+G | Xbox Record | Record last 30 seconds (Xbox) |
| Win+Alt+R | Xbox Capture | Start / stop recording (Xbox) |
| Win+Ctrl+Shift+B | Reset Display | Reset display / graphics driver |

> **Note:** `Win+L` (Lock Screen) is owned by `winlogon.exe` and cannot be intercepted from `explorer.exe`.

## How It Works

Injects into `explorer.exe` via three mechanisms:

1. **Hooks `RegisterHotKey`** — fakes registration for blocked shortcuts so Explorer doesn't retry, but never claims the hotkey, so `WM_HOTKEY` never fires.
2. **Hooks `GetMessageW` / `PeekMessageW`** — swallows `WM_HOTKEY` events for shortcuts already registered before the mod loaded.
3. **Subclasses shell windows** (`Progman`, `Shell_TrayWnd`) — catches HWND-targeted hotkey registrations.

Settings changes take effect immediately. To restore a re-enabled shortcut, restart Explorer via Task Manager.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- win_a: false
  $name: Win+A — Action Center
- win_b: false
  $name: Win+B — Focus Taskbar
- win_c: false
  $name: Win+C — Copilot / Chat
- win_d: false
  $name: Win+D — Show Desktop
- win_e: false
  $name: Win+E — File Explorer
- win_f: false
  $name: Win+F — Feedback Hub
- win_g: false
  $name: Win+G — Game Bar
- win_h: false
  $name: Win+H — Voice Typing
- win_i: false
  $name: Win+I — Settings
- win_k: false
  $name: Win+K — Cast / Connect
- win_m: false
  $name: Win+M — Minimize All
- win_n: false
  $name: Win+N — Notifications
- win_o: false
  $name: Win+O — Orientation Lock
- win_p: false
  $name: Win+P — Project / Display
- win_q: false
  $name: Win+Q — Search (alias)
- win_r: false
  $name: Win+R — Run Dialog
- win_s: false
  $name: Win+S — Search
- win_t: false
  $name: Win+T — Cycle Taskbar
- win_u: false
  $name: Win+U — Accessibility
- win_v: false
  $name: Win+V — Clipboard History
- win_w: false
  $name: Win+W — Widgets
- win_x: false
  $name: Win+X — Quick Link Menu
- win_z: false
  $name: Win+Z — Snap Layouts
- win_tab: false
  $name: Win+Tab — Task View
- win_space: false
  $name: Win+Space — Switch Input Language
- win_enter: false
  $name: Win+Enter — Narrator
- win_home: false
  $name: Win+Home — Minimize Others
- win_pause: false
  $name: Win+Pause — System Properties
- win_period: false
  $name: Win+. — Emoji Panel
- win_up: false
  $name: Win+Up — Snap Up
- win_down: false
  $name: Win+Down — Snap Down
- win_left: false
  $name: Win+Left — Snap Left
- win_right: false
  $name: Win+Right — Snap Right
- win_prtscn: false
  $name: Win+PrtScn — Screenshot to File
- win_shift_s: false
  $name: Win+Shift+S — Snipping Tool
- win_shift_left: false
  $name: Win+Shift+Left — Move to Left Monitor
- win_shift_right: false
  $name: Win+Shift+Right — Move to Right Monitor
- win_shift_up: false
  $name: Win+Shift+Up — Stretch Vertical
- win_shift_down: false
  $name: Win+Shift+Down — Restore Vertical
- win_shift_m: false
  $name: Win+Shift+M — Restore All
- win_ctrl_d: false
  $name: Win+Ctrl+D — New Virtual Desktop
- win_ctrl_left: false
  $name: Win+Ctrl+Left — Previous Desktop
- win_ctrl_right: false
  $name: Win+Ctrl+Right — Next Desktop
- win_ctrl_f4: false
  $name: Win+Ctrl+F4 — Close Desktop
- win_ctrl_enter: false
  $name: Win+Ctrl+Enter — Narrator (Alt)
- win_ctrl_c: false
  $name: Win+Ctrl+C — Color Filters
- win_ctrl_q: false
  $name: Win+Ctrl+Q — Quick Assist
- win_alt_d: false
  $name: Win+Alt+D — Date / Time Flyout
- win_alt_g: false
  $name: Win+Alt+G — Xbox Record Last 30s
- win_alt_r: false
  $name: Win+Alt+R — Xbox Start/Stop Capture
- win_ctrl_shift_b: false
  $name: Win+Ctrl+Shift+B — Reset Display Driver
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <commctrl.h>
#include <windows.h>

#include <mutex>
#include <unordered_set>

struct Shortcut {
    const char* key;
    UINT        mods;
    UINT        vk;
};

static const Shortcut kShortcuts[] = {
    // Win + letter
    {"win_a",             MOD_WIN,                          'A'            },
    {"win_b",             MOD_WIN,                          'B'            },
    {"win_c",             MOD_WIN,                          'C'            },
    {"win_d",             MOD_WIN,                          'D'            },
    {"win_e",             MOD_WIN,                          'E'            },
    {"win_f",             MOD_WIN,                          'F'            },
    {"win_g",             MOD_WIN,                          'G'            },
    {"win_h",             MOD_WIN,                          'H'            },
    {"win_i",             MOD_WIN,                          'I'            },
    {"win_k",             MOD_WIN,                          'K'            },
    {"win_m",             MOD_WIN,                          'M'            },
    {"win_n",             MOD_WIN,                          'N'            },
    {"win_o",             MOD_WIN,                          'O'            },
    {"win_p",             MOD_WIN,                          'P'            },
    {"win_q",             MOD_WIN,                          'Q'            },
    {"win_r",             MOD_WIN,                          'R'            },
    {"win_s",             MOD_WIN,                          'S'            },
    {"win_t",             MOD_WIN,                          'T'            },
    {"win_u",             MOD_WIN,                          'U'            },
    {"win_v",             MOD_WIN,                          'V'            },
    {"win_w",             MOD_WIN,                          'W'            },
    {"win_x",             MOD_WIN,                          'X'            },
    {"win_z",             MOD_WIN,                          'Z'            },
    // Win + special keys
    {"win_tab",           MOD_WIN,                          VK_TAB         },
    {"win_space",         MOD_WIN,                          VK_SPACE       },
    {"win_enter",         MOD_WIN,                          VK_RETURN      },
    {"win_home",          MOD_WIN,                          VK_HOME        },
    {"win_pause",         MOD_WIN,                          VK_PAUSE       },
    {"win_period",        MOD_WIN,                          VK_OEM_PERIOD  },
    {"win_up",            MOD_WIN,                          VK_UP          },
    {"win_down",          MOD_WIN,                          VK_DOWN        },
    {"win_left",          MOD_WIN,                          VK_LEFT        },
    {"win_right",         MOD_WIN,                          VK_RIGHT       },
    {"win_prtscn",        MOD_WIN,                          VK_SNAPSHOT    },
    // Win+Shift
    {"win_shift_s",       MOD_WIN | MOD_SHIFT,              'S'            },
    {"win_shift_left",    MOD_WIN | MOD_SHIFT,              VK_LEFT        },
    {"win_shift_right",   MOD_WIN | MOD_SHIFT,              VK_RIGHT       },
    {"win_shift_up",      MOD_WIN | MOD_SHIFT,              VK_UP          },
    {"win_shift_down",    MOD_WIN | MOD_SHIFT,              VK_DOWN        },
    {"win_shift_m",       MOD_WIN | MOD_SHIFT,              'M'            },
    // Win+Ctrl
    {"win_ctrl_d",        MOD_WIN | MOD_CONTROL,            'D'            },
    {"win_ctrl_left",     MOD_WIN | MOD_CONTROL,            VK_LEFT        },
    {"win_ctrl_right",    MOD_WIN | MOD_CONTROL,            VK_RIGHT       },
    {"win_ctrl_f4",       MOD_WIN | MOD_CONTROL,            VK_F4          },
    {"win_ctrl_enter",    MOD_WIN | MOD_CONTROL,            VK_RETURN      },
    {"win_ctrl_c",        MOD_WIN | MOD_CONTROL,            'C'            },
    {"win_ctrl_q",        MOD_WIN | MOD_CONTROL,            'Q'            },
    // Win+Alt
    {"win_alt_d",         MOD_WIN | MOD_ALT,                'D'            },
    {"win_alt_g",         MOD_WIN | MOD_ALT,                'G'            },
    {"win_alt_r",         MOD_WIN | MOD_ALT,                'R'            },
    // Win+Ctrl+Shift
    {"win_ctrl_shift_b",  MOD_WIN | MOD_CONTROL | MOD_SHIFT, 'B'           },
};

static constexpr size_t kShortcutCount = sizeof(kShortcuts) / sizeof(kShortcuts[0]);
static bool g_blocked[kShortcutCount];

static void LoadSettings() {
    for (size_t i = 0; i < kShortcutCount; i++)
        g_blocked[i] = Wh_GetIntSetting(kShortcuts[i].key) != 0;
}

static bool IsBlocked(UINT fsModifiers, UINT vk) {
    const UINT mods = fsModifiers & ~static_cast<UINT>(MOD_NOREPEAT);
    for (size_t i = 0; i < kShortcutCount; i++) {
        if (g_blocked[i] && kShortcuts[i].mods == mods && kShortcuts[i].vk == vk)
            return true;
    }
    return false;
}

static std::mutex               g_subMtx;
static std::unordered_set<HWND> g_subclassedWindows;

LRESULT CALLBACK HotkeySubclassProc(
    HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR)
{
    if (uMsg == WM_HOTKEY) {
        if (IsBlocked(LOWORD(lParam), HIWORD(lParam))) {
            Wh_Log(L"[subclass] Blocked WM_HOTKEY mods=0x%04X vk=0x%04X", LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
    }
    if (uMsg == WM_DESTROY) {
        std::lock_guard<std::mutex> lock(g_subMtx);
        g_subclassedWindows.erase(hwnd);
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void TrySubclass(HWND hwnd) {
    if (!hwnd) return;
    std::lock_guard<std::mutex> lock(g_subMtx);
    if (g_subclassedWindows.count(hwnd)) return;
    if (WindhawkUtils::SetWindowSubclassFromAnyThread(hwnd, HotkeySubclassProc, 0)) {
        g_subclassedWindows.insert(hwnd);
        Wh_Log(L"Subclassed HWND=%p", static_cast<void*>(hwnd));
    }
}

static void SubclassShellWindows() {
    TrySubclass(FindWindowW(L"Progman",       nullptr));
    TrySubclass(FindWindowW(L"Shell_TrayWnd", nullptr));
    TrySubclass(FindWindowW(L"WorkerW",       nullptr));
}

using RegisterHotKey_t = decltype(&RegisterHotKey);
static RegisterHotKey_t RegisterHotKey_Orig = nullptr;

BOOL WINAPI RegisterHotKey_Hook(HWND hWnd, int id, UINT fsModifiers, UINT vk) {
    if (IsBlocked(fsModifiers, vk)) {
        Wh_Log(L"[hook] Faking RegisterHotKey mods=0x%04X vk=0x%04X", fsModifiers, vk);
        SetLastError(0);
        return TRUE;
    }
    const BOOL ok = RegisterHotKey_Orig(hWnd, id, fsModifiers, vk);
    if (ok && hWnd) TrySubclass(hWnd);
    return ok;
}

static void FilterHotkeyMsg(LPMSG lpMsg) {
    if (!lpMsg || lpMsg->message != WM_HOTKEY) return;
    if (IsBlocked(LOWORD(lpMsg->lParam), HIWORD(lpMsg->lParam))) {
        Wh_Log(L"[queue] Blocked WM_HOTKEY mods=0x%04X vk=0x%04X", LOWORD(lpMsg->lParam), HIWORD(lpMsg->lParam));
        lpMsg->message = WM_NULL;
    }
}

using GetMessageW_t = decltype(&GetMessageW);
static GetMessageW_t GetMessageW_Orig = nullptr;

BOOL WINAPI GetMessageW_Hook(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    const BOOL ret = GetMessageW_Orig(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    if (ret && ret != -1) FilterHotkeyMsg(lpMsg);
    return ret;
}

using PeekMessageW_t = decltype(&PeekMessageW);
static PeekMessageW_t PeekMessageW_Orig = nullptr;

BOOL WINAPI PeekMessageW_Hook(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    const BOOL ret = PeekMessageW_Orig(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    if (ret) FilterHotkeyMsg(lpMsg);
    return ret;
}

BOOL Wh_ModInit() {
    Wh_Log(L"disable-windows-shortcuts: init");
    LoadSettings();

    if (!Wh_SetFunctionHook(
            reinterpret_cast<void*>(RegisterHotKey),
            reinterpret_cast<void*>(RegisterHotKey_Hook),
            reinterpret_cast<void**>(&RegisterHotKey_Orig))) {
        Wh_Log(L"ERROR: failed to hook RegisterHotKey");
        return FALSE;
    }
    if (!Wh_SetFunctionHook(
            reinterpret_cast<void*>(GetMessageW),
            reinterpret_cast<void*>(GetMessageW_Hook),
            reinterpret_cast<void**>(&GetMessageW_Orig))) {
        Wh_Log(L"ERROR: failed to hook GetMessageW");
        return FALSE;
    }
    if (!Wh_SetFunctionHook(
            reinterpret_cast<void*>(PeekMessageW),
            reinterpret_cast<void*>(PeekMessageW_Hook),
            reinterpret_cast<void**>(&PeekMessageW_Orig))) {
        Wh_Log(L"ERROR: failed to hook PeekMessageW");
        return FALSE;
    }

    SubclassShellWindows();
    Wh_Log(L"disable-windows-shortcuts: ready");
    return TRUE;
}

void Wh_ModSettingsChanged() {
    Wh_Log(L"disable-windows-shortcuts: settings changed");
    LoadSettings();
}

void Wh_ModUninit() {
    Wh_Log(L"disable-windows-shortcuts: uninit");
}
