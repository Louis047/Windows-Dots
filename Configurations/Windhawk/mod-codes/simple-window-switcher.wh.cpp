// ==WindhawkMod==
// @id              simple-window-switcher
// @name            Simple Window Switcher
// @description     Replaces the default Alt+Tab with a lightweight window switcher inspired by ExplorerPatcher's Simple Window Switcher
// @version         1.0
// @author          Lone
// @github          https://github.com/Louis047
// @include         explorer.exe
// @compilerOptions -ldwmapi -luxtheme -lgdi32 -lshlwapi -loleaut32 -lole32 -lcomctl32
// @architecture    x86-64
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Simple Window Switcher

A lightweight Alt+Tab replacement for Windows, ported from the
[Simple Window Switcher](https://github.com/valinet/sws) project
(part of the ExplorerPatcher ecosystem).

## Features
- Clean grid layout with live DWM thumbnail previews
- Keyboard navigation (Tab / Shift+Tab to cycle, Enter or release Alt to switch, Esc to cancel)
- Mouse click to select a window
- Alt+Ctrl+Tab sticky mode (switcher stays open after releasing Alt)
- Automatic dark/light mode detection
- DPI-aware rendering
- Proper UWP / immersive app support

## How It Works
The mod installs a low-level keyboard hook to intercept Alt+Tab before Windows
processes it, then displays a custom switcher window with DWM live thumbnails.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- rowHeight: 36
  $name: Row Title Height
  $description: Height of the title row in each cell (pixels, before DPI scaling)
- maxThumbWidth: 200
  $name: Max Thumbnail Width
  $description: Maximum width of window thumbnail previews (pixels)
- maxThumbHeight: 150
  $name: Max Thumbnail Height
  $description: Maximum height of window thumbnail previews (pixels)
- showThumbnails: true
  $name: Show Thumbnails
  $description: Show live DWM thumbnail previews of windows
- opacity: 90
  $name: Background Opacity
  $description: Background opacity percentage (0-100)
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <vector>
#include <atomic>
#include <algorithm>

// ============================================================================
// Constants
// ============================================================================
#define SWS_CLASSNAME       L"WindhawkSWS_Switcher"
#define SWS_ICON_SIZE       24
#define SWS_CELL_PADDING    8
#define SWS_GRID_SPACING    4
#define SWS_CONTOUR_SIZE    2
#define SWS_SCREEN_MARGIN   40
#define SWS_TIMER_KEYCHECK  1
#define SWS_TIMER_DELAY     30

// Custom messages from LL hook thread to switcher window
#define WM_SWS_SHOW         (WM_USER + 200)
#define WM_SWS_NEXT         (WM_USER + 201)
#define WM_SWS_PREV         (WM_USER + 202)
#define WM_SWS_DISMISS      (WM_USER + 203)
#define WM_SWS_CANCEL       (WM_USER + 204)

// Colors
#define SWS_BG_DARK          RGB(32, 32, 32)
#define SWS_BG_LIGHT         RGB(243, 243, 243)
#define SWS_HIGHLIGHT_DARK   RGB(60, 120, 200)
#define SWS_HIGHLIGHT_LIGHT  RGB(0, 90, 180)
#define SWS_TEXT_DARK         RGB(255, 255, 255)
#define SWS_TEXT_LIGHT        RGB(0, 0, 0)
#define SWS_BORDER_DARK       RGB(68, 68, 68)
#define SWS_BORDER_LIGHT      RGB(200, 200, 200)

// ============================================================================
// Undocumented API typedefs
// ============================================================================
typedef BOOL (WINAPI *IsShellWindow_t)(HWND);
typedef HWND (WINAPI *GhostWindowFromHungWindow_t)(HWND);

// ============================================================================
// Structures
// ============================================================================
struct WindowEntry {
    HWND hWnd;
    HICON hIcon;
    WCHAR title[256];
    HTHUMBNAIL hThumb;
    RECT rcCell;
    RECT rcThumb;
    bool isUWP;
};

struct Settings {
    int rowHeight;
    int maxThumbWidth;
    int maxThumbHeight;
    bool showThumbnails;
    int opacity;
};

// ============================================================================
// Globals
// ============================================================================
static HWND g_hSwitcher = NULL;
static std::vector<WindowEntry> g_windows;
static int g_selectedIndex = 0;
static int g_hoverIndex = -1;
static bool g_isVisible = false;
static bool g_isSticky = false;
static bool g_isDarkMode = false;
static HFONT g_hFont = NULL;
static UINT g_shellHookMsg = 0;
static int g_dpiX = 96, g_dpiY = 96;
static int g_gridCols = 1, g_gridRows = 1;
static int g_cellW = 0, g_cellH = 0;
static int g_winW = 0, g_winH = 0;

static HANDLE g_hookThread = NULL;
static DWORD g_hookThreadId = 0;
static HHOOK g_hHook = NULL;
static std::atomic<bool> g_hookRunning{false};
static std::atomic<bool> g_altHeld{false};
static std::atomic<bool> g_switcherActive{false};

static Settings g_settings;

static IsShellWindow_t g_IsShellManagedWindow = nullptr;
static IsShellWindow_t g_IsShellFrameWindow = nullptr;
static GhostWindowFromHungWindow_t g_GhostWindowFromHungWindow = nullptr;
static GhostWindowFromHungWindow_t g_HungWindowFromGhostWindow = nullptr;

// ============================================================================
// Dark Mode Detection
// ============================================================================
static bool ShouldUseDarkMode() {
    DWORD value = 0, size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &value, &size) == ERROR_SUCCESS) {
        return value == 0;
    }
    return true;
}

// ============================================================================
// Undocumented API Resolution
// ============================================================================
static bool ResolveAPIs() {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) return false;
    g_IsShellManagedWindow = (IsShellWindow_t)GetProcAddress(hUser32, (LPCSTR)2574);
    g_IsShellFrameWindow = (IsShellWindow_t)GetProcAddress(hUser32, (LPCSTR)2573);
    g_GhostWindowFromHungWindow = (GhostWindowFromHungWindow_t)GetProcAddress(hUser32, "GhostWindowFromHungWindow");
    g_HungWindowFromGhostWindow = (GhostWindowFromHungWindow_t)GetProcAddress(hUser32, "HungWindowFromGhostWindow");
    return (g_IsShellManagedWindow && g_IsShellFrameWindow &&
            g_GhostWindowFromHungWindow && g_HungWindowFromGhostWindow);
}

// ============================================================================
// Window Filtering (ported from SWS sws_WindowHelpers.c)
// ============================================================================
static bool TestExStyle(HWND hWnd, DWORD dwExStyle) {
    return (dwExStyle & (DWORD)GetWindowLongPtrW(hWnd, GWL_EXSTYLE)) == dwExStyle;
}

static bool IsOwnerToolWindow(HWND hwnd) {
    HWND hwndCurrent = hwnd;
    HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
    while (!TestExStyle(hwndCurrent, WS_EX_APPWINDOW) && hwndOwner) {
        HWND hwndPrev = hwndCurrent;
        hwndCurrent = hwndOwner;
        hwndOwner = GetWindow(hwndOwner, GW_OWNER);
        if (TestExStyle(hwndCurrent, WS_EX_TOOLWINDOW)) {
            return !TestExStyle(hwndPrev, WS_EX_CONTROLPARENT) || hwndOwner != NULL;
        }
    }
    return false;
}

static bool IsReallyVisible(HWND hWnd) {
    RECT rc;
    GetWindowRect(hWnd, &rc);
    return IsWindowVisible(hWnd) && !IsRectEmpty(&rc);
}

static bool IsGhosted(HWND hwnd) {
    return g_GhostWindowFromHungWindow && g_GhostWindowFromHungWindow(hwnd) != NULL;
}

static bool ShouldListInAltTab(HWND hwnd) {
    if (!IsWindow(hwnd)) return false;
    DWORD dwExStyle = (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
    bool bOwnerVisible = IsWindow(hwndOwner) && IsWindowEnabled(hwndOwner) && IsReallyVisible(hwndOwner);
    bool bNoActivate = (dwExStyle & WS_EX_NOACTIVATE) || (dwExStyle & WS_EX_TOOLWINDOW);
    bool bAppWindow = (dwExStyle & WS_EX_APPWINDOW) != 0;
    if (bAppWindow) bNoActivate = false;
    return IsReallyVisible(hwnd) && !bNoActivate &&
           (bAppWindow || (!bOwnerVisible && !IsOwnerToolWindow(hwnd))) &&
           !IsGhosted(hwnd);
}

static bool IsTaskWindow(HWND hwnd) {
    DWORD dwExStyle = (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    return ((dwExStyle & WS_EX_APPWINDOW) || (!(dwExStyle & WS_EX_TOOLWINDOW) && !(dwExStyle & WS_EX_NOACTIVATE)))
           && IsWindowVisible(hwnd) && !IsGhosted(hwnd);
}

static bool ShouldTreatAsNormal(HWND hWnd) {
    return GetPropW(hWnd, L"Microsoft.Windows.ShellManagedWindowAsNormalWindow") != NULL;
}

static bool IsAltTabWindow(HWND hWnd) {
    if (!IsWindow(hWnd)) return false;
    // Include UWP shell frame windows (not hung)
    if (g_IsShellFrameWindow && g_IsShellFrameWindow(hWnd) &&
        !(g_GhostWindowFromHungWindow && g_GhostWindowFromHungWindow(hWnd)))
        return true;
    // Exclude shell-managed windows (Start menu, taskbar, etc.)
    if (g_IsShellManagedWindow && g_IsShellManagedWindow(hWnd) && !ShouldTreatAsNormal(hWnd))
        return false;
    // Exclude ExplorerPatcher-managed windows
    if (GetPropW(hWnd, L"valinet.ExplorerPatcher.ShellManagedWindow"))
        return false;
    // Standard alt-tab filtering
    if (!ShouldListInAltTab(hWnd)) return false;
    // Walk owner chain to find the task window
    HWND hwndCurrent = hWnd;
    while (HWND hwndOwner = GetWindow(hwndCurrent, GW_OWNER)) {
        if (!IsTaskWindow(hwndOwner)) break;
        hwndCurrent = hwndOwner;
    }
    return true;
}

// ============================================================================
// Window Enumeration
// ============================================================================
static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    auto* list = reinterpret_cast<std::vector<WindowEntry>*>(lParam);
    if (hWnd == g_hSwitcher) return TRUE;
    if (!IsAltTabWindow(hWnd)) return TRUE;
    // Skip cloaked windows
    BOOL cloaked = FALSE;
    DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked) return TRUE;

    WindowEntry entry = {};
    entry.hWnd = hWnd;
    entry.hThumb = NULL;
    entry.isUWP = g_IsShellFrameWindow && g_IsShellFrameWindow(hWnd);
    // Get title
    InternalGetWindowText(hWnd, entry.title, 256);
    if (!entry.title[0]) GetWindowTextW(hWnd, entry.title, 256);
    // Get icon
    entry.hIcon = NULL;
    SendMessageTimeoutW(hWnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, (DWORD_PTR*)&entry.hIcon);
    if (!entry.hIcon) SendMessageTimeoutW(hWnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, (DWORD_PTR*)&entry.hIcon);
    if (!entry.hIcon) SendMessageTimeoutW(hWnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, (DWORD_PTR*)&entry.hIcon);
    if (!entry.hIcon) entry.hIcon = (HICON)GetClassLongPtrW(hWnd, GCLP_HICONSM);
    if (!entry.hIcon) entry.hIcon = (HICON)GetClassLongPtrW(hWnd, GCLP_HICON);
    if (!entry.hIcon) entry.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    list->push_back(entry);
    return TRUE;
}

static void BuildWindowList() {
    // Unregister old thumbnails
    for (auto& w : g_windows) {
        if (w.hThumb) { DwmUnregisterThumbnail(w.hThumb); w.hThumb = NULL; }
    }
    g_windows.clear();
    EnumWindows(EnumWindowsProc, (LPARAM)&g_windows);
}

// ============================================================================
// Layout Computation
// ============================================================================
static int DpiScale(int val, int dpi) { return MulDiv(val, dpi, 96); }

static void ComputeLayout(HMONITOR hMon) {
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);
    RECT rcWork = mi.rcWork;
    int monW = rcWork.right - rcWork.left;
    int monH = rcWork.bottom - rcWork.top;

    // Get DPI
    UINT dpiX = 96, dpiY = 96;
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        typedef HRESULT(WINAPI*GDPFM)(HMONITOR,int,UINT*,UINT*);
        auto fn = (GDPFM)GetProcAddress(hShcore, "GetDpiForMonitor");
        if (fn) fn(hMon, 0, &dpiX, &dpiY);
        FreeLibrary(hShcore);
    }
    g_dpiX = dpiX; g_dpiY = dpiY;

    int iconSz = DpiScale(SWS_ICON_SIZE, dpiX);
    int pad = DpiScale(SWS_CELL_PADDING, dpiX);
    int spacing = DpiScale(SWS_GRID_SPACING, dpiX);
    int titleH = DpiScale(g_settings.rowHeight, dpiY);
    int thumbW = g_settings.showThumbnails ? DpiScale(g_settings.maxThumbWidth, dpiX) : 0;
    int thumbH = g_settings.showThumbnails ? DpiScale(g_settings.maxThumbHeight, dpiY) : 0;

    g_cellW = (std::max)(thumbW, iconSz + DpiScale(160, dpiX)) + 2 * pad;
    g_cellH = titleH + (g_settings.showThumbnails ? thumbH + pad : 0) + 2 * pad;

    int n = (int)g_windows.size();
    if (n == 0) { g_winW = 0; g_winH = 0; return; }

    int maxCols = (std::max)(1, (monW - DpiScale(SWS_SCREEN_MARGIN * 2, dpiX)) / (g_cellW + spacing));
    g_gridCols = (std::min)(maxCols, n);
    g_gridRows = (n + g_gridCols - 1) / g_gridCols;

    int margin = DpiScale(SWS_SCREEN_MARGIN, dpiX);
    g_winW = g_gridCols * g_cellW + (g_gridCols - 1) * spacing + 2 * margin;
    g_winH = g_gridRows * g_cellH + (g_gridRows - 1) * spacing + 2 * margin;

    // Clamp to monitor
    if (g_winW > monW - 20) g_winW = monW - 20;
    if (g_winH > monH - 20) g_winH = monH - 20;

    // Compute each cell's rect
    for (int i = 0; i < n; i++) {
        int col = i % g_gridCols;
        int row = i / g_gridCols;
        int x = margin + col * (g_cellW + spacing);
        int y = margin + row * (g_cellH + spacing);
        g_windows[i].rcCell = { x, y, x + g_cellW, y + g_cellH };
        // Thumbnail rect within cell
        if (g_settings.showThumbnails) {
            int tx = x + pad;
            int ty = y + titleH + pad;
            int tw = g_cellW - 2 * pad;
            int th = thumbH;
            g_windows[i].rcThumb = { tx, ty, tx + tw, ty + th };
        }
    }
}

// ============================================================================
// DWM Thumbnail Management
// ============================================================================
static void RegisterThumbnails() {
    if (!g_settings.showThumbnails || !g_hSwitcher) return;
    for (auto& w : g_windows) {
        if (w.hThumb) { DwmUnregisterThumbnail(w.hThumb); w.hThumb = NULL; }
        HTHUMBNAIL hThumb = NULL;
        if (SUCCEEDED(DwmRegisterThumbnail(g_hSwitcher, w.hWnd, &hThumb))) {
            SIZE srcSize;
            DwmQueryThumbnailSourceSize(hThumb, &srcSize);
            // Scale to fit within the thumb rect while preserving aspect ratio
            int dstW = w.rcThumb.right - w.rcThumb.left;
            int dstH = w.rcThumb.bottom - w.rcThumb.top;
            if (srcSize.cx > 0 && srcSize.cy > 0) {
                double scaleX = (double)dstW / srcSize.cx;
                double scaleY = (double)dstH / srcSize.cy;
                double scale = (std::min)(scaleX, scaleY);
                int fitW = (int)(srcSize.cx * scale);
                int fitH = (int)(srcSize.cy * scale);
                int offX = (dstW - fitW) / 2;
                int offY = (dstH - fitH) / 2;
                DWM_THUMBNAIL_PROPERTIES props = {};
                props.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE | DWM_TNP_OPACITY;
                props.rcDestination = {
                    w.rcThumb.left + offX, w.rcThumb.top + offY,
                    w.rcThumb.left + offX + fitW, w.rcThumb.top + offY + fitH
                };
                props.opacity = 255;
                props.fVisible = TRUE;
                DwmUpdateThumbnailProperties(hThumb, &props);
            }
            w.hThumb = hThumb;
        }
    }
}

static void UnregisterThumbnails() {
    for (auto& w : g_windows) {
        if (w.hThumb) { DwmUnregisterThumbnail(w.hThumb); w.hThumb = NULL; }
    }
}

// ============================================================================
// Rendering
// ============================================================================
static void PaintSwitcher() {
    if (!g_hSwitcher || !g_isVisible) return;
    RECT rcClient;
    GetClientRect(g_hSwitcher, &rcClient);
    int w = rcClient.right, h = rcClient.bottom;
    if (w <= 0 || h <= 0) return;

    HDC hdcScreen = GetDC(g_hSwitcher);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = NULL;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    BYTE bgAlpha = (BYTE)(g_settings.opacity * 255 / 100);
    COLORREF bgColor = g_isDarkMode ? SWS_BG_DARK : SWS_BG_LIGHT;
    BYTE bgR = GetRValue(bgColor), bgG = GetGValue(bgColor), bgB = GetBValue(bgColor);

    // Fill background with premultiplied alpha
    DWORD* pixels = (DWORD*)bits;
    DWORD bgPixel = ((DWORD)bgAlpha << 24) |
                    ((DWORD)(bgR * bgAlpha / 255) << 16) |
                    ((DWORD)(bgG * bgAlpha / 255) << 8) |
                    (DWORD)(bgB * bgAlpha / 255);
    for (int i = 0; i < w * h; i++) pixels[i] = bgPixel;

    HFONT hOldFont = (HFONT)SelectObject(hdcMem, g_hFont);
    SetBkMode(hdcMem, TRANSPARENT);

    for (int i = 0; i < (int)g_windows.size(); i++) {
        auto& entry = g_windows[i];
        RECT rc = entry.rcCell;
        int pad = DpiScale(SWS_CELL_PADDING, g_dpiX);

        // Draw selection/hover highlight
        bool isSelected = (i == g_selectedIndex);
        bool isHover = (i == g_hoverIndex && !isSelected);
        if (isSelected || isHover) {
            COLORREF hlColor = isSelected ?
                (g_isDarkMode ? SWS_HIGHLIGHT_DARK : SWS_HIGHLIGHT_LIGHT) :
                (g_isDarkMode ? RGB(50, 50, 50) : RGB(220, 220, 220));
            BYTE hlAlpha = isSelected ? 200 : 120;
            BYTE hlR = GetRValue(hlColor), hlG = GetGValue(hlColor), hlB = GetBValue(hlColor);
            DWORD hlPixel = ((DWORD)hlAlpha << 24) |
                            ((DWORD)(hlR * hlAlpha / 255) << 16) |
                            ((DWORD)(hlG * hlAlpha / 255) << 8) |
                            (DWORD)(hlB * hlAlpha / 255);
            for (int py = rc.top; py < rc.bottom && py < h; py++) {
                for (int px = rc.left; px < rc.right && px < w; px++) {
                    if (py >= 0 && px >= 0) pixels[py * w + px] = hlPixel;
                }
            }
            // Draw border for selected
            if (isSelected) {
                int bsz = DpiScale(SWS_CONTOUR_SIZE, g_dpiX);
                COLORREF brdCol = g_isDarkMode ? SWS_HIGHLIGHT_DARK : SWS_HIGHLIGHT_LIGHT;
                BYTE bR = GetRValue(brdCol), bG = GetGValue(brdCol), bB = GetBValue(brdCol);
                DWORD brdPixel = (255u << 24) | ((DWORD)bR << 16) | ((DWORD)bG << 8) | bB;
                for (int py = rc.top; py < rc.bottom && py < h; py++) {
                    for (int px = rc.left; px < rc.right && px < w; px++) {
                        if (py < 0 || px < 0) continue;
                        bool onBorder = (px < rc.left + bsz || px >= rc.right - bsz ||
                                        py < rc.top + bsz || py >= rc.bottom - bsz);
                        if (onBorder) pixels[py * w + px] = brdPixel;
                    }
                }
            }
        }

        // Draw icon
        int iconSz = DpiScale(SWS_ICON_SIZE, g_dpiX);
        int iconX = rc.left + pad;
        int iconY = rc.top + pad + (DpiScale(g_settings.rowHeight, g_dpiY) - iconSz) / 2;
        if (entry.hIcon) {
            DrawIconEx(hdcMem, iconX, iconY, entry.hIcon, iconSz, iconSz, 0, NULL, DI_NORMAL);
        }

        // Draw title text
        RECT rcText;
        rcText.left = iconX + iconSz + pad;
        rcText.top = rc.top + pad;
        rcText.right = rc.right - pad;
        rcText.bottom = rc.top + pad + DpiScale(g_settings.rowHeight, g_dpiY);
        SetTextColor(hdcMem, g_isDarkMode ? SWS_TEXT_DARK : SWS_TEXT_LIGHT);
        DrawTextW(hdcMem, entry.title, -1, &rcText,
                  DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    SelectObject(hdcMem, hOldFont);

    // Update layered window
    POINT ptSrc = {0, 0};
    SIZE szWin = {w, h};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_hSwitcher, hdcScreen, NULL, &szWin, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(g_hSwitcher, hdcScreen);
}

// ============================================================================
// Switcher Show / Hide / Switch Logic
// ============================================================================
static void ShowSwitcher(bool sticky) {
    if (g_windows.size() > 0) UnregisterThumbnails();
    BuildWindowList();
    if (g_windows.empty()) return;

    g_isDarkMode = ShouldUseDarkMode();
    g_isSticky = sticky;

    POINT pt;
    GetCursorPos(&pt);
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    ComputeLayout(hMon);

    if (g_winW <= 0 || g_winH <= 0) return;

    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);
    int cx = (mi.rcWork.left + mi.rcWork.right - g_winW) / 2;
    int cy = (mi.rcWork.top + mi.rcWork.bottom - g_winH) / 2;

    SetWindowPos(g_hSwitcher, HWND_TOPMOST, cx, cy, g_winW, g_winH, SWP_NOACTIVATE);

    // Set rounded corners on Win11
    INT cornerPref = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(g_hSwitcher, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &cornerPref, sizeof(cornerPref));

    g_selectedIndex = (g_windows.size() > 1) ? 1 : 0;
    g_hoverIndex = -1;
    g_isVisible = true;

    ShowWindow(g_hSwitcher, SW_SHOWNOACTIVATE);
    SetForegroundWindow(g_hSwitcher);

    RegisterThumbnails();
    PaintSwitcher();

    if (!g_isSticky) {
        SetTimer(g_hSwitcher, SWS_TIMER_KEYCHECK, SWS_TIMER_DELAY, NULL);
    }
}

static void HideSwitcher() {
    KillTimer(g_hSwitcher, SWS_TIMER_KEYCHECK);
    UnregisterThumbnails();
    ShowWindow(g_hSwitcher, SW_HIDE);
    g_isVisible = false;
    g_isSticky = false;
    g_switcherActive.store(false);
}

static void SwitchToSelected() {
    if (g_selectedIndex < 0 || g_selectedIndex >= (int)g_windows.size()) {
        HideSwitcher();
        return;
    }
    HWND hTarget = g_windows[g_selectedIndex].hWnd;
    HideSwitcher();
    if (IsWindow(hTarget)) {
        HWND hOwner = GetWindow(hTarget, GW_OWNER);
        HWND hPopup = GetLastActivePopup(hOwner ? hOwner : hTarget);
        HWND hFinal = (IsWindowVisible(hPopup)) ? hPopup : hTarget;
        if (IsIconic(hFinal)) ShowWindow(hFinal, SW_RESTORE);
        SwitchToThisWindow(hFinal, TRUE);
    }
}

static void CycleSelection(int delta) {
    if (g_windows.empty()) return;
    int n = (int)g_windows.size();
    g_selectedIndex = ((g_selectedIndex + delta) % n + n) % n;
    PaintSwitcher();
}

static int HitTest(int x, int y) {
    for (int i = 0; i < (int)g_windows.size(); i++) {
        RECT rc = g_windows[i].rcCell;
        if (x >= rc.left && x < rc.right && y >= rc.top && y < rc.bottom)
            return i;
    }
    return -1;
}

// ============================================================================
// Switcher Window Procedure
// ============================================================================
static LRESULT CALLBACK SwitcherWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SWS_SHOW:
        ShowSwitcher(wParam != 0);
        return 0;
    case WM_SWS_NEXT:
        if (g_isVisible) CycleSelection(1);
        return 0;
    case WM_SWS_PREV:
        if (g_isVisible) CycleSelection(-1);
        return 0;
    case WM_SWS_DISMISS:
        if (g_isVisible && !g_isSticky) SwitchToSelected();
        return 0;
    case WM_SWS_CANCEL:
        if (g_isVisible) HideSwitcher();
        return 0;
    case WM_TIMER:
        if (wParam == SWS_TIMER_KEYCHECK) {
            if (!g_isSticky && !(GetAsyncKeyState(VK_MENU) & 0x8000)) {
                SwitchToSelected();
            }
        }
        return 0;
    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        int idx = HitTest(x, y);
        if (idx != g_hoverIndex) {
            g_hoverIndex = idx;
            PaintSwitcher();
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        int idx = HitTest(x, y);
        if (idx >= 0) {
            g_selectedIndex = idx;
            SwitchToSelected();
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (g_isSticky) {
            switch (wParam) {
            case VK_TAB:
                CycleSelection((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return 0;
            case VK_LEFT: case VK_UP:
                CycleSelection(-1); return 0;
            case VK_RIGHT: case VK_DOWN:
                CycleSelection(1); return 0;
            case VK_RETURN:
                SwitchToSelected(); return 0;
            case VK_ESCAPE:
                HideSwitcher(); return 0;
            }
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        UnregisterThumbnails();
        return 0;
    }

    // Handle shell hook messages for window tracking
    if (g_shellHookMsg && uMsg == g_shellHookMsg) {
        HWND hShellWnd = (HWND)lParam;
        if (g_isVisible && hShellWnd && hShellWnd != g_hSwitcher) {
            int code = (int)(wParam & 0x7FFF);
            if (code == HSHELL_WINDOWDESTROYED) {
                // Check if destroyed window is in our list
                for (int i = 0; i < (int)g_windows.size(); i++) {
                    if (g_windows[i].hWnd == hShellWnd) {
                        ShowSwitcher(g_isSticky); // Refresh
                        break;
                    }
                }
            }
        }
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// ============================================================================
// Low-Level Keyboard Hook
// ============================================================================
static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION) return CallNextHookEx(g_hHook, nCode, wParam, lParam);

    KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
    bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
    DWORD vk = kb->vkCode;

    // Ignore injected keystrokes
    if (kb->flags & LLKHF_INJECTED) return CallNextHookEx(g_hHook, nCode, wParam, lParam);

    bool altDown = (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU);
    bool tabKey = (vk == VK_TAB);
    bool escKey = (vk == VK_ESCAPE);
    bool shiftHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrlHeld = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool altHeld = g_altHeld.load();

    if (altDown && isDown) {
        g_altHeld.store(true);
    }
    if (altDown && isUp) {
        g_altHeld.store(false);
        if (g_switcherActive.load()) {
            PostMessageW(g_hSwitcher, WM_SWS_DISMISS, 0, 0);
            return 1; // Block the Alt release to prevent menu activation
        }
    }

    if (tabKey && isDown && g_altHeld.load()) {
        if (!g_switcherActive.load()) {
            g_switcherActive.store(true);
            PostMessageW(g_hSwitcher, WM_SWS_SHOW, ctrlHeld ? 1 : 0, 0);
        } else {
            PostMessageW(g_hSwitcher, shiftHeld ? WM_SWS_PREV : WM_SWS_NEXT, 0, 0);
        }
        return 1; // Block Alt+Tab from reaching the system
    }

    if (escKey && isDown && g_switcherActive.load()) {
        PostMessageW(g_hSwitcher, WM_SWS_CANCEL, 0, 0);
        return 1;
    }

    // In sticky mode, handle navigation keys even without Alt
    if (g_switcherActive.load() && g_isSticky && isDown && !g_altHeld.load()) {
        if (tabKey || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
            vk == VK_RETURN || escKey) {
            // Forward to the switcher window as WM_KEYDOWN
            PostMessageW(g_hSwitcher, WM_KEYDOWN, vk, 0);
            return 1;
        }
    }

    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

static DWORD WINAPI HookThreadProc(LPVOID) {
    g_hHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!g_hHook) {
        Wh_Log(L"Failed to install keyboard hook");
        return 1;
    }
    Wh_Log(L"Keyboard hook installed");
    g_hookRunning.store(true);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_hHook);
    g_hHook = NULL;
    g_hookRunning.store(false);
    return 0;
}

// ============================================================================
// Settings
// ============================================================================
static void LoadSettings() {
    g_settings.rowHeight = Wh_GetIntSetting(L"rowHeight");
    if (g_settings.rowHeight <= 0) g_settings.rowHeight = 36;
    g_settings.maxThumbWidth = Wh_GetIntSetting(L"maxThumbWidth");
    if (g_settings.maxThumbWidth <= 0) g_settings.maxThumbWidth = 200;
    g_settings.maxThumbHeight = Wh_GetIntSetting(L"maxThumbHeight");
    if (g_settings.maxThumbHeight <= 0) g_settings.maxThumbHeight = 150;
    g_settings.showThumbnails = Wh_GetIntSetting(L"showThumbnails");
    g_settings.opacity = Wh_GetIntSetting(L"opacity");
    if (g_settings.opacity <= 0 || g_settings.opacity > 100) g_settings.opacity = 90;
}

// ============================================================================
// Font Creation
// ============================================================================
static void CreateSwitcherFont() {
    if (g_hFont) { DeleteObject(g_hFont); g_hFont = NULL; }
    NONCLIENTMETRICSW ncm = { sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    ncm.lfSmCaptionFont.lfWeight = FW_NORMAL;
    g_hFont = CreateFontIndirectW(&ncm.lfSmCaptionFont);
    if (!g_hFont) g_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

// ============================================================================
// Windhawk Lifecycle
// ============================================================================
BOOL Wh_ModInit() {
    Wh_Log(L"Simple Window Switcher: Init");

    if (!ResolveAPIs()) {
        Wh_Log(L"Failed to resolve undocumented APIs, continuing with limited filtering");
    }

    LoadSettings();
    CreateSwitcherFont();

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SwitcherWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = SWS_CLASSNAME;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    if (!RegisterClassExW(&wc)) {
        Wh_Log(L"Failed to register window class");
        return FALSE;
    }

    // Create switcher window (hidden, layered, topmost, toolwindow)
    g_hSwitcher = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        SWS_CLASSNAME, L"Simple Window Switcher",
        WS_POPUP,
        0, 0, 1, 1,
        NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!g_hSwitcher) {
        Wh_Log(L"Failed to create switcher window");
        UnregisterClassW(SWS_CLASSNAME, GetModuleHandleW(NULL));
        return FALSE;
    }

    // Register shell hook for window tracking
    g_shellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");
    RegisterShellHookWindow(g_hSwitcher);

    // Start keyboard hook thread
    g_hookThread = CreateThread(NULL, 0, HookThreadProc, NULL, 0, &g_hookThreadId);
    if (!g_hookThread) {
        Wh_Log(L"Failed to start hook thread");
        DeregisterShellHookWindow(g_hSwitcher);
        DestroyWindow(g_hSwitcher);
        UnregisterClassW(SWS_CLASSNAME, GetModuleHandleW(NULL));
        return FALSE;
    }

    Wh_Log(L"Simple Window Switcher: Ready");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Simple Window Switcher: Uninit");

    // Stop keyboard hook thread
    if (g_hookThreadId) {
        PostThreadMessageW(g_hookThreadId, WM_QUIT, 0, 0);
        if (g_hookThread) {
            WaitForSingleObject(g_hookThread, 2000);
            CloseHandle(g_hookThread);
            g_hookThread = NULL;
        }
        g_hookThreadId = 0;
    }

    // Clean up switcher
    if (g_isVisible) HideSwitcher();
    UnregisterThumbnails();

    if (g_hSwitcher) {
        DeregisterShellHookWindow(g_hSwitcher);
        DestroyWindow(g_hSwitcher);
        g_hSwitcher = NULL;
    }

    UnregisterClassW(SWS_CLASSNAME, GetModuleHandleW(NULL));

    if (g_hFont) { DeleteObject(g_hFont); g_hFont = NULL; }

    g_windows.clear();
}

void Wh_ModSettingsChanged() {
    Wh_Log(L"Simple Window Switcher: Settings changed");
    LoadSettings();
    if (g_isVisible) {
        // Refresh with new settings
        POINT pt; GetCursorPos(&pt);
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
        UnregisterThumbnails();
        ComputeLayout(hMon);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(hMon, &mi);
        int cx = (mi.rcWork.left + mi.rcWork.right - g_winW) / 2;
        int cy = (mi.rcWork.top + mi.rcWork.bottom - g_winH) / 2;
        SetWindowPos(g_hSwitcher, HWND_TOPMOST, cx, cy, g_winW, g_winH, SWP_NOACTIVATE);
        RegisterThumbnails();
        PaintSwitcher();
    }
}