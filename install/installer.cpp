#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")

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
#include <stdint.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// ---------- resource IDs ----------
#define IDR_HOST_EXE 101
#define IDR_EXT_ZIP  102

const std::wstring HOST_EXE_NAME = L"pratibimb_host.exe";
const std::wstring ZIP_FILE_NAME = L"extension.zip";

struct Browser { std::wstring name, nativeRoot; };
static const std::vector<Browser> browsers = {
    {L"Chrome",   L"Software\\Google\\Chrome\\NativeMessagingHosts"},
    {L"Brave",    L"Software\\BraveSoftware\\Brave-Browser\\NativeMessagingHosts"},
    {L"Edge",     L"Software\\Microsoft\\Edge\\NativeMessagingHosts"},
    {L"Chromium", L"Software\\Chromium\\NativeMessagingHosts"},
    {L"Opera",    L"Software\\Opera Software\\Opera Stable\\NativeMessagingHosts"},
    {L"Vivaldi",  L"Software\\Vivaldi\\Vivaldi\\NativeMessagingHosts"},
};

// ---------- control IDs ----------
enum {
    IDC_MODE_INSTALL   = 101,
    IDC_MODE_UNINSTALL = 102,
    IDC_CB_BASE        = 200,   // 200-205
    IDC_SELECT_ALL     = 301,
    IDC_CLEAR_ALL      = 302,
    IDC_BTN_EXTRACT    = 401,
    IDC_EDIT_ID        = 403,
    IDC_BTN_LINK       = 404,
    IDC_BTN_UNINSTALL  = 405,
    IDC_STATUS         = 500,
};

// ---------- colours ----------
// Background layers
#define BG_MAIN   RGB(15, 15, 18)
#define BG_CARD   RGB(22, 22, 28)
#define BG_FIELD  RGB(28, 28, 35)
// Borders
#define BD_DIM    RGB(40, 40, 52)
#define BD_MED    RGB(55, 55, 70)
// Text
#define TX_HIGH   RGB(230, 228, 240)
#define TX_MED    RGB(155, 152, 175)
#define TX_LOW    RGB(80,  78,  98)
// Accent (purple)
#define AC_FILL   RGB(124, 77, 230)
#define AC_HOT    RGB(140, 95, 245)
#define AC_BG     RGB(30,  24, 50)
#define AC_BD     RGB(70,  50, 120)
// Danger
#define DG_FILL   RGB(35, 14, 14)
#define DG_BD     RGB(90, 30, 30)
#define DG_TX     RGB(240, 150, 150)

// ---------- globals ----------
HWND g_hwnd       = nullptr;
HWND g_status     = nullptr;
HWND g_editId     = nullptr;
HWND g_cb[6]      = {};
HWND g_btnInst    = nullptr;
HWND g_btnUninst  = nullptr;
HWND g_btnExtract = nullptr;
HWND g_btnLink    = nullptr;
HWND g_btnDel     = nullptr;
HWND g_selAll     = nullptr;
HWND g_clrAll     = nullptr;

HFONT g_fTitle  = nullptr;  // 18px light  — logo name
HFONT g_fSect   = nullptr;  // 10px bold   — section caps
HFONT g_fBody   = nullptr;  // 13px normal — body
HFONT g_fBold   = nullptr;  // 13px bold   — button labels
HFONT g_fMono   = nullptr;  // 12px mono   — ID field
HFONT g_fSmall  = nullptr;  // 11px        — status

HBRUSH g_brMain  = nullptr;
HBRUSH g_brCard  = nullptr;
HBRUSH g_brField = nullptr;
HBRUSH g_brAcBg  = nullptr;
HBRUSH g_brDgBg  = nullptr;

bool g_install = true;  // mode flag

// ---------- helpers ----------
std::wstring EscapeJson(const std::wstring &v) {
    std::wstring r;
    for (wchar_t c : v) switch (c) {
        case L'"':  r += L"\\\""; break;
        case L'\\': r += L"\\\\"; break;
        case L'\n': r += L"\\n";  break;
        default:    r += c;
    }
    return r;
}

bool RegWrite(const std::wstring &kp, const std::wstring &val) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kp.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) return false;
    LONG rc = RegSetValueExW(k, nullptr, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(val.c_str()),
        static_cast<DWORD>((val.size()+1)*sizeof(wchar_t)));
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

void Status(const wchar_t *t) { if (g_status) SetWindowTextW(g_status, t); }

std::vector<int> Checked() {
    std::vector<int> r;
    for (int i = 0; i < 6; ++i)
        if (SendMessageW(g_cb[i], BM_GETCHECK, 0, 0) == BST_CHECKED) r.push_back(i);
    return r;
}

// ---------- business logic (unchanged) ----------
bool DoExtract(HWND p) {
    std::wstring dir = AppDir();
    if (dir.empty()) { MessageBoxW(p, L"Cannot determine AppData path.", L"Error", MB_ICONERROR); return false; }
    std::filesystem::create_directories(dir);
    std::filesystem::create_directories(dir + L"\\extension");
    std::wstring zip = dir + L"\\" + ZIP_FILE_NAME;
    if (!WriteRes(IDR_EXT_ZIP, zip)) {
        MessageBoxW(p, L"Failed to unpack extension.zip.\nDid you compile resources.res?", L"Error", MB_ICONERROR);
        return false;
    }
    Status(L"Extracting…");
    std::wstring cmd = L"powershell.exe -WindowStyle Hidden -NoProfile -Command "
        L"\"Expand-Archive -LiteralPath '" + zip +
        L"' -DestinationPath '" + dir + L"\\extension' -Force\"";
    STARTUPINFOW si = { sizeof(si) }; si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }
    DeleteFileW(zip.c_str());
    MessageBoxW(p,
        L"Folder extracted!\n\n"
        L"1. The folder will now open.\n"
        L"2. Go to chrome://extensions and enable Developer Mode.\n"
        L"3. Drag the 'extension' folder into your browser.\n"
        L"4. Copy the generated ID and paste it into the field below.",
        L"Step 1 Complete", MB_OK | MB_ICONINFORMATION);
    ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
    Status(L"Waiting for Extension ID…");
    return true;
}

bool DoLink(HWND p) {
    auto sel = Checked();
    if (sel.empty()) { Status(L"Error: select at least one browser."); return false; }
    wchar_t buf[256] = {};
    GetWindowTextW(g_editId, buf, 256);
    std::wstring id = buf;
    if (id.size() != 32) { Status(L"Error: paste a valid 32-character Extension ID."); return false; }

    std::wstring dir = AppDir();
    std::filesystem::create_directories(dir + L"\\host");
    std::wstring hostPath = dir + L"\\host\\" + HOST_EXE_NAME;
    std::wstring mfPath   = dir + L"\\host\\com.suntzv.pratibimb.json";

    if (!WriteRes(IDR_HOST_EXE, hostPath)) {
        MessageBoxW(p, L"Failed to unpack host executable.", L"Error", MB_ICONERROR); return false;
    }

    std::wofstream mf(mfPath.c_str(), std::ios::binary);
    if (!mf) { MessageBoxW(p, L"Cannot write manifest file.", L"Error", MB_ICONERROR); return false; }
    mf << L"{\r\n"
       << L"  \"name\": \"com.suntzv.pratibimb\",\r\n"
       << L"  \"description\": \"Pratibimb Native Host\",\r\n"
       << L"  \"path\": \"" << EscapeJson(hostPath) << L"\",\r\n"
       << L"  \"type\": \"stdio\",\r\n"
       << L"  \"allowed_origins\": [ \"chrome-extension://" << id << L"/\" ]\r\n"
       << L"}\r\n";
    mf.close();

    bool ok = true;
    for (int i : sel)
        if (!RegWrite(browsers[i].nativeRoot + L"\\com.suntzv.pratibimb", mfPath)) ok = false;

    if (ok) {
        Status(L"Setup complete. You can close this window.");
        MessageBoxW(p, L"Native host linked successfully!\n\nSetup is 100% complete.", L"Success", MB_OK | MB_ICONINFORMATION);
    } else {
        Status(L"Warning: some registry entries failed to write.");
    }
    return ok;
}

bool DoUninstall(HWND p) {
    auto sel = Checked();
    if (sel.empty()) { Status(L"Error: select at least one browser."); return false; }
    bool ok = true;
    for (int i : sel)
        if (!RegDel(browsers[i].nativeRoot + L"\\com.suntzv.pratibimb")) ok = false;
    std::wstring dir = AppDir();
    if (!dir.empty() && std::filesystem::exists(dir)) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
    Status(ok ? L"Pratibimb uninstalled successfully." : L"Warning: some entries could not be removed.");
    return ok;
}

// ---------- layout ----------
// All coordinates reference the 420px-wide client area.
// PAD = 24px outer margin.  Content width = 372px.
namespace Y {
    // logo row
    const int LOGO      = 20;   // top of 44px logo block
    // mode switcher
    const int MODE      = 80;   // top of 32px switcher
    // browsers
    const int BR_LABEL  = 128;
    const int BR_ROW1   = 146;
    const int BR_ROW2   = 182;
    // quick actions
    const int QA        = 222;
    // divider
    const int DIV       = 256;
    // install steps
    const int STEPS_LBL = 266;
    const int STEP1     = 284;  // extract button  y
    const int STEP2     = 334;  // ID edit         y
    const int STEP3     = 374;  // link button     y
    // uninstall section
    const int UNST_LBL  = 266;
    const int UNST_BTN  = 288;
    // status bar
    const int STATUS    = 428;
    // window height
    const int WIN_H     = 480;
}

void ShowInstallControls(bool show) {
    int sw = show ? SW_SHOW : SW_HIDE;
    ShowWindow(g_btnExtract, sw);
    ShowWindow(g_editId,     sw);
    ShowWindow(g_btnLink,    sw);
}
void ShowUninstallControls(bool show) {
    ShowWindow(g_btnDel, show ? SW_SHOW : SW_HIDE);
}

void SwitchMode(HWND hwnd, bool install) {
    g_install = install;
    ShowInstallControls(install);
    ShowUninstallControls(!install);
    InvalidateRect(hwnd, nullptr, TRUE);
}

// ---------- GDI helpers ----------
static void FillRR(HDC dc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF border) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID, 1, border);
    auto ob = SelectObject(dc, br);
    auto op = SelectObject(dc, pn);
    RoundRect(dc, x, y, x+w, y+h, r, r);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(br); DeleteObject(pn);
}

static void TxtAt(HDC dc, const wchar_t *s, HFONT f, COLORREF c,
                  int x, int y, int w, int h, UINT flags = DT_SINGLELINE|DT_VCENTER|DT_LEFT) {
    auto of = SelectObject(dc, f);
    SetTextColor(dc, c);
    RECT r{x, y, x+w, y+h};
    DrawTextW(dc, s, -1, &r, flags);
    SelectObject(dc, of);
}

// ---------- owner-draw button paint ----------
static void PaintModeBtn(LPDRAWITEMSTRUCT d, bool active) {
    HDC dc = d->hDC;
    int x = d->rcItem.left, y = d->rcItem.top;
    int w = d->rcItem.right - x, h = d->rcItem.bottom - y;
    SetBkMode(dc, TRANSPARENT);
    if (active) {
        FillRR(dc, x, y, w, h, 6, AC_BG, AC_BD);
    } else {
        FillRR(dc, x, y, w, h, 6, BG_CARD, BD_DIM);
    }
    wchar_t txt[64]; GetWindowTextW(d->hwndItem, txt, 64);
    TxtAt(dc, txt, g_fBold,
          active ? RGB(196,181,253) : TX_LOW,
          x, y, w, h, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
}

static void PaintGhostBtn(LPDRAWITEMSTRUCT d) {
    HDC dc = d->hDC;
    int x = d->rcItem.left, y = d->rcItem.top;
    int w = d->rcItem.right - x, h = d->rcItem.bottom - y;
    bool hot = (d->itemState & ODS_HOTLIGHT) != 0;
    SetBkMode(dc, TRANSPARENT);
    FillRR(dc, x, y, w, h, 5, hot ? BG_FIELD : BG_CARD, BD_MED);
    wchar_t txt[64]; GetWindowTextW(d->hwndItem, txt, 64);
    TxtAt(dc, txt, g_fBody, TX_MED, x, y, w, h, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
}

static void PaintSurfaceBtn(LPDRAWITEMSTRUCT d) {
    HDC dc = d->hDC;
    int x = d->rcItem.left, y = d->rcItem.top;
    int w = d->rcItem.right - x, h = d->rcItem.bottom - y;
    bool hot = (d->itemState & ODS_HOTLIGHT) != 0;
    SetBkMode(dc, TRANSPARENT);
    FillRR(dc, x, y, w, h, 6, hot ? BG_FIELD : BG_CARD, BD_MED);
    wchar_t txt[64]; GetWindowTextW(d->hwndItem, txt, 64);
    TxtAt(dc, txt, g_fBold, TX_MED, x+12, y, w-12, h, DT_SINGLELINE|DT_VCENTER|DT_LEFT);
}

static void PaintAccentBtn(LPDRAWITEMSTRUCT d) {
    HDC dc = d->hDC;
    int x = d->rcItem.left, y = d->rcItem.top;
    int w = d->rcItem.right - x, h = d->rcItem.bottom - y;
    bool hot  = (d->itemState & ODS_HOTLIGHT) != 0;
    bool down = (d->itemState & ODS_SELECTED) != 0;
    COLORREF fill = down ? RGB(100,55,200) : (hot ? AC_HOT : AC_FILL);
    SetBkMode(dc, TRANSPARENT);
    FillRR(dc, x, y, w, h, 6, fill, fill);
    wchar_t txt[64]; GetWindowTextW(d->hwndItem, txt, 64);
    TxtAt(dc, txt, g_fBold, RGB(255,255,255), x, y, w, h, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
}

static void PaintDangerBtn(LPDRAWITEMSTRUCT d) {
    HDC dc = d->hDC;
    int x = d->rcItem.left, y = d->rcItem.top;
    int w = d->rcItem.right - x, h = d->rcItem.bottom - y;
    bool hot = (d->itemState & ODS_HOTLIGHT) != 0;
    SetBkMode(dc, TRANSPARENT);
    FillRR(dc, x, y, w, h, 6, hot ? RGB(55,18,18) : DG_FILL, DG_BD);
    wchar_t txt[64]; GetWindowTextW(d->hwndItem, txt, 64);
    TxtAt(dc, txt, g_fBold, DG_TX, x, y, w, h, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
}

// ---------- WM_PAINT ----------
static void OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    SetBkMode(dc, TRANSPARENT);

    const int PX = 24;
    const int IW = 420 - PX*2;   // 372

    // ── Logo row ──────────────────────────────
    // Purple square
    FillRR(dc, PX, Y::LOGO, 42, 42, 9, AC_FILL, AC_FILL);
    TxtAt(dc, L"P", g_fTitle, RGB(255,255,255),
          PX, Y::LOGO, 42, 42, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    // Name + sub
    TxtAt(dc, L"PRATIBIMB", g_fTitle, TX_HIGH,
          PX+52, Y::LOGO+2, 220, 22, DT_SINGLELINE|DT_TOP|DT_LEFT);
    TxtAt(dc, L"Native Browser Extension", g_fSmall, TX_LOW,
          PX+52, Y::LOGO+24, 220, 18, DT_SINGLELINE|DT_TOP|DT_LEFT);
    // v1.0 badge
    FillRR(dc, 420-PX-44, Y::LOGO+11, 44, 20, 10, AC_BG, AC_BD);
    TxtAt(dc, L"v 1.0", g_fSmall, RGB(167,139,250),
          420-PX-44, Y::LOGO+11, 44, 20, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

    // ── Mode switcher container ────────────────
    FillRR(dc, PX, Y::MODE, IW, 34, 8, BG_CARD, BD_DIM);

    // ── TARGET BROWSERS label ─────────────────
    TxtAt(dc, L"TARGET BROWSERS", g_fSect, TX_LOW,
          PX, Y::BR_LABEL, IW, 16, DT_SINGLELINE|DT_TOP|DT_LEFT);

    // ── Browser cards (3 cols × 2 rows) ───────
    const wchar_t *bName[6] = {L"Chrome",L"Brave",L"Edge",L"Chromium",L"Opera",L"Vivaldi"};
    // Simple single-char label and accent colors (no emoji)
    const wchar_t *bLtr[6]  = {L"C", L"B", L"E", L"C", L"O", L"V"};
    const COLORREF bClr[6]  = {
        RGB(66,133,244), RGB(251,84,43), RGB(0,168,232),
        RGB(100,181,246), RGB(255,80,80), RGB(239,65,53)
    };
    int colW = (IW - 8) / 3;  // 121px each, 4px gaps
    for (int i = 0; i < 6; ++i) {
        int col = i % 3, row = i / 3;
        int cx = PX + col * (colW + 4);
        int cy = (row == 0) ? Y::BR_ROW1 : Y::BR_ROW2;
        bool on = SendMessageW(g_cb[i], BM_GETCHECK, 0, 0) == BST_CHECKED;

        // Card bg
        FillRR(dc, cx, cy, colW, 30, 6,
               on ? AC_BG   : BG_CARD,
               on ? AC_BD   : BD_DIM);

        // Coloured dot (small circle)
        HBRUSH db = CreateSolidBrush(bClr[i]);
        HPEN   dp = CreatePen(PS_SOLID, 0, bClr[i]);
        auto ob = SelectObject(dc, db), op = SelectObject(dc, dp);
        Ellipse(dc, cx+8, cy+9, cx+20, cy+21);
        SelectObject(dc, ob); SelectObject(dc, op);
        DeleteObject(db); DeleteObject(dp);
        // Letter on dot
        TxtAt(dc, bLtr[i], g_fSmall, RGB(255,255,255),
              cx+8, cy+9, 12, 12, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

        // Browser name
        TxtAt(dc, bName[i], g_fBody,
              on ? RGB(196,181,253) : TX_MED,
              cx+24, cy, colW-38, 30, DT_SINGLELINE|DT_VCENTER|DT_LEFT);

        // Tick box
        FillRR(dc, cx+colW-18, cy+9, 12, 12, 3,
               on ? AC_FILL : BG_FIELD,
               on ? AC_FILL : BD_MED);
        if (on) TxtAt(dc, L"v", g_fSmall, RGB(255,255,255),
                      cx+colW-18, cy+9, 12, 12, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    }

    // ── Divider ──────────────────────────────
    HPEN dp = CreatePen(PS_SOLID, 1, BD_DIM);
    auto op = SelectObject(dc, dp);
    MoveToEx(dc, PX, Y::DIV, nullptr);
    LineTo(dc, PX+IW, Y::DIV);
    SelectObject(dc, op); DeleteObject(dp);

    if (g_install) {
        // ── INSTALLATION STEPS label ─────────
        TxtAt(dc, L"INSTALLATION STEPS", g_fSect, TX_LOW,
              PX, Y::STEPS_LBL, IW, 16, DT_SINGLELINE|DT_TOP|DT_LEFT);

        // Step circles + titles
        // Step 1
        FillRR(dc, PX, Y::STEP1+4, 22, 22, 11, BG_FIELD, BD_MED);
        TxtAt(dc, L"1", g_fSmall, TX_MED, PX, Y::STEP1+4, 22, 22, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        TxtAt(dc, L"Extract & open extension folder", g_fBody, TX_MED,
              PX+28, Y::STEP1, 200, 30, DT_SINGLELINE|DT_VCENTER|DT_LEFT);

        // Step 2
        FillRR(dc, PX, Y::STEP2+4, 22, 22, 11, AC_BG, AC_BD);
        TxtAt(dc, L"2", g_fSmall, RGB(196,181,253), PX, Y::STEP2+4, 22, 22, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        TxtAt(dc, L"Paste Extension ID from browser", g_fBody, TX_MED,
              PX+28, Y::STEP2, 200, 30, DT_SINGLELINE|DT_VCENTER|DT_LEFT);

        // Step 3
        FillRR(dc, PX, Y::STEP3+4, 22, 22, 11, BG_FIELD, BD_MED);
        TxtAt(dc, L"3", g_fSmall, TX_LOW, PX, Y::STEP3+4, 22, 22, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        TxtAt(dc, L"Link native host to selected browsers", g_fBody, TX_LOW,
              PX+28, Y::STEP3, 200, 30, DT_SINGLELINE|DT_VCENTER|DT_LEFT);
    } else {
        // ── UNINSTALL label ──────────────────
        TxtAt(dc, L"UNINSTALL", g_fSect, TX_LOW,
              PX, Y::UNST_LBL, IW, 16, DT_SINGLELINE|DT_TOP|DT_LEFT);
        TxtAt(dc, L"Removes registry entries and all installed files.", g_fBody, TX_MED,
              PX, Y::UNST_LBL+20, IW, 30, DT_SINGLELINE|DT_VCENTER|DT_LEFT);
    }

    // ── Status bar ───────────────────────────
    FillRR(dc, PX, Y::STATUS, IW, 32, 6, BG_CARD, BD_DIM);
    // Dot
    HBRUSH sb = CreateSolidBrush(TX_LOW);
    HPEN   sp = CreatePen(PS_SOLID, 0, TX_LOW);
    auto osb = SelectObject(dc, sb), osp = SelectObject(dc, sp);
    Ellipse(dc, PX+10, Y::STATUS+13, PX+16, Y::STATUS+19);
    SelectObject(dc, osb); SelectObject(dc, osp);
    DeleteObject(sb); DeleteObject(sp);

    EndPaint(hwnd, &ps);
}

// ---------- window procedure ----------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    case WM_CREATE: {
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        g_hwnd = hwnd;

        // Brushes
        g_brMain  = CreateSolidBrush(BG_MAIN);
        g_brCard  = CreateSolidBrush(BG_CARD);
        g_brField = CreateSolidBrush(BG_FIELD);
        g_brAcBg  = CreateSolidBrush(AC_BG);
        g_brDgBg  = CreateSolidBrush(DG_FILL);

        // Fonts
        auto MkFont = [](int h, int w, const wchar_t *n) {
            return CreateFontW(h,0,0,0,w,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,n);
        };
        g_fTitle = MkFont(18, FW_LIGHT,    L"Segoe UI Light");
        g_fSect  = MkFont(10, FW_BOLD,     L"Segoe UI");
        g_fBody  = MkFont(13, FW_NORMAL,   L"Segoe UI");
        g_fBold  = MkFont(13, FW_SEMIBOLD, L"Segoe UI Semibold");
        g_fMono  = MkFont(12, FW_NORMAL,   L"Consolas");
        g_fSmall = MkFont(11, FW_NORMAL,   L"Segoe UI");

        const int PX = 24, IW = 420 - PX*2;

        // Mode switcher — two owner-draw buttons side by side inside the card
        int hw = (IW - 4) / 2;
        g_btnInst   = CreateWindowW(L"BUTTON", L"Install",
            WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,
            PX+2, Y::MODE+2, hw, 30, hwnd, (HMENU)(intptr_t)IDC_MODE_INSTALL, nullptr, nullptr);
        g_btnUninst = CreateWindowW(L"BUTTON", L"Uninstall",
            WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,
            PX+2+hw+2, Y::MODE+2, hw, 30, hwnd, (HMENU)(intptr_t)IDC_MODE_UNINSTALL, nullptr, nullptr);

        // Hidden checkboxes for state
        for (int i = 0; i < 6; ++i) {
            g_cb[i] = CreateWindowW(L"BUTTON", L"",
                WS_CHILD|BS_AUTOCHECKBOX, 0,0,0,0,
                hwnd, (HMENU)(intptr_t)(IDC_CB_BASE+i), nullptr, nullptr);
        }
        // Chrome, Brave, Edge checked by default
        SendMessageW(g_cb[0], BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(g_cb[1], BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(g_cb[2], BM_SETCHECK, BST_CHECKED, 0);

        // Select All / Clear All
        g_selAll = CreateWindowW(L"BUTTON", L"Select All",
            WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,
            PX, Y::QA, 88, 24, hwnd, (HMENU)(intptr_t)IDC_SELECT_ALL, nullptr, nullptr);
        g_clrAll = CreateWindowW(L"BUTTON", L"Clear All",
            WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,
            PX+94, Y::QA, 88, 24, hwnd, (HMENU)(intptr_t)IDC_CLEAR_ALL, nullptr, nullptr);

        // ── Install controls ──
        // Step 1: Extract button (left-aligned, after step number area)
        g_btnExtract = CreateWindowW(L"BUTTON", L"Extract & Open Folder",
            WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,
            PX+28, Y::STEP1, IW-28, 30, hwnd, (HMENU)(intptr_t)IDC_BTN_EXTRACT, nullptr, nullptr);

        // Step 2: ID edit field
        g_editId = CreateWindowExW(0, L"EDIT", L"",
            WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,
            PX+28, Y::STEP2+2, IW-28, 26, hwnd, (HMENU)(intptr_t)IDC_EDIT_ID, nullptr, nullptr);
        SendMessageW(g_editId, WM_SETFONT, (WPARAM)g_fMono, TRUE);
        SendMessageW(g_editId, EM_SETCUEBANNER, FALSE, (LPARAM)L"Paste 32-character extension ID");

        // Step 3: Link button
        g_btnLink = CreateWindowW(L"BUTTON", L"Link Native Host  →",
            WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,
            PX+28, Y::STEP3, 180, 30, hwnd, (HMENU)(intptr_t)IDC_BTN_LINK, nullptr, nullptr);

        // ── Uninstall control (hidden) ──
        g_btnDel = CreateWindowW(L"BUTTON", L"Uninstall Everything",
            WS_CHILD|BS_OWNERDRAW,
            PX+28, Y::UNST_BTN, IW-28, 34, hwnd, (HMENU)(intptr_t)IDC_BTN_UNINSTALL, nullptr, nullptr);

        // Status label (inside the drawn card)
        g_status = CreateWindowW(L"STATIC", L"Ready — select browsers and begin.",
            WS_VISIBLE|WS_CHILD|SS_LEFT,
            PX+22, Y::STATUS, IW-28, 32, hwnd, (HMENU)(intptr_t)IDC_STATUS, nullptr, nullptr);
        SendMessageW(g_status, WM_SETFONT, (WPARAM)g_fSmall, TRUE);

        break;
    }

    case WM_ERASEBKGND: {
        HDC dc = (HDC)wp;
        RECT r; GetClientRect(hwnd, &r);
        FillRect(dc, &r, g_brMain);
        return 1;
    }

    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_DRAWITEM: {
        auto *d = (LPDRAWITEMSTRUCT)lp;
        int id = (int)d->CtlID;
        if      (id == IDC_MODE_INSTALL)   PaintModeBtn(d, g_install);
        else if (id == IDC_MODE_UNINSTALL) PaintModeBtn(d, !g_install);
        else if (id == IDC_SELECT_ALL || id == IDC_CLEAR_ALL) PaintGhostBtn(d);
        else if (id == IDC_BTN_EXTRACT)    PaintSurfaceBtn(d);
        else if (id == IDC_BTN_LINK)       PaintAccentBtn(d);
        else if (id == IDC_BTN_UNINSTALL)  PaintDangerBtn(d);
        return TRUE;
    }

    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, TX_LOW);
        return (LRESULT)g_brMain;
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wp;
        SetBkColor(dc, BG_FIELD);
        SetTextColor(dc, TX_MED);
        return (LRESULT)g_brField;
    }
    case WM_CTLCOLORBTN:
        return (LRESULT)g_brMain;

    // Hit-test browser grid (click anywhere on a card)
    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lp), my = HIWORD(lp);
        const int PX = 24, IW = 420-PX*2;
        int colW = (IW-8)/3;
        for (int i = 0; i < 6; ++i) {
            int col = i%3, row = i/3;
            int cx = PX + col*(colW+4);
            int cy = (row == 0) ? Y::BR_ROW1 : Y::BR_ROW2;
            if (mx>=cx && mx<=cx+colW && my>=cy && my<=cy+30) {
                bool on = SendMessageW(g_cb[i], BM_GETCHECK,0,0) == BST_CHECKED;
                SendMessageW(g_cb[i], BM_SETCHECK, on ? BST_UNCHECKED : BST_CHECKED, 0);
                InvalidateRect(hwnd, nullptr, TRUE);
                break;
            }
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_MODE_INSTALL:
            if (!g_install) SwitchMode(hwnd, true);
            else InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case IDC_MODE_UNINSTALL:
            if (g_install) SwitchMode(hwnd, false);
            else InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case IDC_SELECT_ALL:
            for (int i=0;i<6;i++) SendMessageW(g_cb[i], BM_SETCHECK, BST_CHECKED, 0);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case IDC_CLEAR_ALL:
            for (int i=0;i<6;i++) SendMessageW(g_cb[i], BM_SETCHECK, BST_UNCHECKED, 0);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        case IDC_BTN_EXTRACT:    DoExtract(hwnd);   break;
        case IDC_BTN_LINK:       DoLink(hwnd);      break;
        case IDC_BTN_UNINSTALL:  DoUninstall(hwnd); break;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        DeleteObject(g_brMain); DeleteObject(g_brCard);
        DeleteObject(g_brField);DeleteObject(g_brAcBg); DeleteObject(g_brDgBg);
        DeleteObject(g_fTitle); DeleteObject(g_fSect);  DeleteObject(g_fBody);
        DeleteObject(g_fBold);  DeleteObject(g_fMono);  DeleteObject(g_fSmall);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

// ---------- entry point ----------
int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int ncs) {
    INITCOMMONCONTROLSEX icx = { sizeof(icx), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icx);

    const wchar_t CLS[] = L"PratibimbWnd";
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hi;
    wc.lpszClassName = CLS;
    wc.hCursor       = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG_MAIN);
    RegisterClassW(&wc);

    // Client area: 420 × Y::WIN_H.  Add non-client frame.
    RECT wr = { 0, 0, 420, Y::WIN_H };
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
    return (int)msg.wParam;
}