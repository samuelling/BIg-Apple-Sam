// Big Apple Sam as a Windows screen saver (.scr).
//
// A thin native shell around the same index.html the web version uses. The page is
// embedded in this file as a resource, unpacked to %LOCALAPPDATA%\Big Apple Sam\site and
// shown in Microsoft Edge WebView2 (built into Windows 10 and 11) from a private virtual
// host. Nothing is fetched from the network, and navigation anywhere else is refused.
//
// Windows starts a screen saver with one of:
//   /s            run full screen (one window per monitor)
//   /p <hwnd>     draw the little preview inside the Screen Saver Settings dialog
//   /c[:<hwnd>]   show the options dialog (also when started with no arguments)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <cwchar>
#include <cstdlib>
#include "WebView2.h"
#include "resource.h"

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

enum class Mode { Run, Config, Preview };

const wchar_t kClass[] = L"BigAppleSamScreenSaver";
const wchar_t kHost[] = L"big-apple-sam.example";
const wchar_t kOrigin[] = L"https://big-apple-sam.example/";
const wchar_t kRegKey[] = L"Software\\Big Apple Sam";

struct Screen {
    HWND hwnd = nullptr;
    int index = 0;
    bool preview = false;
    ComPtr<ICoreWebView2Controller> ctl;
    ComPtr<ICoreWebView2> wv;
};

Mode g_mode = Mode::Config;
HINSTANCE g_inst = nullptr;
HWND g_parent = nullptr;
std::vector<Screen*> g_screens;
ComPtr<ICoreWebView2Environment> g_env;
std::wstring g_dataDir, g_siteDir;
bool g_quitting = false;
bool g_noRuntime = false;
POINT g_pt0{};
ULONGLONG g_t0 = 0;

// ---------------------------------------------------------------- settings

struct Settings {
    std::wstring code;
    bool night = false, captions = true, cyber = false;
    DWORD speed = 1;    // 1, 10 or 60
    DWORD display = 1;  // 0 CGA, 1 VGA, 2 SVGA, 3 EGA (same tags as the Mac options sheet)
};

DWORD RegDword(HKEY k, const wchar_t* name, DWORD def) {
    DWORD v = 0, n = sizeof(v);
    return RegGetValueW(k, nullptr, name, RRF_RT_REG_DWORD, nullptr, &v, &n) == ERROR_SUCCESS ? v : def;
}

Settings LoadSettings() {
    Settings s;
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegKey, 0, KEY_READ, &k) != ERROR_SUCCESS) return s;
    wchar_t buf[64] = {};
    DWORD n = sizeof(buf);
    if (RegGetValueW(k, nullptr, L"cityCode", RRF_RT_REG_SZ, nullptr, buf, &n) == ERROR_SUCCESS) s.code = buf;
    s.night = RegDword(k, L"alwaysNight", 0) != 0;
    s.captions = RegDword(k, L"captions", 1) != 0;
    s.cyber = RegDword(k, L"cyber", 0) != 0;
    s.speed = RegDword(k, L"speed", 1);
    s.display = RegDword(k, L"display", 1);
    RegCloseKey(k);
    if (s.speed != 1 && s.speed != 10 && s.speed != 60) s.speed = 1;
    if (s.display > 3) s.display = 1;
    return s;
}

void SaveSettings(const Settings& s) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) return;
    RegSetValueExW(k, L"cityCode", 0, REG_SZ, reinterpret_cast<const BYTE*>(s.code.c_str()),
                   static_cast<DWORD>((s.code.size() + 1) * sizeof(wchar_t)));
    auto put = [&](const wchar_t* name, DWORD v) { RegSetValueExW(k, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v)); };
    put(L"alwaysNight", s.night);
    put(L"captions", s.captions);
    put(L"cyber", s.cyber);
    put(L"speed", s.speed);
    put(L"display", s.display);
    RegCloseKey(k);
}

// City codes look like "bod5nr-g09": keep letters, digits and dashes only.
std::wstring CleanCode(const std::wstring& in) {
    std::wstring out;
    for (wchar_t c : in) {
        if (c >= L'A' && c <= L'Z') c = c - L'A' + L'a';
        if ((c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9') || c == L'-') out += c;
        if (out.size() >= 40) break;
    }
    return out;
}

// ---------------------------------------------------------------- page

std::wstring SaverScript(const Screen& s) {
    const Settings st = LoadSettings();
    const wchar_t* gfx[] = {L"cga", L"vga", L"svga", L"ega"};
    wchar_t cfg[512];
    swprintf_s(cfg, L"window.BAS_SAVER={\"code\":\"%ls\",\"night\":%ls,\"captions\":%ls,\"speed\":%u,\"gfx\":\"%ls\","
                    L"\"cyber\":%ls,\"display\":%d,\"preview\":%ls};",
               CleanCode(st.code).c_str(), st.night ? L"true" : L"false",
               (st.captions && !s.preview) ? L"true" : L"false", st.speed, gfx[st.display],
               st.cyber ? L"true" : L"false", s.index, s.preview ? L"true" : L"false");
    std::wstring js = cfg;
    if (!s.preview) {
        // Any key, click, wheel or real mouse movement ends the screen saver.
        js += LR"JS(
(()=>{let x0=null,y0=null;const bye=()=>{try{chrome.webview.postMessage('bye')}catch(e){}};
addEventListener('mousemove',e=>{if(x0===null){x0=e.screenX;y0=e.screenY;return}
  if(Math.abs(e.screenX-x0)+Math.abs(e.screenY-y0)>12)bye()},true);
for(const t of ['mousedown','pointerdown','keydown','wheel','touchstart'])
  addEventListener(t,e=>{e.stopPropagation();bye()},true);})();
)JS";
    }
    return js;
}

std::wstring KnownFolder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_CREATE, nullptr, &p))) out = p;
    CoTaskMemFree(p);
    return out;
}

// Unpack the embedded page if the copy on disk is missing or out of date.
bool UnpackSite() {
    g_dataDir = KnownFolder(FOLDERID_LocalAppData) + L"\\Big Apple Sam";
    g_siteDir = g_dataDir + L"\\site";
    SHCreateDirectoryExW(nullptr, g_siteDir.c_str(), nullptr);

    HRSRC r = FindResourceW(g_inst, MAKEINTRESOURCEW(IDR_INDEX), RT_RCDATA);
    if (!r) return false;
    const DWORD size = SizeofResource(g_inst, r);
    const void* data = LockResource(LoadResource(g_inst, r));
    if (!data || !size) return false;

    const std::wstring path = g_siteDir + L"\\index.html";
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f != INVALID_HANDLE_VALUE) {
        std::vector<char> cur(size + 1);
        DWORD got = 0;
        const BOOL ok = ReadFile(f, cur.data(), size + 1, &got, nullptr);
        CloseHandle(f);
        if (ok && got == size && memcmp(cur.data(), data, size) == 0) return true;
    }
    const std::wstring tmp = path + L".tmp";
    f = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD wrote = 0;
    const BOOL ok = WriteFile(f, data, size, &wrote, nullptr);
    CloseHandle(f);
    if (!ok || wrote != size) return false;
    return MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}

// ---------------------------------------------------------------- lifetime

void Quit() {
    if (g_quitting) return;
    g_quitting = true;
    for (Screen* s : g_screens)
        if (s->ctl) s->ctl->Close();
    PostQuitMessage(0);
}

// Ignore input for the first moment: the click or key that started us is still settling.
bool PastGrace() { return GetTickCount64() - g_t0 > 1200; }

void UserActivity() {
    if (g_mode == Mode::Run && PastGrace()) Quit();
}

void AttachWebView(Screen* s) {
    g_env->CreateCoreWebView2Controller(s->hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [s](HRESULT hr, ICoreWebView2Controller* ctl) -> HRESULT {
            if (FAILED(hr) || !ctl || g_quitting) return S_OK;
            s->ctl = ctl;
            ctl->get_CoreWebView2(&s->wv);

            ComPtr<ICoreWebView2Controller2> c2;
            if (SUCCEEDED(s->ctl.As(&c2))) c2->put_DefaultBackgroundColor(COREWEBVIEW2_COLOR{255, 0, 0, 0});
            RECT rc;
            GetClientRect(s->hwnd, &rc);
            ctl->put_Bounds(rc);

            ComPtr<ICoreWebView2Settings> st;
            s->wv->get_Settings(&st);
            st->put_AreDevToolsEnabled(FALSE);
            st->put_AreDefaultContextMenusEnabled(FALSE);
            st->put_AreDefaultScriptDialogsEnabled(FALSE);
            st->put_IsStatusBarEnabled(FALSE);
            st->put_IsZoomControlEnabled(FALSE);
            st->put_IsBuiltInErrorPageEnabled(FALSE);
            ComPtr<ICoreWebView2Settings3> st3;
            if (SUCCEEDED(st.As(&st3))) st3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
            ComPtr<ICoreWebView2Settings5> st5;
            if (SUCCEEDED(st.As(&st5))) st5->put_IsPinchZoomEnabled(FALSE);
            ComPtr<ICoreWebView2Settings6> st6;
            if (SUCCEEDED(st.As(&st6))) st6->put_IsSwipeNavigationEnabled(FALSE);

            s->wv->AddScriptToExecuteOnDocumentCreated(SaverScript(*s).c_str(), nullptr);

            EventRegistrationToken tok;
            // Stay on the bundled page: no links out, no new windows.
            s->wv->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* a) -> HRESULT {
                    LPWSTR uri = nullptr;
                    a->get_Uri(&uri);
                    const bool ours = uri && wcsncmp(uri, kOrigin, wcslen(kOrigin)) == 0;
                    CoTaskMemFree(uri);
                    if (!ours) a->put_Cancel(TRUE);
                    return S_OK;
                }).Get(), &tok);
            s->wv->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                [](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* a) -> HRESULT {
                    a->put_Handled(TRUE);
                    return S_OK;
                }).Get(), &tok);

            if (!s->preview) {
                s->wv->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs*) -> HRESULT {
                        UserActivity();
                        return S_OK;
                    }).Get(), &tok);
                ctl->add_AcceleratorKeyPressed(Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                    [](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* a) -> HRESULT {
                        a->put_Handled(TRUE);
                        COREWEBVIEW2_KEY_EVENT_KIND kind;
                        if (SUCCEEDED(a->get_KeyEventKind(&kind)) &&
                            (kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN || kind == COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN))
                            UserActivity();
                        return S_OK;
                    }).Get(), &tok);
            }

            ComPtr<ICoreWebView2_3> w3;
            if (SUCCEEDED(s->wv.As(&w3)))
                w3->SetVirtualHostNameToFolderMapping(kHost, g_siteDir.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY);
            s->wv->Navigate((std::wstring(kOrigin) + L"index.html").c_str());
            if (!s->preview && s->index == 0) ctl->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
            return S_OK;
        }).Get());
}

// ---------------------------------------------------------------- windows

LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    Screen* s = reinterpret_cast<Screen*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    switch (msg) {
    case WM_NCCREATE:
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams));
        break;
    case WM_SIZE:
        if (s && s->ctl) {
            RECT rc;
            GetClientRect(h, &rc);
            s->ctl->put_Bounds(rc);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc;
        GetClientRect(h, &rc);
        FillRect(dc, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        if (g_noRuntime) {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(150, 150, 170));
            DrawTextW(dc, L"Big Apple Sam needs the Microsoft Edge WebView2 Runtime.", -1, &rc,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        EndPaint(h, &ps);
        return 0;
    }
    case WM_SETCURSOR:
        if (g_mode == Mode::Run) {
            SetCursor(nullptr);
            return TRUE;
        }
        break;
    case WM_TIMER:
        if (g_mode == Mode::Preview) {
            if (!IsWindow(g_parent)) Quit();
        } else if (PastGrace()) {
            // Polling catches input wherever it lands (page, browser or this window).
            POINT p;
            if (GetCursorPos(&p) && abs(p.x - g_pt0.x) + abs(p.y - g_pt0.y) > 12) Quit();
            for (int vk = VK_LBUTTON; vk <= VK_OEM_CLEAR && !g_quitting; ++vk)
                if (GetAsyncKeyState(vk) & 0x8000) Quit();
        } else {
            GetCursorPos(&g_pt0);
        }
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_MOUSEWHEEL:
        UserActivity();
        return 0;
    case WM_SYSCOMMAND:
        if (g_mode == Mode::Run && (wp & 0xFFF0) == SC_SCREENSAVE) return 0;  // already running
        break;
    case WM_CLOSE:
        Quit();
        return 0;
    case WM_DESTROY:
        Quit();
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

BOOL CALLBACK AddMonitor(HMONITOR, HDC, LPRECT r, LPARAM) {
    auto* s = new Screen;
    s->index = static_cast<int>(g_screens.size());
    s->hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kClass, L"Big Apple Sam", WS_POPUP | WS_VISIBLE,
                              r->left, r->top, r->right - r->left, r->bottom - r->top, nullptr, nullptr, g_inst, s);
    if (s->hwnd) g_screens.push_back(s);
    else delete s;
    return TRUE;
}

// ---------------------------------------------------------------- options dialog

INT_PTR CALLBACK ConfigProc(HWND d, UINT msg, WPARAM wp, LPARAM) {
    switch (msg) {
    case WM_INITDIALOG: {
        const Settings s = LoadSettings();
        SetDlgItemTextW(d, IDC_CODE, s.code.c_str());
        SendDlgItemMessageW(d, IDC_CODE, EM_SETLIMITTEXT, 40, 0);
        SendDlgItemMessageW(d, IDC_CODE, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"e.g. bod5nr-g09"));
        auto fill = [&](int id, std::initializer_list<std::pair<const wchar_t*, DWORD>> items, DWORD cur) {
            for (auto& it : items) {
                const LRESULT i = SendDlgItemMessageW(d, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(it.first));
                SendDlgItemMessageW(d, id, CB_SETITEMDATA, i, it.second);
                if (it.second == cur) SendDlgItemMessageW(d, id, CB_SETCURSEL, i, 0);
            }
        };
        fill(IDC_SPEED, {{L"Live (real time)", 1}, {L"10× faster", 10}, {L"60× faster", 60}}, s.speed);
        fill(IDC_DISPLAY, {{L"CGA (4 colors)", 0}, {L"EGA (16 colors)", 3}, {L"VGA", 1}, {L"SVGA (sharper close-ups)", 2}}, s.display);
        CheckDlgButton(d, IDC_NIGHT, s.night ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(d, IDC_CYBER, s.cyber ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(d, IDC_CAPTIONS, s.captions ? BST_CHECKED : BST_UNCHECKED);
        HICON icon = LoadIconW(g_inst, MAKEINTRESOURCEW(IDI_APP));
        SendMessageW(d, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
        SendMessageW(d, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            Settings s;
            wchar_t buf[64] = {};
            GetDlgItemTextW(d, IDC_CODE, buf, 64);
            s.code = CleanCode(buf);
            auto pick = [&](int id) {
                const LRESULT i = SendDlgItemMessageW(d, id, CB_GETCURSEL, 0, 0);
                return static_cast<DWORD>(SendDlgItemMessageW(d, id, CB_GETITEMDATA, i == CB_ERR ? 0 : i, 0));
            };
            s.speed = pick(IDC_SPEED);
            s.display = pick(IDC_DISPLAY);
            s.night = IsDlgButtonChecked(d, IDC_NIGHT) == BST_CHECKED;
            s.cyber = IsDlgButtonChecked(d, IDC_CYBER) == BST_CHECKED;
            s.captions = IsDlgButtonChecked(d, IDC_CAPTIONS) == BST_CHECKED;
            SaveSettings(s);
            EndDialog(d, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) {
            EndDialog(d, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// ---------------------------------------------------------------- arguments

HWND ParseHwnd(const wchar_t* s) {
    while (*s == L':' || *s == L' ') ++s;
    return *s ? reinterpret_cast<HWND>(static_cast<INT_PTR>(_wcstoui64(s, nullptr, 10))) : nullptr;
}

void ParseArgs() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    g_mode = Mode::Config;
    if (argv && argc > 1) {
        const wchar_t* a = argv[1];
        if (*a == L'/' || *a == L'-') ++a;
        const wchar_t c = static_cast<wchar_t>(towlower(*a));
        const wchar_t* rest = a + 1;
        if (c == L's') {
            g_mode = Mode::Run;
        } else if (c == L'p' || c == L'l') {
            g_mode = Mode::Preview;
            g_parent = ParseHwnd(*rest ? rest : (argc > 2 ? argv[2] : L""));
        } else if (c == L'c') {
            g_parent = ParseHwnd(*rest ? rest : (argc > 2 ? argv[2] : L""));
        }
    }
    LocalFree(argv);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
    g_inst = inst;
    ParseArgs();

    if (g_mode == Mode::Config) {
        INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&icc);
        DialogBoxParamW(inst, MAKEINTRESOURCEW(IDD_CONFIG), g_parent ? g_parent : GetForegroundWindow(), ConfigProc, 0);
        return 0;
    }
    if (g_mode == Mode::Preview && !IsWindow(g_parent)) return 0;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool unpacked = UnpackSite();

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APP));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = kClass;
    RegisterClassExW(&wc);

    if (g_mode == Mode::Preview) {
        RECT rc;
        GetClientRect(g_parent, &rc);
        auto* s = new Screen;
        s->preview = true;
        s->hwnd = CreateWindowExW(0, kClass, L"Big Apple Sam", WS_CHILD | WS_VISIBLE, 0, 0, rc.right, rc.bottom,
                                  g_parent, nullptr, inst, s);
        if (!s->hwnd) return 0;
        g_screens.push_back(s);
    } else {
        EnumDisplayMonitors(nullptr, nullptr, AddMonitor, 0);
        if (g_screens.empty()) return 0;
        ShowCursor(FALSE);
    }
    g_t0 = GetTickCount64();
    GetCursorPos(&g_pt0);
    SetTimer(g_screens[0]->hwnd, 1, 100, nullptr);

    LPWSTR version = nullptr;
    const bool haveRuntime = SUCCEEDED(GetAvailableCoreWebView2BrowserVersionString(nullptr, &version)) && version;
    CoTaskMemFree(version);
    if (!unpacked || !haveRuntime) {
        g_noRuntime = true;
    } else {
        const std::wstring userData = g_dataDir + L"\\WebView2";
        const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userData.c_str(), nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [](HRESULT hr, ICoreWebView2Environment* env) -> HRESULT {
                    if (FAILED(hr) || !env) {
                        g_noRuntime = true;
                        for (Screen* s : g_screens) InvalidateRect(s->hwnd, nullptr, TRUE);
                        return S_OK;
                    }
                    g_env = env;
                    for (Screen* s : g_screens) AttachWebView(s);
                    return S_OK;
                }).Get());
        if (FAILED(hr)) g_noRuntime = true;
    }
    for (Screen* s : g_screens) InvalidateRect(s->hwnd, nullptr, TRUE);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    for (Screen* s : g_screens) {
        s->wv.Reset();
        s->ctl.Reset();
    }
    g_env.Reset();
    CoUninitialize();
    return 0;
}
