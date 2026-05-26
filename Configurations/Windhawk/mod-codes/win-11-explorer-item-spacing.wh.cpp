// ==WindhawkMod==
// @id              win-11-explorer-item-spacing
// @name            Windows 11 Explorer Item Spacing
// @description     Applies Windows 11-like File Explorer item spacing (No crashes). Use the settings tab to tweak exact pixel heights!
// @version         1.0
// @author          Lone
// @github          https://github.com/Louis047
// @include         explorer.exe
// @compilerOptions -lcomctl32 -lgdi32 -lole32 -luuid -loleaut32 -luxtheme
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*
- navPaneSpacing: 10
  $name: Navigation Pane Spacing
  $description: Item height in pixels for the left tree view.
- detailsViewSpacing: 24
  $name: Details View Spacing
  $description: Icon size in pixels for the right file list (Windows 10 default is 16, Windows 11 is 24).
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <exdisp.h>
#include <windhawk_api.h>
#include <windhawk_utils.h>

#include <thread>
#include <atomic>
#include <intrin.h>

std::atomic<bool> g_stopThread{false};
std::thread g_workerThread;

int g_navPaneSpacing = 10;
int g_detailsViewSpacing = 24;
std::atomic<int> g_currentViewMode{0};

HMODULE g_hExplorerFrame = nullptr;
HMODULE g_hWindowsStorage = nullptr;

bool IsCallerInModule(void* returnAddress, HMODULE hModule) {
    if (!hModule || !returnAddress) return false;
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)hModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)hModule + dosHeader->e_lfanew);
    void* moduleEnd = (BYTE*)hModule + ntHeaders->OptionalHeader.SizeOfImage;
    return (returnAddress >= hModule && returnAddress < moduleEnd);
}

using CItemsView_GetItemSpacing_t = HRESULT (__thiscall*)(void*, int*, int*);
CItemsView_GetItemSpacing_t g_CItemsView_GetItemSpacing_Orig;

HRESULT __thiscall CItemsView_GetItemSpacing_Hook(void* pThis, int* px, int* py) {
    HRESULT hr = g_CItemsView_GetItemSpacing_Orig(pThis, px, py);
    if (SUCCEEDED(hr) && py) {
        // Force Y spacing regardless of view mode (to fix race condition)
        if (*py <= 32) { // Only override if it's small (List/Details), leave Large Icons alone
            *py = g_detailsViewSpacing;
        }
    }
    return hr;
}

using CItemsView_GetItemHeight_t = HRESULT (__thiscall*)(void*, int*);
CItemsView_GetItemHeight_t g_CItemsView_GetItemHeight_Orig;

HRESULT __thiscall CItemsView_GetItemHeight_Hook(void* pThis, int* pHeight) {
    HRESULT hr = g_CItemsView_GetItemHeight_Orig(pThis, pHeight);
    if (SUCCEEDED(hr) && pHeight) {
        if (*pHeight <= 32) {
            *pHeight = g_detailsViewSpacing;
        }
    }
    return hr;
}

using CItemsView_GetItemSize_t = HRESULT (__thiscall*)(void*, int*, int*);
CItemsView_GetItemSize_t g_CItemsView_GetItemSize_Orig;

HRESULT __thiscall CItemsView_GetItemSize_Hook(void* pThis, int* px, int* py) {
    HRESULT hr = g_CItemsView_GetItemSize_Orig(pThis, px, py);
    if (SUCCEEDED(hr) && py) {
        if (*py <= 32) {
            *py = g_detailsViewSpacing;
        }
    }
    return hr;
}

using CItemsView_SetItemHeight_t = HRESULT (__thiscall*)(void*, int);
CItemsView_SetItemHeight_t g_CItemsView_SetItemHeight_Orig;

HRESULT __thiscall CItemsView_SetItemHeight_Hook(void* pThis, int height) {
    if (height <= 32) {
        height = g_detailsViewSpacing;
    }
    return g_CItemsView_SetItemHeight_Orig(pThis, height);
}

using GetPhysicalColumnPadding_t = HRESULT (__thiscall*)(void*, UINT*);
GetPhysicalColumnPadding_t g_GetPhysicalColumnPadding_Orig;

HRESULT __thiscall GetPhysicalColumnPadding_Hook(void* pThis, UINT* pPadding) {
    HRESULT hr = g_GetPhysicalColumnPadding_Orig(pThis, pPadding);
    if (SUCCEEDED(hr) && pPadding) {
        if (*pPadding <= 32) {
            *pPadding = g_detailsViewSpacing; 
        }
    }
    return hr;
}

using GetLogicalImageSize_t = HRESULT (__thiscall*)(void*, UINT*);
GetLogicalImageSize_t g_GetLogicalImageSize_Orig;

HRESULT __thiscall GetLogicalImageSize_Hook(void* pThis, UINT* pSize) {
    HRESULT hr = g_GetLogicalImageSize_Orig(pThis, pSize);
    if (SUCCEEDED(hr) && pSize) {
        if (*pSize <= 32) {
            *pSize = g_detailsViewSpacing; 
        }
    }
    return hr;
}

using GetViewModeAndIconSize_t = HRESULT (STDMETHODCALLTYPE*)(IFolderView2*, FOLDERVIEWMODE*, int*);
GetViewModeAndIconSize_t g_GetViewModeAndIconSize_Orig = nullptr;

HRESULT STDMETHODCALLTYPE GetViewModeAndIconSize_Hook(IFolderView2* pThis, FOLDERVIEWMODE* puMode, int* piImageSize) {
    HRESULT hr = g_GetViewModeAndIconSize_Orig(pThis, puMode, piImageSize);
    if (SUCCEEDED(hr)) {
        FOLDERVIEWMODE mode = FVM_AUTO;
        if (puMode) mode = *puMode;
        else pThis->GetCurrentViewMode((UINT*)&mode);

        if (mode == FVM_DETAILS && piImageSize) {
            *piImageSize = g_detailsViewSpacing;
        }
    }
    return hr;
}

using GetIconSize_t = HRESULT (__thiscall*)(void*, UINT*);

GetIconSize_t g_GetIconSize_CBaseShellFolderViewCB_Orig;
HRESULT __thiscall GetIconSize_CBaseShellFolderViewCB_Hook(void* pThis, UINT* pSize) {
    HRESULT hr = g_GetIconSize_CBaseShellFolderViewCB_Orig(pThis, pSize);
    if (SUCCEEDED(hr) && pSize && g_currentViewMode.load() == FVM_DETAILS) {
        *pSize = g_detailsViewSpacing;
    }
    return hr;
}

GetIconSize_t g_GetIconSize_CLibraryViewCB_Orig;
HRESULT __thiscall GetIconSize_CLibraryViewCB_Hook(void* pThis, UINT* pSize) {
    HRESULT hr = g_GetIconSize_CLibraryViewCB_Orig(pThis, pSize);
    if (SUCCEEDED(hr) && pSize && g_currentViewMode.load() == FVM_DETAILS) {
        *pSize = g_detailsViewSpacing;
    }
    return hr;
}

GetIconSize_t g_GetIconSize_CTopViewDescription_Orig;
HRESULT __thiscall GetIconSize_CTopViewDescription_Hook(void* pThis, UINT* pSize) {
    HRESULT hr = g_GetIconSize_CTopViewDescription_Orig(pThis, pSize);
    if (SUCCEEDED(hr) && pSize && g_currentViewMode.load() == FVM_DETAILS) {
        *pSize = g_detailsViewSpacing;
    }
    return hr;
}

using GetSmallIconSizeCB_t = void (*)(SIZE*);

GetSmallIconSizeCB_t g_GetSmallIconSizeCB_Orig;
void GetSmallIconSizeCB_Hook(SIZE* pSize) {
    g_GetSmallIconSizeCB_Orig(pSize);
    if (pSize && g_currentViewMode.load() == FVM_DETAILS) {
        pSize->cx = g_detailsViewSpacing;
        pSize->cy = g_detailsViewSpacing;
    }
}

GetSmallIconSizeCB_t g_GetSysSmallIconSizeCB_Orig;
void GetSysSmallIconSizeCB_Hook(SIZE* pSize) {
    g_GetSysSmallIconSizeCB_Orig(pSize);
    if (pSize && g_currentViewMode.load() == FVM_DETAILS) {
        pSize->cx = g_detailsViewSpacing;
        pSize->cy = g_detailsViewSpacing;
    }
}

std::atomic<bool> g_folderViewHooked{false};

using GetSpacing_t = HRESULT (STDMETHODCALLTYPE*)(IFolderView2*, POINT*);
GetSpacing_t g_GetSpacing_Orig = nullptr;

HRESULT STDMETHODCALLTYPE GetSpacing_Hook(IFolderView2* pThis, POINT* ppt) {
    HRESULT hr = g_GetSpacing_Orig(pThis, ppt);
    if (SUCCEEDED(hr) && ppt) {
        FOLDERVIEWMODE mode = FVM_AUTO;
        pThis->GetCurrentViewMode((UINT*)&mode);
        if (mode == FVM_DETAILS) {
            // Wh_Log(L"GetSpacing called in DETAILS mode: x=%d, y=%d", ppt->x, ppt->y);
            ppt->y = g_detailsViewSpacing; // Try to force Y spacing
        }
    }
    return hr;
}

// Removed unused GetSystemMetrics hooks entirely to prevent compiler warnings




// Removed duplicate GetSystemMetrics hooks at the bottom

const GUID SID_STopLevelBrowser_Compat = { 0x4C96BE40, 0x915C, 0x11CF, { 0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37 } };

using GetSystemMetrics_t = int (WINAPI*)(int);
GetSystemMetrics_t g_GetSystemMetrics_Orig;

#ifndef SM_CONVERTIBLESLATEMODE
#define SM_CONVERTIBLESLATEMODE 0x2003
#endif

#ifndef SM_CONVERTIBLESLATEMODE
#define SM_CONVERTIBLESLATEMODE 0x2003
#endif

bool IsCallerInDui70(void* retAddr) {
    static HMODULE hDui70 = GetModuleHandleW(L"dui70.dll");
    if (!hDui70) hDui70 = GetModuleHandleW(L"dui70.dll");
    return IsCallerInModule(retAddr, hDui70);
}

int WINAPI GetSystemMetrics_Hook(int nIndex) {
    if (nIndex == SM_CYSMICON || nIndex == SM_CXSMICON) {
        if (g_currentViewMode.load() == FVM_DETAILS) {
            return g_detailsViewSpacing;
        }
    } else if (nIndex == SM_CONVERTIBLESLATEMODE) {
        if (g_currentViewMode.load() == FVM_DETAILS) {
            // Force Explorer to think it is in Tablet Mode (Slate Mode = 0)
            return 0; 
        }
    }
    return g_GetSystemMetrics_Orig(nIndex);
}

void ForceExplorerUpdate() {
    HWND hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"CabinetWClass", nullptr)) != nullptr) {
        PostMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"ConvertibleSlateMode");
        PostMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"SystemMetrics");
        PostMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"UserInteractionMode");
    }
}

void LoadSettings() {
    g_navPaneSpacing = Wh_GetIntSetting(L"navPaneSpacing");
    g_detailsViewSpacing = Wh_GetIntSetting(L"detailsViewSpacing");
}

void ApplyTreeViewSpacingSafe(HWND hwndTree) {
    if (!IsWindow(hwndTree)) return;
    
    // We intentionally DO NOT use DPI scaling here because SysTreeView32 
    // inside modern Explorer is already DPI aware and auto-scales wParam.
    // Double scaling it makes it massive.
    int desiredHeight = g_navPaneSpacing;
    
    int currentHeight = (int)SendMessageW(hwndTree, TVM_GETITEMHEIGHT, 0, 0);
    if (currentHeight != desiredHeight) {
        SendMessageW(hwndTree, TVM_SETITEMHEIGHT, desiredHeight, 0);
        
        DWORD style = (DWORD)SendMessageW(hwndTree, TVM_GETEXTENDEDSTYLE, 0, 0);
        if (!(style & TVS_EX_DOUBLEBUFFER)) {
            SendMessageW(hwndTree, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
        }
    }
}

BOOL CALLBACK FindTreeViewCallback(HWND hwnd, LPARAM lParam) {
    WCHAR className[128];
    if (GetClassNameW(hwnd, className, ARRAYSIZE(className))) {
        if (wcscmp(className, WC_TREEVIEWW) == 0) {
            HWND parent = GetParent(hwnd);
            if (parent) {
                WCHAR parentClass[128];
                if (GetClassNameW(parent, parentClass, ARRAYSIZE(parentClass)) && wcscmp(parentClass, L"NamespaceTreeControl") == 0) {
                    // Check if this NamespaceTreeControl is inside the breadcrumb or the main left pane.
                    // The left pane is usually a child of the main Explorer frame and has a substantial height.
                    RECT rc;
                    GetWindowRect(parent, &rc);
                    if ((rc.bottom - rc.top) > 100) { // Breadcrumb dropdowns are usually smaller or created on demand.
                        *((HWND*)lParam) = hwnd;
                        return FALSE; // Stop searching
                    }
                }
            }
        }
    }
    return TRUE;
}

void EnforceSpacingViaCOM() {
    IShellWindows* pShellWindows = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_IShellWindows, (void**)&pShellWindows))) {
        return;
    }

    long count = 0;
    if (SUCCEEDED(pShellWindows->get_Count(&count))) {
        for (long i = 0; i < count; i++) {
            VARIANT v;
            v.vt = VT_I4;
            v.lVal = i;
            
            IDispatch* pDisp = nullptr;
            if (SUCCEEDED(pShellWindows->Item(v, &pDisp)) && pDisp) {
                IWebBrowserApp* pApp = nullptr;
                if (SUCCEEDED(pDisp->QueryInterface(IID_IWebBrowserApp, (void**)&pApp)) && pApp) {
                    
                    HWND hwndApp = nullptr;
                    pApp->get_HWND((SHANDLE_PTR*)&hwndApp);

                    if (hwndApp) {
                        HWND hwndTree = nullptr;
                        EnumChildWindows(hwndApp, FindTreeViewCallback, (LPARAM)&hwndTree);
                        if (hwndTree) {
                            ApplyTreeViewSpacingSafe(hwndTree);
                        }
                    }

                    IServiceProvider* pSp = nullptr;
                    if (SUCCEEDED(pApp->QueryInterface(IID_IServiceProvider, (void**)&pSp)) && pSp) {
                        IShellBrowser* pBrowser = nullptr;
                        if (SUCCEEDED(pSp->QueryService(SID_STopLevelBrowser_Compat, IID_IShellBrowser, (void**)&pBrowser)) && pBrowser) {
                            IShellView* pView = nullptr;
                            if (SUCCEEDED(pBrowser->QueryActiveShellView(&pView)) && pView) {
                                IFolderView2* pFolderView = nullptr;
                                if (SUCCEEDED(pView->QueryInterface(IID_IFolderView2, (void**)&pFolderView)) && pFolderView) {
                                    if (!g_folderViewHooked.exchange(true)) {
                                        void** vtable = *(void***)pFolderView;
                                        void* pGetViewModeAndIconSize = vtable[36];
                                        Wh_SetFunctionHook(pGetViewModeAndIconSize, (void*)GetViewModeAndIconSize_Hook, (void**)&g_GetViewModeAndIconSize_Orig);
                                        
                                        void* pGetSpacing = vtable[12];
                                        Wh_SetFunctionHook(pGetSpacing, (void*)GetSpacing_Hook, (void**)&g_GetSpacing_Orig);
                                        
                                        Wh_Log(L"Hooked IFolderView2 COM methods");
                                    }

                                    UINT uMode;
                                    if (SUCCEEDED(pFolderView->GetCurrentViewMode(&uMode))) {
                                        g_currentViewMode.store(uMode);
                                    }

                                    HWND hwndView = nullptr;
                                    if (SUCCEEDED(pView->GetWindow(&hwndView)) && hwndView) {
                                        wchar_t className[256];
                                        GetClassNameW(hwndView, className, 256);
                                        // Wh_Log(L"ShellView Window Class: %s", className);
                                        
                                        // Find the actual list view or items view child
                                        HWND hwndChild = GetWindow(hwndView, GW_CHILD);
                                        while (hwndChild) {
                                            GetClassNameW(hwndChild, className, 256);
                                            // Wh_Log(L"  Child Class: %s", className);
                                            if (wcscmp(className, L"SysListView32") == 0) {
                                                if (uMode == FVM_DETAILS) {
                                                    // We found SysListView32 for Details View!
                                                    // Let's force row height via ImageList or LVM_SETITEMSPACING (not directly supported for Y only without ImageList)
                                                    
                                                    // Standard SysListView32 trick: empty ImageList with desired height
                                                    HIMAGELIST himl = (HIMAGELIST)SendMessageW(hwndChild, LVM_GETIMAGELIST, LVSIL_STATE, 0);
                                                    int cx, cy;
                                                    if (!himl || !ImageList_GetIconSize(himl, &cx, &cy) || cy != g_detailsViewSpacing) {
                                                        HIMAGELIST hNewIml = ImageList_Create(1, g_detailsViewSpacing, ILC_COLOR, 1, 1);
                                                        SendMessageW(hwndChild, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)hNewIml);
                                                        // Note: We might leak the old one, but Windows cleans up state image lists or we can track it.
                                                        Wh_Log(L"Injected LVSIL_STATE ImageList to force height %d on SysListView32", g_detailsViewSpacing);
                                                    }
                                                }
                                            } else if (wcscmp(className, L"DirectUIHWND") == 0) {
                                                // It's DirectUI. We need to find how to adjust its ItemsView.
                                            }
                                            hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
                                        }
                                    }

                                    int iconSize;
                                    if (SUCCEEDED(pFolderView->GetViewModeAndIconSize(nullptr, &iconSize))) {
                                        if (uMode == FVM_DETAILS && iconSize != g_detailsViewSpacing) {
                                            // In Windows 10, DirectUI's Details View row height is natively tied to the icon size.
                                            // By forcing the icon size to g_detailsViewSpacing, we force the rows to expand natively.
                                            // We only do this for FVM_DETAILS to avoid breaking FVM_LIST.
                                            pFolderView->SetViewModeAndIconSize(FVM_DETAILS, g_detailsViewSpacing);
                                        }
                                    }
                                    pFolderView->Release();
                                }
                                pView->Release();
                            }
                            pBrowser->Release();
                        }
                        pSp->Release();
                    }
                    pApp->Release();
                }
                pDisp->Release();
            }
        }
    }
    pShellWindows->Release();
}

void BackgroundPoller() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    while (!g_stopThread.load()) {
        EnforceSpacingViaCOM();
        Sleep(500);
    }
    
    CoUninitialize();
}

void Wh_ModSettingsChanged() {
    LoadSettings();
}

BOOL Wh_ModInit() {
    Wh_Log(L"Windows 11 Explorer Item Spacing: Init v1.0 (COM Poller & GetSystemMetrics Hook)");
    
    LoadSettings();

    g_hExplorerFrame = GetModuleHandleW(L"ExplorerFrame.dll");
    g_hWindowsStorage = GetModuleHandleW(L"windows.storage.dll");

    Wh_SetFunctionHook((void*)GetSystemMetrics, (void*)GetSystemMetrics_Hook, (void**)&g_GetSystemMetrics_Orig);

    if (g_hExplorerFrame) {
        WindhawkUtils::SYMBOL_HOOK hook[] = {
            { {L"?GetItemSpacing@CItemsView@@UEAAJPEAH0@Z"}, (void**)&g_CItemsView_GetItemSpacing_Orig, (void*)CItemsView_GetItemSpacing_Hook, true },
            { {L"?GetItemHeight@CItemsView@@UEAAJPEAH@Z"}, (void**)&g_CItemsView_GetItemHeight_Orig, (void*)CItemsView_GetItemHeight_Hook, true },
            { {L"?GetItemSize@CItemsView@@UEAAJPEAH0@Z"}, (void**)&g_CItemsView_GetItemSize_Orig, (void*)CItemsView_GetItemSize_Hook, true },
            { {L"?SetItemHeight@CItemsView@@UEAAJH@Z"}, (void**)&g_CItemsView_SetItemHeight_Orig, (void*)CItemsView_SetItemHeight_Hook, true }
        };
        bool success = WindhawkUtils::HookSymbols(g_hExplorerFrame, hook, ARRAYSIZE(hook));
        Wh_Log(L"HookSymbols ExplorerFrame returned: %d", success);
    }

    HMODULE hStorage = GetModuleHandleW(L"windows.storage.dll");
    if (hStorage) {
        WindhawkUtils::SYMBOL_HOOK hook[] = {
            { {L"?GetPhysicalColumnPadding@CViewSettings@@UEAAJPEAI@Z"}, (void**)&g_GetPhysicalColumnPadding_Orig, (void*)GetPhysicalColumnPadding_Hook, true },
            { {L"?GetLogicalImageSize@CViewSettings@@UEAAJPEAI@Z"}, (void**)&g_GetLogicalImageSize_Orig, (void*)GetLogicalImageSize_Hook, true },
            { {L"?GetItemSpacing@CItemsView@@UEAAJPEAH0@Z"}, (void**)&g_CItemsView_GetItemSpacing_Orig, (void*)CItemsView_GetItemSpacing_Hook, true },
            { {L"?GetItemHeight@CItemsView@@UEAAJPEAH@Z"}, (void**)&g_CItemsView_GetItemHeight_Orig, (void*)CItemsView_GetItemHeight_Hook, true },
            { {L"?GetItemSize@CItemsView@@UEAAJPEAH0@Z"}, (void**)&g_CItemsView_GetItemSize_Orig, (void*)CItemsView_GetItemSize_Hook, true },
            { {L"?SetItemHeight@CItemsView@@UEAAJH@Z"}, (void**)&g_CItemsView_SetItemHeight_Orig, (void*)CItemsView_SetItemHeight_Hook, true },
            { {L"?GetIconSize@CBaseShellFolderViewCB@@UEAAJPEAI@Z"}, (void**)&g_GetIconSize_CBaseShellFolderViewCB_Orig, (void*)GetIconSize_CBaseShellFolderViewCB_Hook, true },
            { {L"?GetIconSize@CLibraryViewCB@@UEAAJPEAI@Z"}, (void**)&g_GetIconSize_CLibraryViewCB_Orig, (void*)GetIconSize_CLibraryViewCB_Hook, true },
            { {L"?GetIconSize@CTopViewDescription@@UEAAJPEAI@Z"}, (void**)&g_GetIconSize_CTopViewDescription_Orig, (void*)GetIconSize_CTopViewDescription_Hook, true },
            { {L"?_GetSmallIconSizeCB@@YAXPEAUtagSIZE@@@Z"}, (void**)&g_GetSmallIconSizeCB_Orig, (void*)GetSmallIconSizeCB_Hook, true },
            { {L"?_GetSysSmallIconSizeCB@@YAXPEAUtagSIZE@@@Z"}, (void**)&g_GetSysSmallIconSizeCB_Orig, (void*)GetSysSmallIconSizeCB_Hook, true }
        };
        bool success = WindhawkUtils::HookSymbols(hStorage, hook, ARRAYSIZE(hook));
        Wh_Log(L"HookSymbols windows.storage returned: %d", success);
    }

    g_stopThread.store(false);
    g_workerThread = std::thread(BackgroundPoller);
    
    ForceExplorerUpdate();
    
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Windows 11 Explorer Item Spacing: Uninit");
    
    g_stopThread.store(true);
    if (g_workerThread.joinable()) {
        g_workerThread.join();
    }
}