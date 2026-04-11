#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// --- HARDCODED CONSTANTS FOR ZERO-CONFIG ---
const std::wstring EXTENSION_ID = L"fnldepapgimkeegfohgdjnhjneoljcil";
const std::wstring CRX_FILE_NAME = L"extension.crx";
const std::wstring HOST_EXE_NAME = L"pratibimb_host.exe";

struct Browser {
    std::wstring name;
    std::wstring nativeRoot;
    std::wstring extRoot;
};

static const std::vector<Browser> browsers = {
    {L"Google Chrome", L"Software\\Google\\Chrome\\NativeMessagingHosts", L"Software\\Google\\Chrome\\Extensions"},
    {L"Brave", L"Software\\BraveSoftware\\Brave-Browser\\NativeMessagingHosts", L"Software\\BraveSoftware\\Brave-Browser\\Extensions"},
    {L"Microsoft Edge", L"Software\\Microsoft\\Edge\\NativeMessagingHosts", L"Software\\Microsoft\\Edge\\Extensions"},
    {L"Chromium", L"Software\\Chromium\\NativeMessagingHosts", L"Software\\Chromium\\Extensions"},
    {L"Opera", L"Software\\Opera Software\\Opera Stable\\NativeMessagingHosts", L"Software\\Opera Software\\Opera Stable\\Extensions"},
    {L"Vivaldi", L"Software\\Vivaldi\\Vivaldi\\NativeMessagingHosts", L"Software\\Vivaldi\\Vivaldi\\Extensions"}
};

enum ControlId {
    IDC_INSTALL_RADIO = 101,
    IDC_UNINSTALL_RADIO,
    IDC_BROWSER_BASE = 200,
    IDC_SELECT_ALL = 301,
    IDC_CLEAR_ALL,
    IDC_ACTION_BUTTON,
    IDC_STATUS_LABEL
};

HWND g_status = nullptr;
HWND g_browserCheckboxes[6] = {};

HBRUSH g_hbrBackground = nullptr;
HFONT g_hFontTitle = nullptr;
HFONT g_hFont = nullptr;
HFONT g_hFontBold = nullptr;

std::wstring EscapeJsonString(const std::wstring &value) {
    std::wstring result;
    for (wchar_t c : value) {
        switch (c) {
            case L'"': result += L"\\\""; break;
            case L'\\': result += L"\\\\"; break;
            case L'\n': result += L"\\n"; break;
            case L'\r': result += L"\\r"; break;
            case L'\t': result += L"\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

bool WriteRegistryValue(const std::wstring &keyPath, const std::wstring &valueName, const std::wstring &value) {
    HKEY key;
    LONG rc = RegCreateKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr);
    if (rc != ERROR_SUCCESS) return false;
    rc = RegSetValueExW(key, valueName.empty() ? nullptr : valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}

bool DeleteRegistryTree(const std::wstring &keyPath) {
    return RegDeleteTreeW(HKEY_CURRENT_USER, keyPath.c_str()) == ERROR_SUCCESS || GetLastError() == ERROR_FILE_NOT_FOUND;
}

std::wstring GetExeFolder() {
    wchar_t buffer[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);
    if (length == 0 || length == MAX_PATH) return L".";
    return std::filesystem::path(buffer).parent_path().wstring();
}

void SetStatusText(const std::wstring &text) {
    if (g_status) SetWindowTextW(g_status, text.c_str());
}

std::vector<int> GetSelectedBrowserIndexes() {
    std::vector<int> result;
    for (int i = 0; i < 6; ++i) {
        if (SendMessageW(g_browserCheckboxes[i], BM_GETCHECK, 0, 0) == BST_CHECKED) result.push_back(i);
    }
    return result;
}

bool CreateNativeManifest(const std::wstring &manifestPath, const std::wstring &hostPath) {
    std::wofstream file;
    file.open(manifestPath.c_str(), std::ios::binary);
    if (!file) return false;
    std::wstring origin = std::wstring(L"chrome-extension://") + EXTENSION_ID + L"/";
    file << L"{\r\n"
         << L"  \"name\": \"com.suntzv.pratibimb\",\r\n"
         << L"  \"description\": \"Pratibimb Native Host Engine\",\r\n"
         << L"  \"path\": \"" << EscapeJsonString(hostPath) << L"\",\r\n"
         << L"  \"type\": \"stdio\",\r\n"
         << L"  \"allowed_origins\": [\r\n"
         << L"    \"" << EscapeJsonString(origin) << L"\"\r\n"
         << L"  ]\r\n"
         << L"}\r\n";
    return true;
}

bool PerformInstall(HWND parent) {
    auto selected = GetSelectedBrowserIndexes();
    if (selected.empty()) { SetStatusText(L"Error: Select at least one browser."); return false; }

    std::filesystem::path rootPath = std::filesystem::path(GetExeFolder()).parent_path();
    std::wstring hostPath = (rootPath / L"host" / HOST_EXE_NAME).lexically_normal().wstring();
    std::wstring crxPath = (rootPath / CRX_FILE_NAME).lexically_normal().wstring();
    std::wstring manifestPath = (rootPath / L"host" / L"com.suntzv.pratibimb.json").lexically_normal().wstring();

    if (!std::filesystem::exists(hostPath)) {
        MessageBoxW(parent, L"Failed to locate pratibimb_host.exe in the 'host' folder.", L"Missing File", MB_ICONERROR);
        return false;
    }
    if (!std::filesystem::exists(crxPath)) {
        MessageBoxW(parent, L"Failed to locate extension.crx in the root folder.", L"Missing File", MB_ICONERROR);
        return false;
    }

    if (!CreateNativeManifest(manifestPath, hostPath)) {
        MessageBoxW(parent, L"Failed to create native messaging manifest.", L"Error", MB_ICONERROR);
        return false;
    }

    bool success = true;
    for (int idx : selected) {
        // 1. Install Native Host
        std::wstring nmKey = browsers[idx].nativeRoot + L"\\com.suntzv.pratibimb";
        if (!WriteRegistryValue(nmKey, L"", manifestPath)) success = false;
        
        // 2. Sideload Extension
        std::wstring extKey = browsers[idx].extRoot + L"\\" + EXTENSION_ID;
        if (!WriteRegistryValue(extKey, L"", crxPath)) success = false;
        if (!WriteRegistryValue(extKey, L"path", crxPath)) success = false;
        if (!WriteRegistryValue(extKey, L"version", L"1.0")) success = false;
    }

    if (success) SetStatusText(L"Success: Pratibimb securely installed.");
    else SetStatusText(L"Warning: Some registry keys failed to write.");
    return success;
}

bool PerformUninstall(HWND parent) {
    auto selected = GetSelectedBrowserIndexes();
    if (selected.empty()) { SetStatusText(L"Error: Select at least one browser."); return false; }

    bool success = true;
    for (int idx : selected) {
        std::wstring nmKey = browsers[idx].nativeRoot + L"\\com.suntzv.pratibimb";
        std::wstring extKey = browsers[idx].extRoot + L"\\" + EXTENSION_ID;
        
        if (!DeleteRegistryTree(nmKey)) success = false;
        if (!DeleteRegistryTree(extKey)) success = false;
    }

    if (success) SetStatusText(L"Success: Pratibimb cleanly uninstalled.");
    else SetStatusText(L"Warning: Some registry keys could not be removed.");
    return success;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Enable Dark Mode Title Bar
            BOOL dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

            // Setup Custom Fonts & Deep Dark Background
            g_hbrBackground = CreateSolidBrush(RGB(18, 18, 18));
            g_hFontTitle = CreateFontW(32, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI Light");
            g_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_hFontBold = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            HWND hTitle = CreateWindowW(L"STATIC", L"PRATIBIMB", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 20, 360, 40, hwnd, nullptr, nullptr, nullptr);
            SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

            HWND hRadioInst = CreateWindowW(L"BUTTON", L"Install", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, 90, 80, 80, 24, hwnd, (HMENU)IDC_INSTALL_RADIO, nullptr, nullptr);
            HWND hRadioUninst = CreateWindowW(L"BUTTON", L"Uninstall", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 200, 80, 100, 24, hwnd, (HMENU)IDC_UNINSTALL_RADIO, nullptr, nullptr);
            SendMessageW(hRadioInst, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageW(hRadioUninst, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageW(hRadioInst, BM_SETCHECK, BST_CHECKED, 0);

            for (int i = 0; i < 6; ++i) {
                g_browserCheckboxes[i] = CreateWindowW(L"BUTTON", browsers[i].name.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 60, 130 + (25 * i), 160, 22, hwnd, (HMENU)(IDC_BROWSER_BASE + i), nullptr, nullptr);
                SendMessageW(g_browserCheckboxes[i], WM_SETFONT, (WPARAM)g_hFont, TRUE);
                SendMessageW(g_browserCheckboxes[i], BM_SETCHECK, BST_CHECKED, 0);
            }

            HWND hBtnSelAll = CreateWindowW(L"BUTTON", L"Select All", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 230, 150, 90, 28, hwnd, (HMENU)IDC_SELECT_ALL, nullptr, nullptr);
            HWND hBtnClrAll = CreateWindowW(L"BUTTON", L"Clear All", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 230, 190, 90, 28, hwnd, (HMENU)IDC_CLEAR_ALL, nullptr, nullptr);
            SendMessageW(hBtnSelAll, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessageW(hBtnClrAll, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hActionBtn = CreateWindowW(L"BUTTON", L"EXECUTE", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 90, 310, 200, 45, hwnd, (HMENU)IDC_ACTION_BUTTON, nullptr, nullptr);
            SendMessageW(hActionBtn, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_status = CreateWindowW(L"STATIC", L"Ready.", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 380, 360, 22, hwnd, (HMENU)IDC_STATUS_LABEL, nullptr, nullptr);
            SendMessageW(g_status, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            break;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(220, 220, 220)); 
            SetBkColor(hdcStatic, RGB(18, 18, 18));      
            return (LRESULT)g_hbrBackground;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_SELECT_ALL:
                    for (int i=0; i<6; i++) SendMessageW(g_browserCheckboxes[i], BM_SETCHECK, BST_CHECKED, 0);
                    break;
                case IDC_CLEAR_ALL:
                    for (int i=0; i<6; i++) SendMessageW(g_browserCheckboxes[i], BM_SETCHECK, BST_UNCHECKED, 0);
                    break;
                case IDC_ACTION_BUTTON:
                    if (SendMessageW(GetDlgItem(hwnd, IDC_INSTALL_RADIO), BM_GETCHECK, 0, 0) == BST_CHECKED) PerformInstall(hwnd);
                    else PerformUninstall(hwnd);
                    break;
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            DeleteObject(g_hbrBackground);
            DeleteObject(g_hFontTitle);
            DeleteObject(g_hFont);
            DeleteObject(g_hFontBold);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"PratibimbInstallerWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(18, 18, 18));

    RegisterClassW(&wc);

    // Slim, perfectly proportioned window
    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Pratibimb Setup", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, 
                                CW_USEDEFAULT, CW_USEDEFAULT, 400, 460, nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}