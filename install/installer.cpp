#define _WIN32_WINNT 0x0A00

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <shlobj.h>
#include <KnownFolders.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <stdint.h>
#include <gdiplus.h>

using namespace Gdiplus;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define IDR_HOST_EXE 101
#define IDR_EXT_ZIP  102

const std::wstring HOST_EXE_NAME = L"pratibimb_host.exe";
const std::wstring ZIP_FILE_NAME = L"extension.zip";

struct Browser {
    std::wstring name;
    std::wstring nativeRoot;
};

static const std::vector<Browser> browsers = {
    {L"Chrome",   L"Software\\Google\\Chrome\\NativeMessagingHosts"},
    {L"Brave",    L"Software\\BraveSoftware\\Brave-Browser\\NativeMessagingHosts"},
    {L"Edge",     L"Software\\Microsoft\\Edge\\NativeMessagingHosts"},
    {L"Chromium", L"Software\\Chromium\\NativeMessagingHosts"},
    {L"Opera",    L"Software\\Opera Software\\Opera Stable\\NativeMessagingHosts"},
    {L"Vivaldi",  L"Software\\Vivaldi\\Vivaldi\\NativeMessagingHosts"},
};

enum {
    IDC_WELCOME_INSTALL   = 101,
    IDC_WELCOME_UNINSTALL = 102,
    IDC_CB_BASE           = 200,
    IDC_SELECT_ALL        = 301,
    IDC_CLEAR_ALL         = 302,
    IDC_CONTINUE          = 303,
    IDC_EDIT_ID           = 403,
    IDC_QUIT              = 404,
};

#define BG_MAIN  RGB(8, 9, 10)
#define BG_CARD  RGB(15, 16, 17)
#define BG_FIELD RGB(24, 25, 26)
#define BD_DIM   RGB(45, 45, 48)
#define BD_MED   RGB(127, 121, 121)
#define TX_HIGH  RGB(244, 244, 248)
#define TX_MED   RGB(230, 230, 234)
#define TX_LOW   RGB(127, 121, 121)
#define AC_FILL  RGB(255, 0, 0)
#define AC_HOT   RGB(255, 70, 70)
#define AC_BG    RGB(35, 10, 10)
#define AC_BD    RGB(130, 0, 0)
#define DG_FILL  RGB(150, 20, 20)
#define DG_HOT   RGB(200, 25, 25)
#define DG_BD    RGB(90, 10, 10)

constexpr int WIN_W = 800;
constexpr int WIN_H = 600;
constexpr UINT_PTR TIMER_AUTO_LINK = 2;
constexpr UINT_PTR TIMER_FADE = 3;
constexpr int FADE_FRAMES = 10;

enum class Screen {
    Welcome,
    BrowserSelect,
    Instructions,
    LinkHost,
    Complete
};

enum class FlowMode {
    None,
    Install,
    Uninstall
};

HWND g_hwnd = nullptr;
HWND g_btnInstall = nullptr;
HWND g_btnUninstall = nullptr;
HWND g_btnSelectAll = nullptr;
HWND g_btnClearAll = nullptr;
HWND g_btnContinue = nullptr;
HWND g_btnQuit = nullptr;
HWND g_editId = nullptr;
HWND g_cb[6] = {};

HFONT g_fTitle = nullptr;
HFONT g_fHero = nullptr;
HFONT g_fSect = nullptr;
HFONT g_fBody = nullptr;
HFONT g_fBold = nullptr;
HFONT g_fMono = nullptr;
HFONT g_fSmall = nullptr;

HBRUSH g_brMain = nullptr;
HBRUSH g_brField = nullptr;

Screen g_screen = Screen::Welcome;
Screen g_prevScreen = Screen::Welcome;
FlowMode g_mode = FlowMode::None;
bool g_fading = false;
int g_fadeFrame = 0;
bool g_extractDone = false;
bool g_linkQueued = false;
std::wstring g_statusText = L"Choose whether you want to install or uninstall.";
std::wstring g_hintText = L"A guided setup will walk through each step automatically.";
std::wstring g_completeTitle = L"Complete";
std::wstring g_completeBody = L"All steps finished successfully.";

namespace Layout {
    constexpr int PX = 36;
    constexpr int IW = WIN_W - PX * 2;
    constexpr int HERO_Y = 34;
    constexpr int HERO_H = 124;
    constexpr int PROGRESS_Y = 182;
    constexpr int CONTENT_Y = 220;
    constexpr int CONTENT_H = 268;
    constexpr int STATUS_Y = 512;
}

std::wstring EscapeJson(const std::wstring &v) {
    std::wstring r;
    for (wchar_t c : v) {
        switch (c) {
        case L'"':  r += L"\\\""; break;
        case L'\\': r += L"\\\\"; break;
        case L'\n': r += L"\\n"; break;
        default:    r += c;
        }
    }
    return r;
}

bool RegWrite(const std::wstring &kp, const std::wstring &val) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kp.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) return false;
    LONG rc = RegSetValueExW(k, nullptr, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(val.c_str()),
        static_cast<DWORD>((val.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(k);
    return rc == ERROR_SUCCESS;
}

bool RegDel(const std::wstring &kp) {
    LONG rc = RegDeleteTreeW(HKEY_CURRENT_USER, kp.c_str());
    return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
}

std::wstring AppDir() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::wstring d = std::wstring(p) + L"\\Pratibimb";
        CoTaskMemFree(p);
        return d;
    }
    return L"";
}

bool WriteRes(int id, const std::wstring &path) {
    HRSRC hr = FindResource(nullptr, MAKEINTRESOURCE(id), RT_RCDATA);
    if (!hr) return false;
    HGLOBAL hg = LoadResource(nullptr, hr);
    DWORD sz = SizeofResource(nullptr, hr);
    void *pd = LockResource(hg);
    if (!pd) return false;
    std::ofstream f(std::filesystem::path(path), std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(pd), sz);
    return true;
}

std::vector<int> Checked() {
    std::vector<int> out;
    for (int i = 0; i < 6; ++i) {
        if (SendMessageW(g_cb[i], BM_GETCHECK, 0, 0) == BST_CHECKED) out.push_back(i);
    }
    return out;
}

bool IsValidExtensionId(const std::wstring &id) {
    if (id.size() != 32) return false;
    for (wchar_t c : id) {
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z'))) return false;
    }
    return true;
}

void SetStatus(const std::wstring &status, const std::wstring &hint) {
    g_statusText = status;
    g_hintText = hint;
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, TRUE);
}

void HideAllControls() {
    ShowWindow(g_btnInstall, SW_HIDE);
    ShowWindow(g_btnUninstall, SW_HIDE);
    ShowWindow(g_btnSelectAll, SW_HIDE);
    ShowWindow(g_btnClearAll, SW_HIDE);
    ShowWindow(g_btnContinue, SW_HIDE);
    ShowWindow(g_btnQuit, SW_HIDE);
    ShowWindow(g_editId, SW_HIDE);
}

void UpdateControls() {
    if (g_fading) {
        HideAllControls();
        return;
    }

    HideAllControls();

    switch (g_screen) {
    case Screen::Welcome:
        break;
    case Screen::BrowserSelect:
        SetWindowTextW(g_btnContinue, g_mode == FlowMode::Install ? L"Continue" : L"Remove");
        ShowWindow(g_btnSelectAll, SW_SHOW);
        ShowWindow(g_btnClearAll, SW_SHOW);
        ShowWindow(g_btnContinue, SW_SHOW);
        break;
    case Screen::Instructions:
        SetWindowTextW(g_btnContinue, L"Continue");
        ShowWindow(g_btnContinue, SW_SHOW);
        break;
    case Screen::LinkHost:
        ShowWindow(g_editId, SW_SHOW);
        SetFocus(g_editId);
        break;
    case Screen::Complete:
        SetWindowTextW(g_btnQuit, L"Quit");
        ShowWindow(g_btnQuit, SW_SHOW);
        SetFocus(g_btnQuit);
        break;
    }
}

void OnScreenEntered(Screen screen) {
    switch (screen) {
    case Screen::Welcome:
        SetStatus(L"Choose whether you want to install or uninstall.",
            L"A guided setup will walk through each step automatically.");
        break;
    case Screen::BrowserSelect:
        SetStatus(L"Select the browsers you want to target.",
            g_mode == FlowMode::Install
                ? L"Continue to prepare the extension package for the selected browsers."
                : L"Continue to remove the native host from the selected browsers.");
        break;
    case Screen::Instructions:
        SetStatus(L"Review the extension loading steps.",
            L"Continue when you're ready and the installer will extract the package and open the folder.");
        break;
    case Screen::LinkHost:
        SetStatus(L"Load the unpacked extension, then paste the generated extension ID.",
            L"Linking begins automatically as soon as the ID looks valid.");
        SetWindowTextW(g_editId, L"");
        break;
    case Screen::Complete:
        SetStatus(g_completeTitle, g_completeBody);
        break;
    }
}

void StartTransition(Screen next) {
    if (g_screen == next) return;
    KillTimer(g_hwnd, TIMER_FADE);
    g_prevScreen = g_screen;
    g_screen = next;
    g_fading = true;
    g_fadeFrame = 0;
    UpdateControls();
    SetTimer(g_hwnd, TIMER_FADE, 16, nullptr);
    InvalidateRect(g_hwnd, nullptr, TRUE);
}

void FinishTransition() {
    KillTimer(g_hwnd, TIMER_FADE);
    g_fading = false;
    g_fadeFrame = FADE_FRAMES;
    UpdateControls();
    OnScreenEntered(g_screen);
    InvalidateRect(g_hwnd, nullptr, TRUE);
}

bool DoExtract(HWND parent) {
    SetStatus(L"Extracting the packaged extension...",
        L"Please wait while the folder is prepared and opened.");

    std::wstring dir = AppDir();
    if (dir.empty()) {
        MessageBoxW(parent, L"Cannot determine AppData path.", L"Error", MB_ICONERROR);
        SetStatus(L"Extraction failed.", L"Could not determine the local application data folder.");
        return false;
    }

    std::filesystem::create_directories(dir);
    std::filesystem::create_directories(dir + L"\\extension");
    std::wstring zip = dir + L"\\" + ZIP_FILE_NAME;

    if (!WriteRes(IDR_EXT_ZIP, zip)) {
        MessageBoxW(parent, L"Failed to unpack extension.zip.\nDid you compile resources.res?", L"Error", MB_ICONERROR);
        SetStatus(L"Extraction failed.", L"The packaged extension could not be written from installer resources.");
        return false;
    }

    std::wstring cmd = L"powershell.exe -WindowStyle Hidden -NoProfile -Command "
        L"\"Expand-Archive -LiteralPath '" + zip +
        L"' -DestinationPath '" + dir + L"\\extension' -Force\"";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    DeleteFileW(zip.c_str());
    g_extractDone = true;
    ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
    SetStatus(L"Folder opened.",
        L"Enable Developer Mode in your browser, load the extracted extension folder, and then paste the generated ID.");
    return true;
}

bool DoLink(HWND parent) {
    auto sel = Checked();
    if (sel.empty()) {
        SetStatus(L"Choose at least one browser.", L"Browser registration needs a destination before linking can continue.");
        return false;
    }

    wchar_t buf[256] = {};
    GetWindowTextW(g_editId, buf, 256);
    std::wstring id = buf;
    if (!IsValidExtensionId(id)) {
        SetStatus(L"Paste a valid 32-character extension ID.", L"Linking will begin automatically once the value looks correct.");
        return false;
    }

    SetStatus(L"Linking the native host...",
        L"Writing the manifest and browser registration entries.");

    std::wstring dir = AppDir();
    std::filesystem::create_directories(dir + L"\\host");
    std::wstring hostPath = dir + L"\\host\\" + HOST_EXE_NAME;
    std::wstring mfPath = dir + L"\\host\\com.suntzv.pratibimb.json";

    if (!WriteRes(IDR_HOST_EXE, hostPath)) {
        MessageBoxW(parent, L"Failed to unpack host executable.", L"Error", MB_ICONERROR);
        SetStatus(L"Linking failed.", L"The native host executable could not be written.");
        return false;
    }

    std::wofstream mf(mfPath.c_str(), std::ios::binary);
    if (!mf) {
        MessageBoxW(parent, L"Cannot write manifest file.", L"Error", MB_ICONERROR);
        SetStatus(L"Linking failed.", L"The native host manifest could not be created.");
        return false;
    }

    mf << L"{\r\n"
       << L"  \"name\": \"com.suntzv.pratibimb\",\r\n"
       << L"  \"description\": \"Pratibimb Native Host\",\r\n"
       << L"  \"path\": \"" << EscapeJson(hostPath) << L"\",\r\n"
       << L"  \"type\": \"stdio\",\r\n"
       << L"  \"allowed_origins\": [ \"chrome-extension://" << id << L"/\" ]\r\n"
       << L"}\r\n";
    mf.close();

    bool ok = true;
    for (int i : sel) {
        if (!RegWrite(browsers[i].nativeRoot + L"\\com.suntzv.pratibimb", mfPath)) ok = false;
    }

    if (!ok) {
        SetStatus(L"Linking finished with warnings.", L"Some browser registry entries could not be written.");
    }
    return ok;
}

bool DoUninstall(HWND parent) {
    auto sel = Checked();
    if (sel.empty()) {
        SetStatus(L"Choose at least one browser.", L"Select the browsers you want to remove Pratibimb from.");
        return false;
    }

    SetStatus(L"Removing installed files...",
        L"Cleaning browser registrations and application data.");

    bool ok = true;
    for (int i : sel) {
        if (!RegDel(browsers[i].nativeRoot + L"\\com.suntzv.pratibimb")) ok = false;
    }

    std::wstring dir = AppDir();
    if (!dir.empty() && std::filesystem::exists(dir)) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    if (!ok) {
        MessageBoxW(parent, L"Some registry entries could not be removed.", L"Warning", MB_ICONWARNING);
    }
    return ok;
}

Color GdiColor(COLORREF c, BYTE alpha = 255) {
    return Color(alpha, GetRValue(c), GetGValue(c), GetBValue(c));
}

void FillRR(HDC dc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF border) {
    Graphics graphics(dc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    GraphicsPath path;
    int d = r * 2;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();

    SolidBrush brush(GdiColor(fill));
    graphics.FillPath(&brush, &path);
    Pen pen(GdiColor(border), 1.0f);
    graphics.DrawPath(&pen, &path);
}

void TxtAt(HDC dc, const wchar_t *s, HFONT f, COLORREF c, int x, int y, int w, int h,
           UINT flags = DT_SINGLELINE | DT_VCENTER | DT_LEFT) {
    HGDIOBJ old = SelectObject(dc, f);
    SetTextColor(dc, c);
    RECT r { x, y, x + w, y + h };
    DrawTextW(dc, s, -1, &r, flags);
    SelectObject(dc, old);
}

RECT BrowserRect(int index) {
    int cardW = (Layout::IW - 64) / 3;
    int col = index % 3;
    int row = index / 3;
    int x = Layout::PX + 22 + col * (cardW + 10);
    int y = Layout::CONTENT_Y + 82 + row * 56;
    return RECT { x, y, x + cardW, y + 42 };
}

RECT WelcomeCardRect(bool install) {
    int x = install ? Layout::PX + 28 : Layout::PX + 376;
    return RECT { x, Layout::CONTENT_Y + 104, x + 326, Layout::CONTENT_Y + 222 };
}

void DrawProgress(HDC dc, Screen screen) {
    if (g_mode != FlowMode::Install || screen == Screen::Welcome) return;

    const wchar_t *steps[4] = { L"Browsers", L"Instructions", L"Link", L"Complete" };
    Screen map[4] = { Screen::BrowserSelect, Screen::Instructions, Screen::LinkHost, Screen::Complete };
    int startX = Layout::PX + Layout::IW - 290;
    int y = Layout::PROGRESS_Y;

    Graphics graphics(dc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    for (int i = 0; i < 4; ++i) {
        bool active = screen == map[i];
        bool done = static_cast<int>(screen) > static_cast<int>(map[i]);
        COLORREF fill = active || done ? AC_FILL : BG_FIELD;
        COLORREF text = active ? TX_HIGH : (done ? TX_MED : TX_LOW);

        FillRR(dc, startX + i * 72, y, 18, 18, 9, fill, done || active ? fill : BD_MED);
        if (done) {
            TxtAt(dc, L"v", g_fSmall, TX_HIGH, startX + i * 72, y, 18, 18, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            wchar_t num[2] = { wchar_t(L'1' + i), 0 };
            TxtAt(dc, num, g_fSmall, active ? TX_HIGH : TX_LOW, startX + i * 72, y, 18, 18, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        TxtAt(dc, steps[i], g_fSmall, text, startX + 22 + i * 72, y - 1, 54, 18, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

void DrawStatusBar(HDC dc) {
    FillRR(dc, Layout::PX, Layout::STATUS_Y, Layout::IW, 54, 12, BG_CARD, BD_DIM);

    COLORREF dot = AC_FILL;
    if (g_screen == Screen::Complete) dot = RGB(40, 190, 90);
    if (g_mode == FlowMode::Uninstall && g_screen == Screen::BrowserSelect) dot = DG_FILL;

    Graphics graphics(dc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    SolidBrush dotBrush(GdiColor(dot));
    graphics.FillEllipse(&dotBrush, Layout::PX + 16, Layout::STATUS_Y + 21, 10, 10);

    TxtAt(dc, g_statusText.c_str(), g_fBold, TX_HIGH,
        Layout::PX + 36, Layout::STATUS_Y + 8, Layout::IW - 52, 18, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    TxtAt(dc, g_hintText.c_str(), g_fSmall, TX_LOW,
        Layout::PX + 36, Layout::STATUS_Y + 26, Layout::IW - 52, 18, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void DrawScene(HDC dc, Screen screen) {
    RECT rc { 0, 0, WIN_W, WIN_H };
    FillRect(dc, &rc, g_brMain);
    SetBkMode(dc, TRANSPARENT);

    FillRR(dc, Layout::PX, Layout::HERO_Y, Layout::IW, Layout::HERO_H, 18, BG_CARD, BD_DIM);

    std::wstring heading = L"Pratibimb";
    std::wstring subheading = L"Native browser bridge with a guided setup experience.";
    std::wstring eyebrow = L"SETUP";

    switch (screen) {
    case Screen::Welcome:
        eyebrow = L"WELCOME";
        subheading = L"Install or remove Pratibimb with a clean step-by-step flow.";
        break;
    case Screen::BrowserSelect:
        eyebrow = L"BROWSER SELECT";
        subheading = g_mode == FlowMode::Install
            ? L"Choose where the native host should be registered."
            : L"Choose which browsers should be cleaned up.";
        break;
    case Screen::Instructions:
        eyebrow = L"INSTRUCTIONS";
        subheading = L"Review the loading steps while the extension package is prepared automatically.";
        break;
    case Screen::LinkHost:
        eyebrow = L"LINK HOST";
        subheading = L"Paste your browser-generated extension ID to finish linking.";
        break;
    case Screen::Complete:
        eyebrow = L"COMPLETE";
        heading = g_completeTitle;
        subheading = g_completeBody;
        break;
    }

    TxtAt(dc, eyebrow.c_str(), g_fSect, TX_LOW,
        Layout::PX + 24, Layout::HERO_Y + 20, Layout::IW - 48, 16, DT_LEFT | DT_TOP | DT_SINGLELINE);
    TxtAt(dc, heading.c_str(), g_fHero, TX_HIGH,
        Layout::PX + 24, Layout::HERO_Y + 40, Layout::IW - 48, 34, DT_LEFT | DT_TOP | DT_SINGLELINE);
    TxtAt(dc, subheading.c_str(), g_fBody, TX_MED,
        Layout::PX + 24, Layout::HERO_Y + 84, Layout::IW - 48, 22, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    DrawProgress(dc, screen);

    if (screen == Screen::Welcome) {
        FillRR(dc, Layout::PX, Layout::CONTENT_Y, Layout::IW, Layout::CONTENT_H, 18, BG_CARD, BD_DIM);
        TxtAt(dc, L"Choose a path", g_fBold, TX_HIGH,
            Layout::PX + 28, Layout::CONTENT_Y + 28, 240, 22, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, L"Install walks you through extraction and host linking. Uninstall removes the native host files and registrations.", g_fBody, TX_LOW,
            Layout::PX + 28, Layout::CONTENT_Y + 60, Layout::IW - 56, 22, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        RECT installCard = WelcomeCardRect(true);
        RECT uninstallCard = WelcomeCardRect(false);

        FillRR(dc, installCard.left, installCard.top, installCard.right - installCard.left, installCard.bottom - installCard.top, 16, BG_MAIN, AC_BD);
        FillRR(dc, installCard.left + 24, installCard.top + 20, 74, 22, 11, AC_BG, AC_BD);
        TxtAt(dc, L"INSTALL", g_fSect, TX_HIGH,
            installCard.left + 24, installCard.top + 20, 74, 22, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, L"Install", g_fBold, TX_HIGH,
            installCard.left + 26, installCard.top + 52, 160, 24, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, L"Prepare the extension, open the folder, paste the ID, and finish automatically.", g_fSmall, TX_LOW,
            installCard.left + 26, installCard.top + 80, 264, 32, DT_WORDBREAK | DT_LEFT | DT_TOP);
        TxtAt(dc, L"Click anywhere on this card", g_fSmall, TX_LOW,
            installCard.left + 26, installCard.top + 94, 220, 18, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        FillRR(dc, uninstallCard.left, uninstallCard.top, uninstallCard.right - uninstallCard.left, uninstallCard.bottom - uninstallCard.top, 16, BG_MAIN, DG_BD);
        FillRR(dc, uninstallCard.left + 24, uninstallCard.top + 20, 90, 22, 11, RGB(60, 14, 14), DG_BD);
        TxtAt(dc, L"UNINSTALL", g_fSect, TX_HIGH,
            uninstallCard.left + 24, uninstallCard.top + 20, 90, 22, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, L"Uninstall", g_fBold, TX_HIGH,
            uninstallCard.left + 26, uninstallCard.top + 52, 160, 24, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, L"Remove the native host files and browser registrations from the selected browsers.", g_fSmall, TX_LOW,
            uninstallCard.left + 26, uninstallCard.top + 80, 264, 32, DT_WORDBREAK | DT_LEFT | DT_TOP);
        TxtAt(dc, L"Click anywhere on this card", g_fSmall, TX_LOW,
            uninstallCard.left + 26, uninstallCard.top + 94, 220, 18, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (screen == Screen::BrowserSelect) {
        FillRR(dc, Layout::PX, Layout::CONTENT_Y, Layout::IW, Layout::CONTENT_H, 18, BG_CARD, BD_DIM);
        TxtAt(dc, L"Target browsers", g_fBold, TX_HIGH,
            Layout::PX + 28, Layout::CONTENT_Y + 28, 220, 22, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, L"Pick one or more browsers before continuing.", g_fBody, TX_LOW,
            Layout::PX + 28, Layout::CONTENT_Y + 58, Layout::IW - 56, 20, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        const wchar_t *letters[6] = { L"C", L"B", L"E", L"C", L"O", L"V" };
        const COLORREF badgeColors[6] = {
            RGB(66, 133, 244), RGB(251, 84, 43), RGB(0, 168, 232),
            RGB(100, 181, 246), RGB(255, 80, 80), RGB(239, 65, 53)
        };

        Graphics graphics(dc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        for (int i = 0; i < 6; ++i) {
            RECT r = BrowserRect(i);
            bool on = SendMessageW(g_cb[i], BM_GETCHECK, 0, 0) == BST_CHECKED;
            FillRR(dc, r.left, r.top, r.right - r.left, r.bottom - r.top, 10,
                on ? AC_BG : BG_MAIN, on ? AC_BD : BD_DIM);

            SolidBrush brush(GdiColor(badgeColors[i]));
            graphics.FillEllipse(&brush, r.left + 14, r.top + 13, 16, 16);
            TxtAt(dc, letters[i], g_fSmall, TX_HIGH, r.left + 14, r.top + 13, 16, 16, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            TxtAt(dc, browsers[i].name.c_str(), g_fBody, on ? TX_HIGH : TX_MED,
                r.left + 40, r.top, (r.right - r.left) - 72, r.bottom - r.top, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            FillRR(dc, r.right - 24, r.top + 14, 12, 12, 4,
                on ? AC_FILL : BG_FIELD, on ? AC_FILL : BD_MED);
            if (on) TxtAt(dc, L"v", g_fSmall, TX_HIGH, r.right - 24, r.top + 14, 12, 12, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    if (screen == Screen::Instructions) {
        FillRR(dc, Layout::PX, Layout::CONTENT_Y, Layout::IW, Layout::CONTENT_H, 18, BG_CARD, BD_DIM);
        TxtAt(dc, L"Before extraction", g_fBold, TX_HIGH,
            Layout::PX + 28, Layout::CONTENT_Y + 28, 220, 22, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        const wchar_t *steps[3] = {
            L"Keep your browser open so you can visit its extensions page.",
            L"After the folder opens, enable Developer Mode and load the extracted extension folder.",
            L"On the next screen, paste the generated extension ID to finish linking automatically."
        };

        for (int i = 0; i < 3; ++i) {
            int y = Layout::CONTENT_Y + 78 + i * 54;
            FillRR(dc, Layout::PX + 28, y, Layout::IW - 56, 42, 10, BG_MAIN, BD_DIM);
            FillRR(dc, Layout::PX + 40, y + 10, 22, 22, 11, AC_FILL, AC_FILL);
            wchar_t num[2] = { wchar_t(L'1' + i), 0 };
            TxtAt(dc, num, g_fSmall, TX_HIGH, Layout::PX + 40, y + 10, 22, 22, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            TxtAt(dc, steps[i], g_fBody, TX_MED,
                Layout::PX + 76, y, Layout::IW - 120, 42, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        TxtAt(dc, L"Use Continue when you've read these steps.", g_fSmall, TX_LOW,
            Layout::PX + 28, Layout::CONTENT_Y + 238, Layout::IW - 56, 18, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (screen == Screen::LinkHost) {
        FillRR(dc, Layout::PX, Layout::CONTENT_Y, Layout::IW, Layout::CONTENT_H, 18, BG_CARD, BD_DIM);
        TxtAt(dc, L"Paste the extension ID", g_fBold, TX_HIGH,
            Layout::PX + 28, Layout::CONTENT_Y + 28, 260, 22, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, L"Once the ID is valid, native host registration starts automatically.", g_fBody, TX_LOW,
            Layout::PX + 28, Layout::CONTENT_Y + 60, Layout::IW - 56, 20, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        FillRR(dc, Layout::PX + 28, Layout::CONTENT_Y + 104, Layout::IW - 56, 72, 14, BG_MAIN, BD_DIM);
        TxtAt(dc, L"Extension ID", g_fSect, TX_LOW,
            Layout::PX + 48, Layout::CONTENT_Y + 122, 180, 16, DT_LEFT | DT_TOP | DT_SINGLELINE);
        TxtAt(dc, L"Paste the 32-character ID from your browser's loaded unpacked extension.", g_fSmall, TX_LOW,
            Layout::PX + 48, Layout::CONTENT_Y + 146, Layout::IW - 96, 16, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        TxtAt(dc, L"Auto-link starts as soon as the value matches the expected format.", g_fSmall, TX_LOW,
            Layout::PX + 28, Layout::CONTENT_Y + 206, Layout::IW - 56, 18, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (screen == Screen::Complete) {
        FillRR(dc, Layout::PX, Layout::CONTENT_Y, Layout::IW, Layout::CONTENT_H, 18, BG_CARD, BD_DIM);
        Graphics graphics(dc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        SolidBrush okBrush(GdiColor(RGB(40, 190, 90)));
        graphics.FillEllipse(&okBrush, Layout::PX + 48, Layout::CONTENT_Y + 52, 60, 60);
        TxtAt(dc, L"v", g_fHero, TX_HIGH,
            Layout::PX + 48, Layout::CONTENT_Y + 52, 60, 60, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        TxtAt(dc, g_completeTitle.c_str(), g_fBold, TX_HIGH,
            Layout::PX + 132, Layout::CONTENT_Y + 56, Layout::IW - 180, 26, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        TxtAt(dc, g_completeBody.c_str(), g_fBody, TX_LOW,
            Layout::PX + 132, Layout::CONTENT_Y + 92, Layout::IW - 180, 44, DT_WORDBREAK | DT_LEFT | DT_TOP);
    }

    DrawStatusBar(dc);
}

void PaintButton(LPDRAWITEMSTRUCT d, COLORREF fill, COLORREF border, COLORREF text) {
    HDC dc = d->hDC;
    int x = d->rcItem.left;
    int y = d->rcItem.top;
    int w = d->rcItem.right - x;
    int h = d->rcItem.bottom - y;
    bool hot = (d->itemState & ODS_HOTLIGHT) != 0;
    bool down = (d->itemState & ODS_SELECTED) != 0;

    COLORREF drawFill = fill;
    if (hot && fill == AC_FILL) drawFill = AC_HOT;
    if (hot && fill == DG_FILL) drawFill = DG_HOT;
    if (down) drawFill = RGB(
        std::max(0, (int)GetRValue(drawFill) - 22),
        std::max(0, (int)GetGValue(drawFill) - 22),
        std::max(0, (int)GetBValue(drawFill) - 22));

    SetBkMode(dc, TRANSPARENT);
    FillRR(dc, x, y, w, h, 10, drawFill, border);

    wchar_t label[128] = {};
    GetWindowTextW(d->hwndItem, label, 128);
    TxtAt(dc, label, g_fBold, text, x, y, w, h, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    HDC baseDc = CreateCompatibleDC(hdc);
    HBITMAP baseBmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
    HGDIOBJ oldBase = SelectObject(baseDc, baseBmp);

    if (!g_fading) {
        DrawScene(baseDc, g_screen);
        BitBlt(hdc, 0, 0, WIN_W, WIN_H, baseDc, 0, 0, SRCCOPY);
    } else {
        HDC prevDc = CreateCompatibleDC(hdc);
        HBITMAP prevBmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
        HGDIOBJ oldPrev = SelectObject(prevDc, prevBmp);

        HDC nextDc = CreateCompatibleDC(hdc);
        HBITMAP nextBmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
        HGDIOBJ oldNext = SelectObject(nextDc, nextBmp);

        DrawScene(prevDc, g_prevScreen);
        DrawScene(nextDc, g_screen);

        BitBlt(hdc, 0, 0, WIN_W, WIN_H, prevDc, 0, 0, SRCCOPY);

        BLENDFUNCTION blend = {};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = static_cast<BYTE>((255 * g_fadeFrame) / FADE_FRAMES);
        AlphaBlend(hdc, 0, 0, WIN_W, WIN_H, nextDc, 0, 0, WIN_W, WIN_H, blend);

        SelectObject(prevDc, oldPrev);
        DeleteObject(prevBmp);
        DeleteDC(prevDc);

        SelectObject(nextDc, oldNext);
        DeleteObject(nextBmp);
        DeleteDC(nextDc);
    }

    SelectObject(baseDc, oldBase);
    DeleteObject(baseBmp);
    DeleteDC(baseDc);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        g_hwnd = hwnd;

        g_brMain = CreateSolidBrush(BG_MAIN);
        g_brField = CreateSolidBrush(BG_FIELD);

        auto MkFont = [](int h, int w, const wchar_t *n) {
            return CreateFontW(h, 0, 0, 0, w, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, n);
        };

        g_fHero = MkFont(34, FW_LIGHT, L"Segoe UI Light");
        g_fTitle = MkFont(28, FW_LIGHT, L"Segoe UI Light");
        g_fSect = MkFont(11, FW_BOLD, L"Segoe UI");
        g_fBody = MkFont(16, FW_NORMAL, L"Segoe UI");
        g_fBold = MkFont(16, FW_SEMIBOLD, L"Segoe UI Semibold");
        g_fMono = MkFont(16, FW_NORMAL, L"Consolas");
        g_fSmall = MkFont(13, FW_NORMAL, L"Segoe UI");

        for (int i = 0; i < 6; ++i) {
            g_cb[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX,
                0, 0, 0, 0, hwnd, (HMENU)(intptr_t)(IDC_CB_BASE + i), nullptr, nullptr);
        }
        SendMessageW(g_cb[0], BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(g_cb[1], BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(g_cb[2], BM_SETCHECK, BST_CHECKED, 0);

        g_btnInstall = CreateWindowW(L"BUTTON", L"Install",
            WS_CHILD | BS_OWNERDRAW,
            Layout::PX + 64, Layout::CONTENT_Y + 134, 250, 46,
            hwnd, (HMENU)(intptr_t)IDC_WELCOME_INSTALL, nullptr, nullptr);

        g_btnUninstall = CreateWindowW(L"BUTTON", L"Uninstall",
            WS_CHILD | BS_OWNERDRAW,
            Layout::PX + 412, Layout::CONTENT_Y + 134, 250, 46,
            hwnd, (HMENU)(intptr_t)IDC_WELCOME_UNINSTALL, nullptr, nullptr);

        g_btnSelectAll = CreateWindowW(L"BUTTON", L"Select All",
            WS_CHILD | BS_OWNERDRAW,
            Layout::PX + Layout::IW - 220, Layout::CONTENT_Y + 26, 96, 30,
            hwnd, (HMENU)(intptr_t)IDC_SELECT_ALL, nullptr, nullptr);

        g_btnClearAll = CreateWindowW(L"BUTTON", L"Clear All",
            WS_CHILD | BS_OWNERDRAW,
            Layout::PX + Layout::IW - 114, Layout::CONTENT_Y + 26, 96, 30,
            hwnd, (HMENU)(intptr_t)IDC_CLEAR_ALL, nullptr, nullptr);

        g_btnContinue = CreateWindowW(L"BUTTON", L"Continue",
            WS_CHILD | BS_OWNERDRAW,
            Layout::PX + Layout::IW - 190, Layout::CONTENT_Y + Layout::CONTENT_H - 58, 162, 40,
            hwnd, (HMENU)(intptr_t)IDC_CONTINUE, nullptr, nullptr);

        g_editId = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | ES_AUTOHSCROLL,
            Layout::PX + 48, Layout::CONTENT_Y + 116, Layout::IW - 96, 34,
            hwnd, (HMENU)(intptr_t)IDC_EDIT_ID, nullptr, nullptr);
        SendMessageW(g_editId, WM_SETFONT, (WPARAM)g_fMono, TRUE);
        SendMessageW(g_editId, EM_SETCUEBANNER, FALSE, (LPARAM)L"Paste 32-character extension ID");

        g_btnQuit = CreateWindowW(L"BUTTON", L"Quit",
            WS_CHILD | BS_OWNERDRAW,
            Layout::PX + Layout::IW - 164, Layout::CONTENT_Y + Layout::CONTENT_H - 58, 136, 40,
            hwnd, (HMENU)(intptr_t)IDC_QUIT, nullptr, nullptr);

        UpdateControls();
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_DRAWITEM: {
        auto *d = (LPDRAWITEMSTRUCT)lp;
        switch ((int)d->CtlID) {
        case IDC_CONTINUE:
        case IDC_QUIT:
            PaintButton(d, AC_FILL, AC_FILL, TX_HIGH);
            return TRUE;
        case IDC_SELECT_ALL:
        case IDC_CLEAR_ALL:
            PaintButton(d, BG_CARD, BD_MED, TX_MED);
            return TRUE;
        }
        return FALSE;
    }

    case WM_CTLCOLORBTN:
        return (LRESULT)g_brMain;

    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wp;
        SetBkColor(dc, BG_FIELD);
        SetTextColor(dc, TX_MED);
        return (LRESULT)g_brField;
    }

    case WM_LBUTTONDOWN:
        if (g_fading) break;
        if (g_screen == Screen::Welcome) {
            int mx = LOWORD(lp);
            int my = HIWORD(lp);
            RECT installCard = WelcomeCardRect(true);
            RECT uninstallCard = WelcomeCardRect(false);
            if (mx >= installCard.left && mx <= installCard.right && my >= installCard.top && my <= installCard.bottom) {
                g_mode = FlowMode::Install;
                StartTransition(Screen::BrowserSelect);
                return 0;
            }
            if (mx >= uninstallCard.left && mx <= uninstallCard.right && my >= uninstallCard.top && my <= uninstallCard.bottom) {
                g_mode = FlowMode::Uninstall;
                StartTransition(Screen::BrowserSelect);
                return 0;
            }
        }
        if (g_screen == Screen::BrowserSelect) {
            int mx = LOWORD(lp);
            int my = HIWORD(lp);
            for (int i = 0; i < 6; ++i) {
                RECT r = BrowserRect(i);
                if (mx >= r.left && mx <= r.right && my >= r.top && my <= r.bottom) {
                    bool on = SendMessageW(g_cb[i], BM_GETCHECK, 0, 0) == BST_CHECKED;
                    SendMessageW(g_cb[i], BM_SETCHECK, on ? BST_UNCHECKED : BST_CHECKED, 0);
                    InvalidateRect(hwnd, nullptr, TRUE);
                    break;
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_SELECT_ALL:
            for (int i = 0; i < 6; ++i) SendMessageW(g_cb[i], BM_SETCHECK, BST_CHECKED, 0);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case IDC_CLEAR_ALL:
            for (int i = 0; i < 6; ++i) SendMessageW(g_cb[i], BM_SETCHECK, BST_UNCHECKED, 0);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case IDC_CONTINUE:
            if (g_screen == Screen::BrowserSelect && Checked().empty()) {
                SetStatus(L"Choose at least one browser.", L"Select the browsers you want to target before continuing.");
                break;
            }
            if (g_screen == Screen::BrowserSelect && g_mode == FlowMode::Install) {
                StartTransition(Screen::Instructions);
            } else if (g_screen == Screen::BrowserSelect) {
                bool ok = DoUninstall(hwnd);
                g_completeTitle = ok ? L"Uninstall complete" : L"Uninstall finished with warnings";
                g_completeBody = ok
                    ? L"Pratibimb files and browser registrations were removed from the selected browsers."
                    : L"Most uninstall steps finished, but some registry entries may still need manual cleanup.";
                StartTransition(Screen::Complete);
            } else if (g_screen == Screen::Instructions && g_mode == FlowMode::Install) {
                if (DoExtract(hwnd)) StartTransition(Screen::LinkHost);
            }
            break;
        case IDC_EDIT_ID:
            if (HIWORD(wp) == EN_CHANGE && g_screen == Screen::LinkHost && !g_fading) {
                wchar_t buf[256] = {};
                GetWindowTextW(g_editId, buf, 256);
                std::wstring id = buf;
                KillTimer(hwnd, TIMER_AUTO_LINK);
                g_linkQueued = false;

                if (IsValidExtensionId(id)) {
                    g_linkQueued = true;
                    SetStatus(L"Extension ID detected.", L"Finishing native host registration automatically...");
                    SetTimer(hwnd, TIMER_AUTO_LINK, 500, nullptr);
                } else {
                    SetStatus(L"Load the unpacked extension, then paste the generated extension ID.",
                        L"Linking begins automatically as soon as the ID looks valid.");
                }
            }
            break;
        case IDC_QUIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;

    case WM_TIMER:
        if (wp == TIMER_FADE) {
            ++g_fadeFrame;
            if (g_fadeFrame >= FADE_FRAMES) FinishTransition();
            else InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        if (wp == TIMER_AUTO_LINK) {
            KillTimer(hwnd, TIMER_AUTO_LINK);
            g_linkQueued = false;
            if (g_screen == Screen::LinkHost && g_mode == FlowMode::Install) {
                bool ok = DoLink(hwnd);
                g_completeTitle = ok ? L"Installation complete" : L"Installation finished with warnings";
                g_completeBody = ok
                    ? L"Pratibimb is linked successfully. You can close setup and start using the extension."
                    : L"The native host was created, but some browser registrations may need manual review.";
                StartTransition(Screen::Complete);
            }
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        DeleteObject(g_brMain);
        DeleteObject(g_brField);
        DeleteObject(g_fHero);
        DeleteObject(g_fTitle);
        DeleteObject(g_fSect);
        DeleteObject(g_fBody);
        DeleteObject(g_fBold);
        DeleteObject(g_fMono);
        DeleteObject(g_fSmall);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int ncs) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    INITCOMMONCONTROLSEX icx = { sizeof(icx), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icx);

    const wchar_t CLS[] = L"PratibimbWnd";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = CLS;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG_MAIN);
    RegisterClassW(&wc);

    RECT wr = { 0, 0, WIN_W, WIN_H };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);

    HWND hwnd = CreateWindowExW(0, CLS, L"Pratibimb Setup",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hi, nullptr);
    if (!hwnd) return 0;

    ShowWindow(hwnd, ncs);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
