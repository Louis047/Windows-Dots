// ==WindhawkMod==
// @id              cursor-motion-blur
// @name            Cursor Motion Blur
// @description     Adds a smooth, motion-blur trail to the mouse cursor, with speed-adaptive parameters.
// @version         1.4.1
// @author          Mykyta Shcherbyna
// @github          https://github.com/mshcherbyna99
// @architecture    x86
// @architecture    amd64
// @architecture    x86-64
// @include         windhawk.exe
// @include         LogonUI.exe
// @include         consent.exe
// @include         LockApp.exe
// @compilerOptions -lgdi32 -lmsimg32 -lwinmm
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Cursor - Trail with Motion Blur

This mod adds a custom, smooth motion-blur trail to your mouse cursor.

**Features**:
- Variable trail copies and opacity.
- Configurable Box Blur for a glow or motion-blur effect.
- Speed-Adaptive mode: The trail dynamically adjusts its length and density based on how fast you move the cursor.
- Dynamic Refresh Rate Scaling: Automatically adjusts the framerate and trail smear based on the monitor you are currently using. Perfect for laptops with battery-saving 60Hz modes or multi-monitor setups.
- Optimized rendering with dirty-region bounding boxes to minimize CPU usage.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- dynamicRefreshScaling: true
  $name: Dynamic Refresh Rate Scaling
  $description: Automatically matches the render framerate to your current monitor and scales the trail length to mask lower refresh rates (e.g., 60Hz). Overrides Maximum Render Framerate.

- frameRate: 144
  $name: Maximum Render Framerate
  $description: "Maximum render rate (5-240 FPS). Used only if Dynamic Refresh Rate Scaling is disabled. Higher values reduce lag between the system cursor and the trail."

- numCopies: 15
  $name: Trail Copies
  $description: Maximum number of cursor trail copies.

- baseOpacity: 50
  $name: Trail Opacity (1-255)
  $description: Opacity of the trail copies.

- delayPerCopy: 0.5
  $name: Delay Between Copies (ms)

- blurRadius: 2
  $name: Trail Blur Radius (0-4)
  $description: Box blur on trail copies. 0 = sharp, 1-2 = soft glow, 3-4 = heavy blur.

- enableSpeedAdaptive: true
  $name: Enable Speed-Adaptive Trail

- minCopies: 5
  $name: Minimum Trail Copies (Adaptive)

- maxCopies: 15
  $name: Maximum Trail Copies (Adaptive)

- minDelay: 0.3
  $name: Minimum Delay Between Copies (ms, Adaptive)

- maxDelay: 0.8
  $name: Maximum Delay Between Copies (ms, Adaptive)
*/
// ==/WindhawkModSettings==

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windhawk_api.h>
#include <atomic>
#include <thread>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <mmsystem.h> // For timeBeginPeriod

// --- Constants & Structures ---

static constexpr wchar_t WINDOW_CLASS_NAME[] = L"WH_CursorMotionBlur";
static constexpr UINT WM_RELOAD_CONFIG = WM_APP + 1;
static constexpr UINT WM_EXIT_THREAD = WM_APP + 2;
static constexpr int MAX_TRAIL_COPIES = 200;
static constexpr int RENDER_PADDING = 32;
static constexpr BYTE ALPHA_THRESHOLD = 4;
static constexpr int RING_BUFFER_CAPACITY = 4096;

struct PositionRecord {
    POINT point;
    LARGE_INTEGER timestamp;
};

// Lock-free ring buffer for mouse positions
struct RingBuffer {
    PositionRecord data[RING_BUFFER_CAPACITY];
    alignas(64) volatile LONG head = 0;
    volatile LONG count = 0;

    void Reset() {
        head = 0;
        count = 0;
    }

    void Push(POINT p, LARGE_INTEGER t) {
        LONG currentHead = InterlockedIncrement(&head) - 1;
        data[currentHead & (RING_BUFFER_CAPACITY - 1)] = { p, t };
        
        LONG currentCount;
        do {
            currentCount = count;
            if (currentCount >= RING_BUFFER_CAPACITY) break;
        } while (InterlockedCompareExchange(&count, currentCount + 1, currentCount) != currentCount);
    }

    int Read(PositionRecord* output, int numToRead) const {
        LONG currentCount = count;
        LONG currentHead = head;
        
        if (currentCount <= 0 || numToRead <= 0) return 0;
        
        if (numToRead > currentCount) numToRead = currentCount;
        if (numToRead > RING_BUFFER_CAPACITY) numToRead = RING_BUFFER_CAPACITY;
        
        LONG startIdx = currentHead - numToRead;
        for (int i = 0; i < numToRead; ++i) {
            output[i] = data[(startIdx + i) & (RING_BUFFER_CAPACITY - 1)];
        }
        return numToRead;
    }

    bool GetLast(PositionRecord* output) const {
        LONG currentCount = count;
        if (currentCount <= 0) return false;
        LONG currentHead = head;
        *output = data[(currentHead - 1) & (RING_BUFFER_CAPACITY - 1)];
        return true;
    }

    LONG GetHead() const {
        return head;
    }
};

struct ModConfig {
    bool dynamicRefreshScaling = true;
    int copies = 15;
    int opacity = 50;
    int blur = 2;
    float delay = 0.5f;
    bool adaptive = true;
    int minCopies = 5;
    int maxCopies = 15;
    int fps = 144;
    float minDelay = 0.3f;
    float maxDelay = 0.8f;
};

struct ModState {
    std::atomic<bool> isRunning{ false };
    std::thread renderThread;
    DWORD threadId = 0;
    
    HWND overlayWindow = nullptr;
    HHOOK mouseHook = nullptr;
    
    RingBuffer ringBuffer;
    ModConfig config;
    LARGE_INTEGER performanceFreq;
    
    // Dynamic Monitor State
    HMONITOR lastMonitor = nullptr;
    int currentRefreshRate = 144;

    // Effective settings after adaptive calculation
    int effectiveCopies = 18;
    float effectiveDelay = 1.0f;
    
    // Cursor Info
    HCURSOR lastCursor = nullptr;
    HICON cursorIcon = nullptr;
    int cursorWidth = 32;
    int cursorHeight = 32;
    POINT cursorHotspot = { 0, 0 };
    
    // Render Surfaces
    HDC stampDC = nullptr;
    HBITMAP stampBitmap = nullptr;
    HBITMAP stampBitmapOld = nullptr;
    void* stampPixels = nullptr;
    int stampWidth = 0;
    int stampHeight = 0;
    bool isStampValid = false;
    int blurPadding = 0;
    
    HDC composeDC = nullptr;
    HBITMAP composeBitmap = nullptr;
    HBITMAP composeBitmapOld = nullptr;
    void* composePixels = nullptr;
    int composeWidth = 0;
    int composeHeight = 0;
    bool isTrailVisible = false;
    struct RenderedStamp { int x, y, w, h; };
    RenderedStamp lastRenderedStamps[MAX_TRAIL_COPIES];
    int numLastRenderedStamps = 0;
};

static ModState g_state;

// --- Forward Declarations ---
static void RefreshMonitorState(POINT pt);
static void RefreshCursorState();
static void PrepareStamp();

// --- Utility Functions ---

template<typename T>
static T Clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

static float ReadFloatSetting(const wchar_t* name, float defaultValue) {
    PCWSTR strValue = Wh_GetStringSetting(name);
    float result = defaultValue;
    if (strValue && *strValue) {
        wchar_t* endPtr = nullptr;
        float parsedValue = wcstof(strValue, &endPtr);
        if (endPtr && endPtr != strValue) {
            result = parsedValue;
        }
    }
    Wh_FreeStringSetting(strValue);
    return result;
}

static void LoadConfig() {
    g_state.config.dynamicRefreshScaling = Wh_GetIntSetting(L"dynamicRefreshScaling") != 0;
    g_state.config.copies   = Clamp(Wh_GetIntSetting(L"numCopies"), 1, MAX_TRAIL_COPIES);
    g_state.config.opacity  = Clamp(Wh_GetIntSetting(L"baseOpacity"), 1, 255);
    g_state.config.delay    = Clamp(ReadFloatSetting(L"delayPerCopy", 1.0f), 0.1f, 30.0f);
    g_state.config.blur     = Clamp(Wh_GetIntSetting(L"blurRadius"), 0, 4);
    g_state.config.adaptive = Wh_GetIntSetting(L"enableSpeedAdaptive") != 0;
    g_state.config.minCopies= Clamp(Wh_GetIntSetting(L"minCopies"), 1, MAX_TRAIL_COPIES);
    g_state.config.maxCopies= Clamp(Wh_GetIntSetting(L"maxCopies"), 1, MAX_TRAIL_COPIES);
    g_state.config.minDelay = Clamp(ReadFloatSetting(L"minDelay", 0.5f), 0.1f, 30.0f);
    g_state.config.maxDelay = Clamp(ReadFloatSetting(L"maxDelay", 2.0f), 0.1f, 60.0f);
    g_state.config.fps      = Clamp(Wh_GetIntSetting(L"frameRate"), 5, 240);
    
    g_state.effectiveCopies = g_state.config.copies;
    g_state.effectiveDelay  = g_state.config.delay;
    
    if (g_state.config.minCopies > g_state.config.maxCopies) {
        std::swap(g_state.config.minCopies, g_state.config.maxCopies);
    }
    if (g_state.config.minDelay > g_state.config.maxDelay) {
        std::swap(g_state.config.minDelay, g_state.config.maxDelay);
    }
    
    g_state.isStampValid = false;
    g_state.lastMonitor = nullptr; // Force monitor refresh evaluation
    
    // OPTIMIZATION: Get current cursor point immediately to setup scaling without waiting for mouse move
    POINT pt;
    if (GetCursorPos(&pt)) {
        RefreshMonitorState(pt);
    }

    // OPTIMIZATION: Pre-calculate the blurred cursor stamp immediately.
    RefreshCursorState();
    if (g_state.cursorIcon) {
        PrepareStamp(); 
    }

    Wh_Log(L"Config loaded: copies=%d, opacity=%d, delay=%.2f, blur=%d, fps=%d, dynScaling=%d",
           g_state.config.copies, g_state.config.opacity, g_state.config.delay, 
           g_state.config.blur, g_state.config.fps, g_state.config.dynamicRefreshScaling);
}

static void RefreshMonitorState(POINT pt) {
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (hMon != g_state.lastMonitor) {
        g_state.lastMonitor = hMon;
        MONITORINFOEXW mi = {};
        mi.cbSize = sizeof(MONITORINFOEXW);
        if (GetMonitorInfoW(hMon, &mi)) {
            DEVMODEW dm = {};
            dm.dmSize = sizeof(DEVMODEW);
            if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
                if (dm.dmDisplayFrequency > 1) {
                    g_state.currentRefreshRate = dm.dmDisplayFrequency;
                    Wh_Log(L"Monitor changed. New Refresh Rate: %d Hz", g_state.currentRefreshRate);
                    return;
                }
            }
        }
        g_state.currentRefreshRate = 60; // Fallback
    }
}

// --- Graphics & Rendering Functions ---

static HBITMAP CreateDIBSection32(HDC dc, int width, int height, void** pixelsOut) {
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // Top-down DIB
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(dc, &bi, DIB_RGB_COLORS, pixelsOut, nullptr, 0);
}

static void ApplyBoxBlur(BYTE* pixels, int width, int height, int radius) {
    if (radius <= 0 || width <= 0 || height <= 0) return;
    
    int totalSize = width * height * 4;
    BYTE* tempPixels = (BYTE*)HeapAlloc(GetProcessHeap(), 0, totalSize);
    if (!tempPixels) return;
    
    int diameter = radius * 2 + 1;
    
    // Horizontal pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int sumB = 0, sumG = 0, sumR = 0, sumA = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                int sampleX = Clamp(x + dx, 0, width - 1);
                int offset = (y * width + sampleX) * 4;
                sumB += pixels[offset];
                sumG += pixels[offset + 1];
                sumR += pixels[offset + 2];
                sumA += pixels[offset + 3];
            }
            int offset = (y * width + x) * 4;
            tempPixels[offset]     = (BYTE)(sumB / diameter);
            tempPixels[offset + 1] = (BYTE)(sumG / diameter);
            tempPixels[offset + 2] = (BYTE)(sumR / diameter);
            tempPixels[offset + 3] = (BYTE)(sumA / diameter);
        }
    }
    
    // Vertical pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int sumB = 0, sumG = 0, sumR = 0, sumA = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                int sampleY = Clamp(y + dy, 0, height - 1);
                int offset = (sampleY * width + x) * 4;
                sumB += tempPixels[offset];
                sumG += tempPixels[offset + 1];
                sumR += tempPixels[offset + 2];
                sumA += tempPixels[offset + 3];
            }
            int offset = (y * width + x) * 4;
            pixels[offset]     = (BYTE)(sumB / diameter);
            pixels[offset + 1] = (BYTE)(sumG / diameter);
            pixels[offset + 2] = (BYTE)(sumR / diameter);
            pixels[offset + 3] = (BYTE)(sumA / diameter);
        }
    }
    
    HeapFree(GetProcessHeap(), 0, tempPixels);
}

static void FreeStampBuffer() {
    if (g_state.stampDC) {
        SelectObject(g_state.stampDC, g_state.stampBitmapOld);
        DeleteObject(g_state.stampBitmap);
        DeleteDC(g_state.stampDC);
        g_state.stampDC = nullptr;
        g_state.stampPixels = nullptr;
        g_state.isStampValid = false;
    }
}

static void FreeComposeBuffer() {
    if (g_state.composeDC) {
        SelectObject(g_state.composeDC, g_state.composeBitmapOld);
        DeleteObject(g_state.composeBitmap);
        DeleteDC(g_state.composeDC);
        g_state.composeDC = nullptr;
        g_state.composePixels = nullptr;
    }
}

static void EnsureStampBuffer(int width, int height) {
    if (g_state.stampDC && g_state.stampWidth == width && g_state.stampHeight == height) return;
    
    FreeStampBuffer();
    
    HDC screenDC = GetDC(nullptr);
    g_state.stampDC = CreateCompatibleDC(screenDC);
    g_state.stampBitmap = CreateDIBSection32(g_state.stampDC, width, height, &g_state.stampPixels);
    
    if (!g_state.stampBitmap) {
        DeleteDC(g_state.stampDC);
        g_state.stampDC = nullptr;
        ReleaseDC(nullptr, screenDC);
        return;
    }
    
    g_state.stampBitmapOld = (HBITMAP)SelectObject(g_state.stampDC, g_state.stampBitmap);
    g_state.stampWidth = width;
    g_state.stampHeight = height;
    
    ReleaseDC(nullptr, screenDC);
}

static void EnsureComposeBuffer(int width, int height) {
    if (g_state.composeDC && g_state.composeWidth >= width && g_state.composeHeight >= height) return;
    
    // Pad dimensions to avoid frequent reallocations
    int paddedWidth = ((width + 127) & ~127);
    int paddedHeight = ((height + 127) & ~127);
    
    FreeComposeBuffer();
    
    HDC screenDC = GetDC(nullptr);
    g_state.composeDC = CreateCompatibleDC(screenDC);
    g_state.composeBitmap = CreateDIBSection32(g_state.composeDC, paddedWidth, paddedHeight, &g_state.composePixels);
    
    if (!g_state.composeBitmap) {
        DeleteDC(g_state.composeDC);
        g_state.composeDC = nullptr;
        ReleaseDC(nullptr, screenDC);
        return;
    }
    
    g_state.composeBitmapOld = (HBITMAP)SelectObject(g_state.composeDC, g_state.composeBitmap);
    g_state.composeWidth = paddedWidth;
    g_state.composeHeight = paddedHeight;
    g_state.numLastRenderedStamps = 0; // OS zero-initializes new DIB sections
    ReleaseDC(nullptr, screenDC);
}

static void PrepareStamp() {
    if (!g_state.cursorIcon) return;
    
    int padding = g_state.config.blur * 2;
    g_state.blurPadding = padding;
    
    int stampW = g_state.cursorWidth + padding * 2;
    int stampH = g_state.cursorHeight + padding * 2;
    
    EnsureStampBuffer(stampW, stampH);
    if (!g_state.stampDC || !g_state.stampPixels) return;
    
    // Clear stamp memory
    memset(g_state.stampPixels, 0, stampW * stampH * 4);
    
    // Draw the cursor icon into the center of the stamp DC
    DrawIconEx(g_state.stampDC, padding, padding, g_state.cursorIcon, g_state.cursorWidth, g_state.cursorHeight, 0, nullptr, DI_NORMAL);
    GdiFlush();
    
    // Premultiply alpha (needed for AlphaBlend later)
    BYTE* pixels = (BYTE*)g_state.stampPixels;
    for (int i = 0; i < stampW * stampH; ++i) {
        int offset = i * 4;
        BYTE b = pixels[offset];
        BYTE g = pixels[offset + 1];
        BYTE r = pixels[offset + 2];
        BYTE a = pixels[offset + 3];
        
        // Handle opaque cursors that lack alpha channel properly
        if (a == 0 && (r | g | b)) {
            a = 255;
        }
        
        pixels[offset]     = (BYTE)((b * a) / 255);
        pixels[offset + 1] = (BYTE)((g * a) / 255);
        pixels[offset + 2] = (BYTE)((r * a) / 255);
        pixels[offset + 3] = a;
    }
    
    if (g_state.config.blur > 0) {
        ApplyBoxBlur(pixels, stampW, stampH, g_state.config.blur);
    }
    
    g_state.isStampValid = true;
}

// --- Hook and Logic Updates ---

static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_MOUSEMOVE) {
        auto* hookStruct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        if (hookStruct) {
            LARGE_INTEGER timestamp;
            QueryPerformanceCounter(&timestamp);
            g_state.ringBuffer.Push(hookStruct->pt, timestamp);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

static float CalculateCursorSpeed() {
    PositionRecord records[8];
    int numRead = g_state.ringBuffer.Read(records, 8);
    if (numRead < 2) return 0.0f;
    
    double dt = (double)(records[numRead - 1].timestamp.QuadPart - records[0].timestamp.QuadPart);
    if (dt <= 0) return 0.0f;
    
    double ms = dt * 1000.0 / (double)g_state.performanceFreq.QuadPart;
    if (ms <= 0) return 0.0f;
    
    int dx = records[numRead - 1].point.x - records[0].point.x;
    int dy = records[numRead - 1].point.y - records[0].point.y;
    
    return (float)(std::sqrt((double)(dx * dx + dy * dy)) / ms);
}

static void UpdateAdaptiveSettings(float speed) {
    // Speed threshold normalization
    float t = Clamp(speed / 5.0f, 0.0f, 1.0f);
    
    float targetMinDelay = g_state.config.minDelay;
    float targetMaxDelay = g_state.config.maxDelay;
    int targetMinCopies = g_state.config.minCopies;
    int targetMaxCopies = g_state.config.maxCopies;

    if (g_state.config.dynamicRefreshScaling) {
        // Baseline is 144Hz. At lower Hz, scale up the number of copies to bridge larger frametime gaps.
        float scale = 144.0f / (float)g_state.currentRefreshRate;
        scale = Clamp(scale, 0.5f, 3.0f);
        
        // We intentionally ONLY scale copies. 
        // Total Trail Time = (Copies * scale) * Delay = BaseTime * scale.
        // This ensures the physical trail length on screen accurately masks lower refresh rates
        // without causing a multiplicative "snake" effect that scaling the delay would also produce.
        targetMinCopies = (int)(targetMinCopies * scale);
        targetMaxCopies = (int)(targetMaxCopies * scale);
    }
    
    // When moving faster, decrease delay (denser trail) and increase copies (longer trail)
    g_state.effectiveDelay = targetMaxDelay - t * (targetMaxDelay - targetMinDelay);
    g_state.effectiveCopies = (int)(targetMinCopies + t * (targetMaxCopies - targetMinCopies));
}

static void RefreshCursorState() {
    CURSORINFO cursorInfo = {};
    cursorInfo.cbSize = sizeof(cursorInfo);
    
    if (!GetCursorInfo(&cursorInfo)) return;
    if (!(cursorInfo.flags & CURSOR_SHOWING) || !cursorInfo.hCursor) return;
    
    // Skip if cursor hasn't changed
    if (cursorInfo.hCursor == g_state.lastCursor && g_state.cursorIcon) return;
    
    if (g_state.cursorIcon) {
        DestroyIcon(g_state.cursorIcon);
        g_state.cursorIcon = nullptr;
    }
    
    g_state.cursorIcon = CopyIcon(cursorInfo.hCursor);
    g_state.lastCursor = cursorInfo.hCursor;
    
    if (!g_state.cursorIcon) return;
    
    ICONINFO iconInfo = {};
    if (GetIconInfo(cursorInfo.hCursor, &iconInfo)) {
        g_state.cursorHotspot = { (LONG)iconInfo.xHotspot, (LONG)iconInfo.yHotspot };
        
        BITMAP bmp = {};
        if (iconInfo.hbmMask && GetObject(iconInfo.hbmMask, sizeof(bmp), &bmp)) {
            g_state.cursorWidth = bmp.bmWidth;
            g_state.cursorHeight = iconInfo.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2;
        }
        
        if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
        if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    }
    
    if (g_state.cursorWidth <= 0) g_state.cursorWidth = 32;
    if (g_state.cursorHeight <= 0) g_state.cursorHeight = 32;
    
    g_state.isStampValid = false;
}

struct TrailCopy {
    POINT pos;
    BYTE alpha;
};

// Instead of toggling ShowWindow (which causes DWM to constantly rebuild underlying UI logic and flicker),
// we just shrink the window to an invisible 1x1 transparent pixel when the cursor is idle.
static void HideOverlay() {
    if (!g_state.isTrailVisible) return;
    
    EnsureComposeBuffer(1, 1);
    if (g_state.composeDC && g_state.composePixels) {
        memset(g_state.composePixels, 0, 4);
        SIZE szWindow = { 1, 1 };
        POINT ptSrc = { 0, 0 };
        BLENDFUNCTION winBlendFunc = {};
        winBlendFunc.BlendOp = AC_SRC_OVER;
        winBlendFunc.SourceConstantAlpha = 255;
        winBlendFunc.AlphaFormat = AC_SRC_ALPHA;
        
        HDC screenDC = GetDC(nullptr);
        UpdateLayeredWindow(g_state.overlayWindow, screenDC, nullptr, &szWindow, 
                            g_state.composeDC, &ptSrc, 0, &winBlendFunc, ULW_ALPHA);
        ReleaseDC(nullptr, screenDC);
    }
    g_state.isTrailVisible = false;
}

static void RenderTrail() {
    if (!g_state.overlayWindow) return;
    
    if (g_state.config.adaptive) {
        UpdateAdaptiveSettings(CalculateCursorSpeed());
    } else if (g_state.config.dynamicRefreshScaling) {
        // If not adaptive but dynamic scaling is on, still scale the fixed copies to match refresh rate
        float scale = Clamp(144.0f / (float)g_state.currentRefreshRate, 0.5f, 3.0f);
        g_state.effectiveCopies = Clamp((int)(g_state.config.copies * scale), 1, MAX_TRAIL_COPIES);
        g_state.effectiveDelay = g_state.config.delay; // Keep fixed to prevent exponential scaling
    }
    static LARGE_INTEGER lastCursorCheck = {{0}};
    LARGE_INTEGER currentTicks;
    QueryPerformanceCounter(&currentTicks);
    if ((currentTicks.QuadPart - lastCursorCheck.QuadPart) * 1000 / g_state.performanceFreq.QuadPart >= 16) {
        RefreshCursorState();
        lastCursorCheck = currentTicks;
    }
    
    if (!g_state.cursorIcon) return;
    if (!g_state.isStampValid) PrepareStamp();
    if (!g_state.isStampValid || !g_state.stampDC || !g_state.stampPixels) return;
    
    int numCopies = Clamp(g_state.effectiveCopies, 1, MAX_TRAIL_COPIES);
    float totalTrailTimeMs = numCopies * g_state.effectiveDelay;
    
    PositionRecord history[512];
    int historyCount = g_state.ringBuffer.Read(history, 512);
    
    if (historyCount < 2) {
        HideOverlay();
        return;
    }
    
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    
    // --- Arc-length uniform sampling ---
    float arcLengths[512];
    int numArcs = 1;
    arcLengths[0] = 0.0f;
    
    LONGLONG cutoffTicks = (LONGLONG)((double)totalTrailTimeMs * 0.001 * (double)g_state.performanceFreq.QuadPart);
    LARGE_INTEGER timeCutoff;
    timeCutoff.QuadPart = now.QuadPart - cutoffTicks;
    
    // Build cumulative arc length backwards in time
    for (int k = 1; k < historyCount && numArcs < 512; ++k) {
        int currentIdx = historyCount - 1 - k;
        if (currentIdx < 0) break;
        if (history[currentIdx].timestamp.QuadPart < timeCutoff.QuadPart) break;
        
        int prevIdx = currentIdx + 1;
        int dx = history[prevIdx].point.x - history[currentIdx].point.x;
        int dy = history[prevIdx].point.y - history[currentIdx].point.y;
        
        arcLengths[numArcs] = arcLengths[numArcs - 1] + std::sqrt((float)(dx * dx + dy * dy));
        numArcs++;
    }
    
    float totalArcLength = arcLengths[numArcs - 1];
    
    if (numArcs < 2 || totalArcLength < 1.0f) {
        HideOverlay();
        return;
    }
    
    TrailCopy rawCopies[MAX_TRAIL_COPIES];
    
    for (int i = 0; i < numCopies; ++i) {
        float targetArc = (numCopies == 1) ? 0.0f : ((float)i / (float)(numCopies - 1)) * totalArcLength;
        
        // Binary search for bracket
        int lowIdx = 0;
        int highIdx = numArcs - 1;
        while (lowIdx < highIdx - 1) {
            int midIdx = (lowIdx + highIdx) / 2;
            if (arcLengths[midIdx] <= targetArc) {
                lowIdx = midIdx;
            } else {
                highIdx = midIdx;
            }
        }
        
        float segmentLength = arcLengths[highIdx] - arcLengths[lowIdx];
        float fraction = (segmentLength > 1e-6f) ? ((targetArc - arcLengths[lowIdx]) / segmentLength) : 0.0f;
        
        POINT ptA = history[historyCount - 1 - lowIdx].point;
        POINT ptB = history[historyCount - 1 - highIdx].point;
        
        rawCopies[i].pos.x = (int)(ptA.x + fraction * (ptB.x - ptA.x));
        rawCopies[i].pos.y = (int)(ptA.y + fraction * (ptB.y - ptA.y));
        
        // Fade out older copies quadratically
        float fadeRatio = (float)i / (float)numCopies;
        float squaredFade = fadeRatio * fadeRatio;
        rawCopies[i].alpha = (BYTE)Clamp((int)(g_state.config.opacity * (1.0f - squaredFade)), 0, 255);
    }
    
    // --- Deduplication & Bounding Box ---
    // Merge overlapping copies at the same pixel position to avoid overdraw and boost performance
    TrailCopy uniqueCopies[MAX_TRAIL_COPIES];
    int numUnique = 0;
    
    for (int i = 0; i < numCopies; ++i) {
        if (rawCopies[i].alpha < ALPHA_THRESHOLD) continue;
        
        bool isMerged = false;
        for (int j = 0; j < numUnique; ++j) {
            if (uniqueCopies[j].pos.x == rawCopies[i].pos.x && uniqueCopies[j].pos.y == rawCopies[i].pos.y) {
                if (rawCopies[i].alpha > uniqueCopies[j].alpha) {
                    uniqueCopies[j].alpha = rawCopies[i].alpha;
                }
                isMerged = true;
                break;
            }
        }
        if (!isMerged) {
            uniqueCopies[numUnique++] = rawCopies[i];
        }
    }
    
    if (numUnique == 0) {
        HideOverlay();
        return;
    }
    
    int pad = g_state.blurPadding;
    int stampW = g_state.stampWidth;
    int stampH = g_state.stampHeight;
    
    int boundLeft = INT_MAX, boundTop = INT_MAX, boundRight = INT_MIN, boundBottom = INT_MIN;
    for (int i = 0; i < numUnique; ++i) {
        int x = uniqueCopies[i].pos.x - g_state.cursorHotspot.x - pad;
        int y = uniqueCopies[i].pos.y - g_state.cursorHotspot.y - pad;
        if (x < boundLeft) boundLeft = x;
        if (y < boundTop) boundTop = y;
        if (x + stampW > boundRight) boundRight = x + stampW;
        if (y + stampH > boundBottom) boundBottom = y + stampH;
    }
    
    boundLeft -= RENDER_PADDING;
    boundTop -= RENDER_PADDING;
    boundRight += RENDER_PADDING;
    boundBottom += RENDER_PADDING;
    
    int boundsWidth = boundRight - boundLeft;
    int boundsHeight = boundBottom - boundTop;
    
    if (boundsWidth <= 0 || boundsHeight <= 0) return;
    if (boundsWidth > 4096) boundsWidth = 4096;
    if (boundsHeight > 4096) boundsHeight = 4096;
    
    EnsureComposeBuffer(boundsWidth, boundsHeight);
    if (!g_state.composeDC || !g_state.composePixels) return;
    // Clear ONLY the areas we dirtied in the previous frame
    BYTE* basePixels = (BYTE*)g_state.composePixels;
    int rowBytes = g_state.composeWidth * 4;
    for (int i = 0; i < g_state.numLastRenderedStamps; ++i) {
        int sx = g_state.lastRenderedStamps[i].x;
        int sy = g_state.lastRenderedStamps[i].y;
        int sw = g_state.lastRenderedStamps[i].w;
        int sh = g_state.lastRenderedStamps[i].h;
        
        int cx = Clamp(sx, 0, g_state.composeWidth - 1);
        int cy = Clamp(sy, 0, g_state.composeHeight - 1);
        int cw = Clamp(sw, 0, g_state.composeWidth - cx);
        int ch = Clamp(sh, 0, g_state.composeHeight - cy);
        
        for (int y = 0; y < ch; ++y) {
            memset(basePixels + (cy + y) * rowBytes + cx * 4, 0, cw * 4);
        }
    }
    g_state.numLastRenderedStamps = 0;
    
    BLENDFUNCTION blendFunc = {};
    blendFunc.BlendOp = AC_SRC_OVER;
    blendFunc.AlphaFormat = AC_SRC_ALPHA;
    
    // Draw copies (oldest first for correct depth ordering when blurred stamps overlap)
    for (int i = numUnique - 1; i >= 0; --i) {
        int dstX = uniqueCopies[i].pos.x - g_state.cursorHotspot.x - pad - boundLeft;
        int dstY = uniqueCopies[i].pos.y - g_state.cursorHotspot.y - pad - boundTop;
        
        if (dstX + stampW <= 0 || dstX >= boundsWidth || dstY + stampH <= 0 || dstY >= boundsHeight) {
            continue;
        }
        
        blendFunc.SourceConstantAlpha = uniqueCopies[i].alpha;
        AlphaBlend(g_state.composeDC, dstX, dstY, stampW, stampH,
                   g_state.stampDC, 0, 0, stampW, stampH, blendFunc);
        if (g_state.numLastRenderedStamps < MAX_TRAIL_COPIES) {
            g_state.lastRenderedStamps[g_state.numLastRenderedStamps++] = { dstX, dstY, stampW, stampH };
        }
    }
    
    POINT ptDst = { boundLeft, boundTop };
    SIZE szWindow = { boundsWidth, boundsHeight };
    POINT ptSrc = { 0, 0 };
    
    BLENDFUNCTION winBlendFunc = {};
    winBlendFunc.BlendOp = AC_SRC_OVER;
    winBlendFunc.SourceConstantAlpha = 255;
    winBlendFunc.AlphaFormat = AC_SRC_ALPHA;
    
    HDC screenDC = GetDC(nullptr);
    UpdateLayeredWindow(g_state.overlayWindow, screenDC, &ptDst, &szWindow, 
                        g_state.composeDC, &ptSrc, 0, &winBlendFunc, ULW_ALPHA);
    ReleaseDC(nullptr, screenDC);
    // Z-Order Enforcement: Ensure the overlay stays on top of other TOPMOST windows (throttled to 500ms)
    static LARGE_INTEGER lastZOrderCheck = {{0}};
    LARGE_INTEGER zNow;
    QueryPerformanceCounter(&zNow);
    if ((zNow.QuadPart - lastZOrderCheck.QuadPart) * 1000 / g_state.performanceFreq.QuadPart >= 500) {
        HWND topWnd = GetTopWindow(nullptr);
        if (topWnd != g_state.overlayWindow) {
            SetWindowPos(g_state.overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, 
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
        lastZOrderCheck = zNow;
    }
    
    g_state.isTrailVisible = true;
}

// --- Window & Lifecycle Management ---

static LRESULT CALLBACK OverlayWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_NCHITTEST:
            return HTTRANSPARENT; // Let mouse events pass through
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static bool CreateOverlayWindow() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    ATOM atom = RegisterClassExW(&wc);
    if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }
    
    DWORD exStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | 
                    WS_EX_LAYERED | WS_EX_TRANSPARENT;
                    
    g_state.overlayWindow = CreateWindowExW(exStyle, WINDOW_CLASS_NAME, L"CursorMotionBlurOverlay", 
                                            WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, wc.hInstance, nullptr);
                                            
    if (!g_state.overlayWindow) {
        return false;
    }
    
    // Optmization: Keep window permanently mapped in DWM to prevent Win32 background flashing 
    ShowWindow(g_state.overlayWindow, SW_SHOWNOACTIVATE);
    
    Wh_Log(L"Overlay window created successfully.");
    return true;
}

static void DestroyOverlayWindow() {
    if (g_state.overlayWindow) {
        DestroyWindow(g_state.overlayWindow);
        g_state.overlayWindow = nullptr;
    }
    UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandleW(nullptr));
}

static void RenderThread() {
    // Increase system timer resolution for smooth frame pacing (1ms)
    timeBeginPeriod(1);
    
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE); // Force message queue creation
    
    QueryPerformanceFrequency(&g_state.performanceFreq);
    g_state.ringBuffer.Reset();
    LoadConfig();
    
    if (!CreateOverlayWindow()) {
        timeEndPeriod(1);
        return;
    }
    
    g_state.mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, GetModuleHandleW(nullptr), 0);
    if (!g_state.mouseHook) {
        Wh_Log(L"Warning: Failed to set mouse hook. Falling back strictly to UIPI-bypass polling.");
    } else {
        Wh_Log(L"Render thread initialized and hooked.");
    }
    
    g_state.isRunning = true;
    
    LARGE_INTEGER lastRenderTime = {{0}};
    QueryPerformanceCounter(&lastRenderTime);
    LONG lastSeenHead = g_state.ringBuffer.GetHead();
    
    // Trail-fade mechanism to let the trail decay naturally after cursor stops
    int tailFramesRemaining = 0;
    
    while (g_state.isRunning) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT || msg.message == WM_EXIT_THREAD) {
                g_state.isRunning = false;
                break;
            }
            if (msg.message == WM_RELOAD_CONFIG) {
                LoadConfig();
                RenderTrail();
                QueryPerformanceCounter(&lastRenderTime);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (!g_state.isRunning) break;
        
        // Universal UIPI-Bypass Polling: Checks physical mouse movement directly,
        // circumventing any event drops caused by High-IL (Admin) windows stealing focus.
        POINT pt;
        if (GetCursorPos(&pt)) {
            PositionRecord lastRec;
            if (!g_state.ringBuffer.GetLast(&lastRec) || pt.x != lastRec.point.x || pt.y != lastRec.point.y) {
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                g_state.ringBuffer.Push(pt, now);
            }
        }
        
        LONG currentHead = g_state.ringBuffer.GetHead();
        bool isNewInput = (currentHead != lastSeenHead);
        
        if (isNewInput && g_state.config.dynamicRefreshScaling) {
            RefreshMonitorState(pt);
        }
        
        int targetFps = g_state.config.dynamicRefreshScaling ? g_state.currentRefreshRate : g_state.config.fps;
        targetFps = Clamp(targetFps, 5, 240);
        LONGLONG frameTicks = g_state.performanceFreq.QuadPart / targetFps;
        
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        LONGLONG elapsedTicks = now.QuadPart - lastRenderTime.QuadPart;
        
        if (elapsedTicks >= frameTicks) {
            if (isNewInput) {
                tailFramesRemaining = 10; // Extra frames to flush trail
                RenderTrail();
                lastRenderTime = now;
                lastSeenHead = currentHead;
            } else if (tailFramesRemaining > 0) {
                tailFramesRemaining--;
                RenderTrail();
                lastRenderTime = now;
            } else {
                // Keep the head updated so old movement isn't mistaken as new movement later
                lastSeenHead = currentHead;
            }
        }
        DWORD waitMs;
        if (isNewInput) {
            waitMs = 2; // 500Hz polling for UIPI bypass while moving
        } else if (tailFramesRemaining > 0) {
            LONGLONG remainingTicks = frameTicks - elapsedTicks;
            waitMs = (remainingTicks > 0) ? (DWORD)(remainingTicks * 1000 / g_state.performanceFreq.QuadPart) : 0;
            if (waitMs == 0) waitMs = 1;
        } else {
            waitMs = 16; // Low-frequency 60Hz polling when idle
        }
        
        MsgWaitForMultipleObjectsEx(0, nullptr, waitMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
    
    if (g_state.mouseHook) {
        UnhookWindowsHookEx(g_state.mouseHook);
        g_state.mouseHook = nullptr;
    }
    
    if (g_state.cursorIcon) {
        DestroyIcon(g_state.cursorIcon);
        g_state.cursorIcon = nullptr;
    }
    
    FreeStampBuffer();
    FreeComposeBuffer();
    DestroyOverlayWindow();
    
    timeEndPeriod(1);
    Wh_Log(L"Render thread exited safely.");
}

// --- Windhawk Integration & Tool Bootstrapper ---

bool g_isToolMod = false;
bool g_isWindhawkMain = false;
bool g_isNativeHost = false;
bool g_startupFailed = false;
HANDLE g_mutex = nullptr;

// Entry point hook to prevent the injected target from fully initializing when launched as a tool mod
void WINAPI EntryPointHook() {
    ExitThread(0);
}

BOOL WhTool_ModInit() {
    g_state.renderThread = std::thread(RenderThread);
    g_state.threadId = GetThreadId((HANDLE)g_state.renderThread.native_handle());
    return TRUE;
}

void WhTool_ModUninit() {
    if (g_state.threadId) {
        PostThreadMessage(g_state.threadId, WM_EXIT_THREAD, 0, 0);
        PostThreadMessage(g_state.threadId, WM_QUIT, 0, 0);
    }
    
    if (g_state.renderThread.joinable()) {
        DWORD t0 = GetTickCount();
        while (g_state.isRunning && (GetTickCount() - t0 < 1000)) {
            Sleep(10);
        }
        if (g_state.isRunning) {
            g_state.renderThread.detach(); // Fallback
        } else {
            g_state.renderThread.join();
        }
    }
    g_state.isRunning = false;
}

void WhTool_ModSettingsChanged() {
    if (g_state.threadId) {
        PostThreadMessage(g_state.threadId, WM_RELOAD_CONFIG, 0, 0);
    }
}

// Setup the mod logic: only spawn the overlay in a detached process context
BOOL Wh_ModInit() {
    WCHAR processPath[MAX_PATH];
    DWORD pathLen = GetModuleFileNameW(nullptr, processPath, ARRAYSIZE(processPath));
    if (pathLen == 0 || pathLen >= ARRAYSIZE(processPath)) {
        g_isWindhawkMain = true;
        return TRUE;
    }
    
    const WCHAR* exeName = wcsrchr(processPath, L'\\');
    exeName = exeName ? exeName + 1 : processPath;
    
    // 1. Run natively inside Secure Desktop and UWP overlay processes
    if (_wcsicmp(exeName, L"LockApp.exe") == 0 ||
        _wcsicmp(exeName, L"LogonUI.exe") == 0 ||
        _wcsicmp(exeName, L"consent.exe") == 0) {
        
        g_isNativeHost = true;
        
        // Prevent multiple threads if Windhawk reinjects, using a process-specific mutex
        WCHAR mutexName[512];
        swprintf_s(mutexName, L"windhawk-native-mod_%s_%s", WH_MOD_ID, exeName);
        g_mutex = CreateMutexW(nullptr, TRUE, mutexName);
        if (!g_mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
            return TRUE;
        }
        
        WhTool_ModInit();
        return TRUE;
    }
    
    // 2. Tool Mod Logic for normal user desktop
    if (_wcsicmp(exeName, L"windhawk.exe") != 0) {
        g_isWindhawkMain = true;
        return TRUE;
    }
    
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        g_isWindhawkMain = true;
        return TRUE;
    }
    
    bool isService = false;
    bool isToolCmd = false;
    bool isThisMod = false;
    
    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-service") == 0) {
            isService = true;
            break;
        }
    }
    for (int i = 1; i < argc - 1; i++) {
        if (wcscmp(argv[i], L"-tool-mod") == 0) {
            isToolCmd = true;
            if (wcscmp(argv[i + 1], WH_MOD_ID) == 0) {
                isThisMod = true;
            }
            break;
        }
    }
    LocalFree(argv);
    
    if (isService) {
        g_isWindhawkMain = true;
        return TRUE;
    }
    
    if (isThisMod) {
        // Prevent multiple instances per desktop
        WCHAR desktopName[256] = {0};
        HDESK hDesk = GetThreadDesktop(GetCurrentThreadId());
        if (hDesk) {
            GetUserObjectInformationW(hDesk, UOI_NAME, desktopName, sizeof(desktopName), nullptr);
        }
        WCHAR mutexName[512];
        swprintf_s(mutexName, L"windhawk-tool-mod_%s_%s", WH_MOD_ID, desktopName[0] ? desktopName : L"default");
        g_mutex = CreateMutexW(nullptr, TRUE, mutexName);
        if (!g_mutex) {
            g_startupFailed = true;
        }
        
        if (!g_startupFailed && GetLastError() == ERROR_ALREADY_EXISTS) {
            DWORD waitRes = WaitForSingleObject(g_mutex, 2000);
            if (waitRes != WAIT_OBJECT_0 && waitRes != WAIT_ABANDONED) {
                g_startupFailed = true;
            }
        }
        
        if (!g_startupFailed && !WhTool_ModInit()) {
            g_startupFailed = true;
        }
        
        // Prevent normal Windhawk UI from loading by hooking entry point
        auto* dosHeader = (IMAGE_DOS_HEADER*)GetModuleHandleW(nullptr);
        auto* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);
        void* entryPoint = (BYTE*)dosHeader + ntHeaders->OptionalHeader.AddressOfEntryPoint;
        Wh_SetFunctionHook(entryPoint, (void*)EntryPointHook, nullptr);
        
        return TRUE;
    }
    
    if (isToolCmd) {
        g_isWindhawkMain = true;
        return TRUE;
    }
    
    g_isToolMod = true;
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_isToolMod) return;
    
    WCHAR exePath[MAX_PATH];
    DWORD pathLen = GetModuleFileNameW(nullptr, exePath, ARRAYSIZE(exePath));
    if (pathLen == 0 || pathLen >= ARRAYSIZE(exePath)) return;
    
    WCHAR cmdLine[MAX_PATH + 64];
    swprintf_s(cmdLine, L"\"%s\" -tool-mod \"%s\"", exePath, WH_MOD_ID);
    
    HMODULE kernelMod = GetModuleHandleW(L"kernelbase.dll");
    if (!kernelMod) kernelMod = GetModuleHandleW(L"kernel32.dll");
    if (!kernelMod) return;
    
    using CreateProcFunc = BOOL(WINAPI*)(HANDLE, LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES,
        LPSECURITY_ATTRIBUTES, WINBOOL, DWORD, LPVOID, LPCWSTR,
        LPSTARTUPINFOW, LPPROCESS_INFORMATION, PHANDLE);
        
    auto fnCreateProcessInternalW = (CreateProcFunc)GetProcAddress(kernelMod, "CreateProcessInternalW");
    if (!fnCreateProcessInternalW) return;
    
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_FORCEOFFFEEDBACK;
    PROCESS_INFORMATION pi;
    
    if (fnCreateProcessInternalW(nullptr, exePath, cmdLine, nullptr, nullptr, FALSE, 
                                 NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi, nullptr)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void Wh_ModSettingsChanged() {
    if (g_isToolMod || g_isWindhawkMain) return;
    WhTool_ModSettingsChanged();
}

void Wh_ModUninit() {
    if (g_isToolMod || g_isWindhawkMain) return;
    WhTool_ModUninit();
    
    // For native hosts (LockApp, consent, LogonUI), we only want to stop our thread and clean up.
    // Calling ExitProcess would kill the lock screen or UAC prompt entirely!
    if (!g_isNativeHost) {
        ExitProcess(0);
    }
}