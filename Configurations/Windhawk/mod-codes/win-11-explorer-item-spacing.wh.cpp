// ==WindhawkMod==
// @id              win-11-explorer-item-spacing
// @name            Windows 11 Explorer Item Spacing
// @description     Applies Windows 11-like File Explorer item spacing safely on Windows 10 by enabling native touch padding.
// @version         1.0
// @author          Lone
// @github          https://github.com/Louis047
// @include         explorer.exe
// @compilerOptions -luser32 -lcomctl32 -lgdi32
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*
- navPaneSpacing: 10
  $name: Navigation Pane Spacing
  $description: Row height in pixels for the left folder tree (Windows 10 default is 24, Windows 11 is 32).
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <commctrl.h>
#include <windhawk_api.h>
#include <windows.ui.viewmanagement.h>
#include <roapi.h>
#include <winstring.h>

int g_navPaneSpacing = 10;

// -------------------------------------------------------------------------
// UI Isolation Checker
// -------------------------------------------------------------------------

bool ShouldSpoofTabletMode() {
    void* caller = __builtin_return_address(0);
    HMODULE hModule;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)caller, &hModule)) {
        wchar_t moduleName[MAX_PATH];
        if (GetModuleFileNameW(hModule, moduleName, MAX_PATH)) {
            // Check if the caller is ExplorerFrame.dll, windows.storage.dll, or dui70.dll
            if (wcsstr(moduleName, L"ExplorerFrame.dll") || 
                wcsstr(moduleName, L"windows.storage.dll") || 
                wcsstr(moduleName, L"dui70.dll")) {
                return true;
            }
        }
    }
    return false;
}

bool IsInCabinetWClass(HWND hwnd) {
    if (!hwnd) return false;
    HWND hRoot = GetAncestor(hwnd, GA_ROOT);
    if (!hRoot) return false;
    wchar_t className[256];
    if (GetClassNameW(hRoot, className, ARRAYSIZE(className))) {
        return wcscmp(className, L"CabinetWClass") == 0;
    }
    return false;
}

// -------------------------------------------------------------------------
// Navigation Pane Forcing
// -------------------------------------------------------------------------

void EnforceTreeViewSpacing(HWND hWnd) {
    int currentHeight = (int)SendMessageW(hWnd, TVM_GETITEMHEIGHT, 0, 0);
    if (currentHeight > 0 && currentHeight != g_navPaneSpacing) {
        SendMessageW(hWnd, TVM_SETITEMHEIGHT, g_navPaneSpacing, 0);
    }
}

LRESULT CALLBACK ExplorerTreeViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == TVM_SETITEMHEIGHT && (int)wParam == g_navPaneSpacing) {
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
    if (uMsg == WM_WINDOWPOSCHANGED || uMsg == TVM_SETITEMHEIGHT) {
        EnforceTreeViewSpacing(hWnd);
    }
    return res;
}

// -------------------------------------------------------------------------
// Tablet Mode Spoofing (Details Pane Forcing)
// -------------------------------------------------------------------------

using get_UserInteractionMode_t = HRESULT(WINAPI*)(ABI::Windows::UI::ViewManagement::IUIViewSettings*, ABI::Windows::UI::ViewManagement::UserInteractionMode*);
get_UserInteractionMode_t g_get_UserInteractionMode_Orig;

HRESULT WINAPI get_UserInteractionMode_Hook(ABI::Windows::UI::ViewManagement::IUIViewSettings* pThis, ABI::Windows::UI::ViewManagement::UserInteractionMode* value) {
    HRESULT hr = g_get_UserInteractionMode_Orig(pThis, value);
    if (SUCCEEDED(hr) && value) {
        if (ShouldSpoofTabletMode()) {
            *value = ABI::Windows::UI::ViewManagement::UserInteractionMode_Touch;
        }
    }
    return hr;
}

using GetSystemMetrics_t = int (WINAPI*)(int);
GetSystemMetrics_t g_GetSystemMetrics_Orig;

int WINAPI GetSystemMetrics_Hook(int nIndex) {
    if (nIndex == SM_CONVERTIBLESLATEMODE) {
        if (ShouldSpoofTabletMode()) {
            return 0; // 0 = Slate Mode (Tablet Mode)
        }
    }
    return g_GetSystemMetrics_Orig(nIndex);
}

using RegGetValueW_t = LSTATUS (WINAPI*)(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);
RegGetValueW_t g_RegGetValueW_Orig;

LSTATUS WINAPI RegGetValueW_Hook(HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData) {
    LSTATUS status = g_RegGetValueW_Orig(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
    if (lpValue && wcscmp(lpValue, L"TabletMode") == 0) {
        if (ShouldSpoofTabletMode() && pvData && pcbData && *pcbData >= sizeof(DWORD)) {
            *(DWORD*)pvData = 1; // 1 = Tablet Mode
            status = ERROR_SUCCESS;
        }
    }
    return status;
}

using RegQueryValueExW_t = LSTATUS (WINAPI*)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
RegQueryValueExW_t g_RegQueryValueExW_Orig;

LSTATUS WINAPI RegQueryValueExW_Hook(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    LSTATUS status = g_RegQueryValueExW_Orig(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    if (lpValueName && wcscmp(lpValueName, L"TabletMode") == 0) {
        if (ShouldSpoofTabletMode() && lpData && lpcbData && *lpcbData >= sizeof(DWORD)) {
            *(DWORD*)lpData = 1; // 1 = Tablet Mode
            status = ERROR_SUCCESS;
        }
    }
    return status;
}

// -------------------------------------------------------------------------
// Safe COM Initialization
// -------------------------------------------------------------------------

void HookWinRT(HWND hwnd) {
    static bool s_winrtHooked = false;
    if (s_winrtHooked) return;
    s_winrtHooked = true;

    HMODULE hCombase = GetModuleHandleW(L"combase.dll");
    if (!hCombase) return;

    auto pRoGetActivationFactory = (HRESULT(WINAPI*)(HSTRING, REFIID, void**))GetProcAddress(hCombase, "RoGetActivationFactory");
    auto pWindowsCreateStringReference = (HRESULT(WINAPI*)(PCWSTR, UINT32, HSTRING_HEADER*, HSTRING*))GetProcAddress(hCombase, "WindowsCreateStringReference");

    if (!pRoGetActivationFactory || !pWindowsCreateStringReference) return;

    HSTRING_HEADER header;
    HSTRING hstr = nullptr;
    const wchar_t* className = L"Windows.UI.ViewManagement.UIViewSettings";
    if (FAILED(pWindowsCreateStringReference(className, wcslen(className), &header, &hstr))) return;

    IInspectable* factory = nullptr;
    // IID_IUIViewSettingsInterop: {3694dbf9-8f68-44be-8ff5-195c98ede8a6}
    const IID IID_IUIViewSettingsInterop = { 0x3694dbf9, 0x8f68, 0x44be, { 0x8f, 0xf5, 0x19, 0x5c, 0x98, 0xed, 0xe8, 0xa6 } };
    
    if (SUCCEEDED(pRoGetActivationFactory(hstr, IID_IUIViewSettingsInterop, (void**)&factory))) {
        // We know the factory implements IUIViewSettingsInterop
        struct IUIViewSettingsInterop_Manual {
            virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) = 0;
            virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
            virtual ULONG STDMETHODCALLTYPE Release() = 0;
            virtual HRESULT STDMETHODCALLTYPE GetInspectableIDs(ULONG* iidCount, IID** iids) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING* className) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel* trustLevel) = 0;
            virtual HRESULT STDMETHODCALLTYPE GetForWindow(HWND appWindow, REFIID riid, void** ppv) = 0;
        };
        
        IUIViewSettingsInterop_Manual* interop = (IUIViewSettingsInterop_Manual*)factory;
        ABI::Windows::UI::ViewManagement::IUIViewSettings* settings = nullptr;
        
        // IID_IUIViewSettings: {c63657f6-8850-470d-88f8-452747c6696b}
        const IID IID_IUIViewSettings = { 0xc63657f6, 0x8850, 0x470d, { 0x88, 0xf8, 0x45, 0x27, 0x47, 0xc6, 0x69, 0x6b } };
        
        if (SUCCEEDED(interop->GetForWindow(hwnd, IID_IUIViewSettings, (void**)&settings))) {
            void** vtable = *(void***)settings;
            Wh_SetFunctionHook(vtable[6], (void*)get_UserInteractionMode_Hook, (void**)&g_get_UserInteractionMode_Orig);
            settings->Release();
            Wh_Log(L"Successfully hooked UIViewSettings::get_UserInteractionMode!");
        } else {
            // Fallback to Desktop Window
            if (SUCCEEDED(interop->GetForWindow(GetDesktopWindow(), IID_IUIViewSettings, (void**)&settings))) {
                void** vtable = *(void***)settings;
                Wh_SetFunctionHook(vtable[6], (void*)get_UserInteractionMode_Hook, (void**)&g_get_UserInteractionMode_Orig);
                settings->Release();
                Wh_Log(L"Successfully hooked UIViewSettings::get_UserInteractionMode via DesktopWindow!");
            }
        }
        interop->Release();
    }
}

using CreateWindowExW_t = HWND (WINAPI*)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
CreateWindowExW_t g_CreateWindowExW_Orig;

HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    HWND hwnd = g_CreateWindowExW_Orig(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    
    if (hwnd && lpClassName && !IS_INTRESOURCE(lpClassName)) {
        if (wcscmp(lpClassName, L"CabinetWClass") == 0) {
            // Hook WinRT ONLY when File Explorer creates its main window.
            // This runs on the UI thread, completely avoiding Loader Lock crashes!
            HookWinRT(hwnd);
            
            // Force a refresh of the window to apply the Tablet Mode spacing immediately
            PostMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"ConvertibleSlateMode");
            PostMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"UserInteractionMode");
        }
        else if (wcscmp(lpClassName, WC_TREEVIEWW) == 0) {
            if (IsInCabinetWClass(hwnd)) {
                SetWindowSubclass(hwnd, ExplorerTreeViewSubclassProc, 1, 0);
            }
        }
    }
    return hwnd;
}

// -------------------------------------------------------------------------
// Initialization
// -------------------------------------------------------------------------

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    wchar_t className[256];
    if (GetClassNameW(hwnd, className, ARRAYSIZE(className))) {
        if (wcscmp(className, WC_TREEVIEWW) == 0) {
            SetWindowSubclass(hwnd, ExplorerTreeViewSubclassProc, 1, 0);
            EnforceTreeViewSpacing(hwnd);
        }
    }
    return TRUE;
}

void SubclassExistingWindows() {
    HWND hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"CabinetWClass", nullptr)) != nullptr) {
        HookWinRT(hwnd);
        EnumChildWindows(hwnd, EnumChildProc, 0);
        PostMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"ConvertibleSlateMode");
        PostMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"UserInteractionMode");
    }
}

void LoadSettings() {
    g_navPaneSpacing = Wh_GetIntSetting(L"navPaneSpacing");
}

void Wh_ModSettingsChanged() {
    LoadSettings();
    SubclassExistingWindows();
}

BOOL Wh_ModInit() {
    Wh_Log(L"Windows 11 Explorer Spacing: Init V4.2 Final Fix");
    LoadSettings();

    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        void* pGetSystemMetrics = (void*)GetProcAddress(hUser32, "GetSystemMetrics");
        if (pGetSystemMetrics) {
            Wh_SetFunctionHook(pGetSystemMetrics, (void*)GetSystemMetrics_Hook, (void**)&g_GetSystemMetrics_Orig);
        }
        void* pCreateWindowExW = (void*)GetProcAddress(hUser32, "CreateWindowExW");
        if (pCreateWindowExW) {
            Wh_SetFunctionHook(pCreateWindowExW, (void*)CreateWindowExW_Hook, (void**)&g_CreateWindowExW_Orig);
        }
    }

    HMODULE hAdvapi32 = GetModuleHandleW(L"advapi32.dll");
    if (hAdvapi32) {
        void* pRegGetValueW = (void*)GetProcAddress(hAdvapi32, "RegGetValueW");
        if (pRegGetValueW) {
            Wh_SetFunctionHook(pRegGetValueW, (void*)RegGetValueW_Hook, (void**)&g_RegGetValueW_Orig);
        }
        void* pRegQueryValueExW = (void*)GetProcAddress(hAdvapi32, "RegQueryValueExW");
        if (pRegQueryValueExW) {
            Wh_SetFunctionHook(pRegQueryValueExW, (void*)RegQueryValueExW_Hook, (void**)&g_RegQueryValueExW_Orig);
        }
    }
    
    // Attempt to hook WinRT and subclass immediately if Explorer windows are already open
    SubclassExistingWindows();
    
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Windows 11 Explorer Spacing: Uninit V4.2");
}
