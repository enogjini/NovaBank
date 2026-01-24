// NovaBank.cpp : Single-file app. All necessary headers inlined and resources removed.

#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#define MAX_LOADSTRING 100

// Control IDs
#define ID_EDIT_USERNAME 101
#define ID_EDIT_PASSWORD 102
#define ID_BTN_CLIENT 201
#define ID_BTN_ADMIN 202
#define ID_BTN_SIGNUP 203

// Signup dialog control IDs
#define ID_SIGN_NAME 301
#define ID_SIGN_PHONE 302
#define ID_SIGN_BAL 303
#define ID_SIGN_OK 304
#define ID_SIGN_CANCEL 305
#define ID_SIGN_USERNAME 306
#define ID_SIGN_PASSWORD 307

// Dashboard control IDs
#define ID_DASH_BAL_LABEL 401
#define ID_DASH_TRANSFER_TO 402
#define ID_DASH_TRANSFER_AMT 403
#define ID_DASH_TRANSFER_BTN 404
#define ID_DASH_ADD_BTN 405
#define ID_DASH_WITHDRAW_BTN 406


// Admin control IDs - ADD THESE
#define ID_ADMIN_LISTBOX 501
#define ID_ADMIN_DELETE_BTN 502
#define ID_ADMIN_REFRESH_BTN 503


// Globals
HINSTANCE hInst;                               // current instance
WCHAR szTitle[MAX_LOADSTRING] = L"NovaBank"; // app title
WCHAR szWindowClass[MAX_LOADSTRING] = L"NovaBankMainClass"; // window class

static HWND editUser = nullptr;                // username edit
static HWND editPass = nullptr;                // password edit

static std::vector<std::string> accounts;      // store records: user|pass|name|phone|balance
static const char* ACCOUNTS_FILENAME = "accounts.txt";

// Simple state for dashboard window stored in GWLP_USERDATA
struct DashState { double balance = 0.0; std::wstring user; };

// Forward declarations
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SignupWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DashboardWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AdminWndProc(HWND, UINT, WPARAM, LPARAM);

// Helpers: split, conversions, load/save, update balance
static std::vector<std::string> SplitRecord(const std::string& rec)
{
    std::vector<std::string> parts;
    std::istringstream ss(rec);
    std::string cur;
    while (std::getline(ss, cur, '|')) parts.push_back(cur);
    return parts;
}

static std::string ToUtf8(const std::wstring& s)
{
    return std::string(s.begin(), s.end()); // simple conversion for ASCII-only use
}

static std::wstring ToW(const std::string& s)
{
    return std::wstring(s.begin(), s.end());
}

static void LoadAccounts()
{
    accounts.clear();
    std::ifstream in(ACCOUNTS_FILENAME);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) if (!line.empty()) accounts.push_back(line);
}

static void SaveAccounts()
{
    std::ofstream out(ACCOUNTS_FILENAME, std::ios::trunc);
    if (!out) return;
    for (auto &r : accounts) out << r << '\n';
}

static int FindAccountIndexByUsername(const std::wstring& username)
{
    std::string u = ToUtf8(username);
    for (size_t i = 0; i < accounts.size(); ++i)
    {
        auto p = SplitRecord(accounts[i]);
        if (!p.empty() && p[0] == u) return (int)i;
    }
    return -1;
}

// Replace the balance field for a specific account and persist
static void UpdateAccountBalance(const std::wstring& username, double bal)
{
    int idx = FindAccountIndexByUsername(username);
    if (idx < 0) return;
    auto parts = SplitRecord(accounts[idx]);
    if (parts.size() < 5) parts.resize(5);
    parts[4] = std::to_string(bal);
    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i) oss << '|';
        oss << parts[i];
    }
    accounts[idx] = oss.str();
    SaveAccounts();
}

static void DeleteAccount(const std::wstring& username)
{
    int idx = FindAccountIndexByUsername(username);
    if (idx >= 0)
    {
        accounts.erase(accounts.begin() + idx);
        SaveAccounts();
    }
}

static void PopulateAdminList(HWND hListBox)
{
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    for (const auto& acc : accounts)
    {
        auto parts = SplitRecord(acc);
        if (parts.size() >= 5)
        {
            std::wstring entry = L"User: " + ToW(parts[0]) +
                L" | Name: " + ToW(parts[2]) +
                L" | Balance: $" + ToW(parts[4]);
            SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)entry.c_str());
        }
    }
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);
    LoadAccounts(); // restore stored accounts

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW w = {};
    w.cbSize = sizeof(w);
    w.style = CS_HREDRAW | CS_VREDRAW;
    w.lpfnWndProc = WndProc;
    w.hInstance = hInstance;
    w.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    w.hCursor = LoadCursor(nullptr, IDC_ARROW);
    w.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    w.lpszMenuName = nullptr;
    w.lpszClassName = szWindowClass;
    w.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    ATOM a = RegisterClassExW(&w);

    // Signup class
    WNDCLASSEXW s = {};
    s.cbSize = sizeof(s);
    s.style = CS_HREDRAW | CS_VREDRAW;
    s.lpfnWndProc = SignupWndProc;
    s.hInstance = hInstance;
    s.hCursor = LoadCursor(nullptr, IDC_ARROW);
    s.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    s.lpszClassName = L"SignupWindowClass";
    RegisterClassExW(&s);

    // Dashboard class
    WNDCLASSEXW d = {};
    d.cbSize = sizeof(d);
    d.style = CS_HREDRAW | CS_VREDRAW;
    d.lpfnWndProc = DashboardWndProc;
    d.hInstance = hInstance;
    d.hCursor = LoadCursor(nullptr, IDC_ARROW);
    d.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    d.lpszClassName = L"DashboardWindowClass";
    RegisterClassExW(&d);

	WNDCLASSEXW admin = {}; //admin class
    admin.cbSize = sizeof(admin);
    admin.style = CS_HREDRAW | CS_VREDRAW;
    admin.lpfnWndProc = AdminWndProc;
    admin.hInstance = hInstance;
    admin.hCursor = LoadCursor(nullptr, IDC_ARROW);
    admin.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    admin.lpszClassName = L"AdminWindowClass";
    RegisterClassExW(&admin);

    return a;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND h = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 400, 260, nullptr, nullptr, hInstance, nullptr);
    if (!h) return FALSE;
    ShowWindow(h, nCmdShow);
    UpdateWindow(h);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        // Labels
        CreateWindowW(L"STATIC", L"Username:", WS_CHILD | WS_VISIBLE, 20, 20, 80, 20,
                      hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Fjalekalimi:", WS_CHILD | WS_VISIBLE, 20, 60, 80, 20,
                      hWnd, nullptr, hInst, nullptr);
        // Edits
        editUser = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            110, 18, 250, 24, hWnd, (HMENU)ID_EDIT_USERNAME, hInst, nullptr);
        editPass = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
            110, 58, 250, 24, hWnd, (HMENU)ID_EDIT_PASSWORD, hInst, nullptr);
        // Buttons: client login, admin login, and signup
        CreateWindowW(L"BUTTON", L"Kycu si Klient",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 110, 155, 30, hWnd, (HMENU)ID_BTN_CLIENT, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Kycu si Admin",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            195, 110, 155, 30, hWnd, (HMENU)ID_BTN_ADMIN, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Nuk keni llogari? Krijoni nje",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 150, 200, 30, hWnd, (HMENU)ID_BTN_SIGNUP, hInst, nullptr);
        break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int ev = HIWORD(wParam);
        if (ev == BN_CLICKED)
        {
            if (id == ID_BTN_SIGNUP)
            {
                RECT r; GetWindowRect(hWnd, &r);
                int w = 420, h = 320;
                int x = r.left + (r.right - r.left - w) / 2;
                int y = r.top + (r.bottom - r.top - h) / 2;
                CreateWindowW(L"SignupWindowClass", L"Krijo llogari",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                              x, y, w, h, hWnd, nullptr, hInst, nullptr);
                return 0;
            }

            if (id == ID_BTN_CLIENT)
            {
                // read edits
                wchar_t u[256] = {}, p[256] = {};
                GetWindowTextW(editUser, u, _countof(u));
                GetWindowTextW(editPass, p, _countof(p));
                if (wcslen(u) == 0 || wcslen(p) == 0)
                {
                    MessageBoxW(hWnd, L"Shkruani username dhe fjalekalimin per tu kycur.",
                                L"Login", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                std::wstring user(u), pass(p);
                int idx = FindAccountIndexByUsername(user);
                if (idx < 0)
                {
                    MessageBoxW(hWnd, L"Username ose fjalekalim i gabuar.", L"Logimi deshtoi!",
                                MB_OK | MB_ICONERROR);
                    return 0;
                }
                auto parts = SplitRecord(accounts[idx]);
                std::wstring stored = (parts.size()>1) ? ToW(parts[1]) : L"";;
                if (stored != pass)
                {
                    MessageBoxW(hWnd, L"Username ose fjalekalim i gabuar.", L"Logimi deshtoi!",
                                MB_OK | MB_ICONERROR);
                    return 0;
                }
                // open dashboard and pass username
                RECT r; GetWindowRect(hWnd, &r);
                int w = 420, h = 300;
                int x = r.left + (r.right - r.left - w) / 2;
                int y = r.top + (r.bottom - r.top - h) / 2;
                wchar_t* uname = _wcsdup(user.c_str());
                HWND hd = CreateWindowW(L"DashboardWindowClass", L"Client Dashboard",
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                    x, y, w, h, hWnd, nullptr, hInst, (LPVOID)uname);
                if (!hd) { MessageBoxW(hWnd, L"Failed to create dashboard.", L"Error",
                                       MB_OK | MB_ICONERROR); free(uname); }
                return 0;
            }

            if (id == ID_BTN_ADMIN)
            {
                // read edits
                wchar_t u[256] = {}, p[256] = {};
                GetWindowTextW(editUser, u, _countof(u));
                GetWindowTextW(editPass, p, _countof(p));
                if (wcslen(u) == 0 || wcslen(p) == 0)
                {
                    MessageBoxW(hWnd, L"Vendos kredencialet e administratorit.",
                                L"Login", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                std::wstring user(u), pass(p);
                // Check hard-coded admin credentials
                if (user != L"admin" || pass != L"adminpass")
                {
                    MessageBoxW(hWnd, L"Invalid admin username or password.", L"Login Failed",
                                MB_OK | MB_ICONERROR);
                    return 0;
                }
                // Admin login successful, show accounts summary
				RECT r; GetWindowRect(hWnd, &r);
				int w = 500, h = 400;
				int x = r.left + (r.right - r.left - w) / 2;
				int y = r.top + (r.bottom - r.top - h) / 2;
				CreateWindowW(L"AdminWindowClass", L"Admin Dashboard", 
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                              x, y, w, h, hWnd, nullptr, hInst, nullptr);
                return 0;
                    
                

               /* std::wstring summary = L"Permbledhja e llogarise:\n\n";
                for (const auto& acc : accounts)
                {
                    auto parts = SplitRecord(acc);
                    if (parts.size() >= 5)
                    {
                        summary += L"Perdoruesi: " + ToW(parts[0]) + L"\n" +
                                   L"Emri: " + ToW(parts[2]) + L"\n" +
                                   L"Celulari: " + ToW(parts[3]) + L"\n" +
                                   L"Balanca: $" + ToW(parts[4]) + L"\n\n";
                    }
                }
                MessageBoxW(hWnd, summary.c_str(), L"Permbledhja e llogarive", MB_OK | MB_ICONINFORMATION);
                return 0; */
            }
        }
        break;
    }

    case WM_PAINT:
        {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Signup window: minimal validation and add new account
LRESULT CALLBACK SignupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Emri:", WS_CHILD | WS_VISIBLE, 20, 20, 100, 20,
                      hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Celulari:", WS_CHILD | WS_VISIBLE, 20, 60, 100, 20,
                      hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Balanca:", WS_CHILD | WS_VISIBLE, 20, 100, 100, 20,
                      hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Username:", WS_CHILD | WS_VISIBLE, 20, 140, 100, 20,
                      hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Fjalekalimi:", WS_CHILD | WS_VISIBLE, 20, 180, 100, 20,
                      hWnd, nullptr, hInst, nullptr);

        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      130, 18, 240, 24, hWnd, (HMENU)ID_SIGN_NAME, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      130, 58, 240, 24, hWnd, (HMENU)ID_SIGN_PHONE, hInst, nullptr);
        CreateWindowW(L"EDIT", L"0.00", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      130, 98, 240, 24, hWnd, (HMENU)ID_SIGN_BAL, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      130, 138, 240, 24, hWnd, (HMENU)ID_SIGN_USERNAME, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
                      130, 178, 240, 24, hWnd, (HMENU)ID_SIGN_PASSWORD, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"OK", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                      130, 220, 100, 28, hWnd, (HMENU)ID_SIGN_OK, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Anullo", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      270, 220, 100, 28, hWnd, (HMENU)ID_SIGN_CANCEL, hInst, nullptr);
        SetFocus(GetDlgItem(hWnd, ID_SIGN_NAME));
        break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == ID_SIGN_OK)
        {
            wchar_t name[256] = {}, phone[256] = {}, bal[256] = {}, user[256] = {}, pass[256] = {};
            GetWindowTextW(GetDlgItem(hWnd, ID_SIGN_NAME), name, _countof(name));
            GetWindowTextW(GetDlgItem(hWnd, ID_SIGN_PHONE), phone, _countof(phone));
            GetWindowTextW(GetDlgItem(hWnd, ID_SIGN_BAL), bal, _countof(bal));
            GetWindowTextW(GetDlgItem(hWnd, ID_SIGN_USERNAME), user, _countof(user));
            GetWindowTextW(GetDlgItem(hWnd, ID_SIGN_PASSWORD), pass, _countof(pass));

            std::wstring wname(name), wphone(phone), wbal(bal), wuser(user), wpass(pass);
            if (wname.empty() || wphone.empty() || wuser.empty() || wpass.empty())
            {
                MessageBoxW(hWnd, L"Te lutem ploteso te gjitha fushat.", L"Validation",
                            MB_OK | MB_ICONWARNING);
                break;
            }
            wchar_t* endp = nullptr;
            double balance = wcstod(wbal.c_str(), &endp);
            if (endp == wbal.c_str() || balance < 0.0)
            {
                MessageBoxW(hWnd, L"Vendos nje balance jo-negative.", L"Validation",
                            MB_OK | MB_ICONWARNING);
                break;
            }
            if (FindAccountIndexByUsername(wuser) != -1)
            {
                MessageBoxW(hWnd, L"Username eshte i zene. Zgjidh nje tjeter.", L"Validation",
                            MB_OK | MB_ICONWARNING);
                break;
            }
            // store record
            std::ostringstream oss;
            oss << ToUtf8(wuser) << '|' << ToUtf8(wpass) << '|' << ToUtf8(wname) << '|' << ToUtf8(wphone)
                << '|' << balance;
            accounts.push_back(oss.str());
            SaveAccounts();
            wchar_t msg[512];
            swprintf_s(msg, L"Llogaria e krijuar per: %s (username: %s)", wname.c_str(), wuser.c_str());
            MessageBoxW(hWnd, msg, L"Rregjistrohu", MB_OK | MB_ICONINFORMATION);
            DestroyWindow(hWnd);
        }
        else if (id == ID_SIGN_CANCEL) DestroyWindow(hWnd);
        break;
    }

    case WM_DESTROY: break;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Dashboard: view balance, add, withdraw, transfer
LRESULT CALLBACK DashboardWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        wchar_t* param = cs ? (wchar_t*)cs->lpCreateParams : nullptr;
        DashState* st = (DashState*)LocalAlloc(LPTR, sizeof(DashState));
        if (!st) {
            MessageBoxW(hWnd, L"Memory allocation failed.", L"Error", MB_OK | MB_ICONERROR);
            DestroyWindow(hWnd);
            return -1;
        }
        if (param)
        {
            st->user = std::wstring(param);
            free(param); // we own the copy
            int idx = FindAccountIndexByUsername(st->user);
            if (idx >= 0)
            {
                auto parts = SplitRecord(accounts[idx]);
                if (parts.size() >= 5) st->balance = std::stod(parts[4]);
            }
        }
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)st);

        wchar_t buf[128]; swprintf_s(buf, L"Balance: $%.2f", st->balance);
        CreateWindowW(L"STATIC", buf, WS_CHILD | WS_VISIBLE, 20, 20, 380, 24,
                      hWnd, (HMENU)ID_DASH_BAL_LABEL, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Transfero tek (client name):", WS_CHILD | WS_VISIBLE,
                      20, 60, 160, 20, hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      190, 58, 210, 24, hWnd, (HMENU)ID_DASH_TRANSFER_TO, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Shuma:", WS_CHILD | WS_VISIBLE, 20, 100, 80, 20,
                      hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"EDIT", L"0.00", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      110, 98, 160, 24, hWnd, (HMENU)ID_DASH_TRANSFER_AMT, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Transfero", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                      290, 96, 110, 28, hWnd, (HMENU)ID_DASH_TRANSFER_BTN, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Shto fonde", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      20, 150, 120, 32, hWnd, (HMENU)ID_DASH_ADD_BTN, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Terhiq", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                      160, 150, 120, 32, hWnd, (HMENU)ID_DASH_WITHDRAW_BTN, hInst, nullptr);
        SetFocus(GetDlgItem(hWnd, ID_DASH_TRANSFER_TO));
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        DashState* st = (DashState*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        if (!st) return 0;
        if (id != ID_DASH_ADD_BTN && id != ID_DASH_WITHDRAW_BTN && id != ID_DASH_TRANSFER_BTN) break;

        wchar_t amtStr[64] = {};
        GetWindowTextW(GetDlgItem(hWnd, ID_DASH_TRANSFER_AMT), amtStr, _countof(amtStr));
        double amt = wcstod(amtStr, nullptr);
        if (amt <= 0.0)
        {
            MessageBoxW(hWnd, L"Vendos nje vlere pozitive.", L"Shuma e gabuar", MB_OK | MB_ICONWARNING);
            break;
        }

        if (id == ID_DASH_ADD_BTN)
        {
            st->balance += amt;
            if (!st->user.empty()) UpdateAccountBalance(st->user, st->balance);
            wchar_t m[256]; swprintf_s(m, L"U shtuan $%.2f ne llogarine tuaj.", amt);
            MessageBoxW(hWnd, m, L"Shto fonde", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == ID_DASH_WITHDRAW_BTN)
        {
            if (st->balance < amt) { MessageBoxW(hWnd, L"Mungese fondesh.", L"Terheqja deshtoi!", MB_OK | MB_ICONERROR); break; }
            st->balance -= amt;
            if (!st->user.empty()) UpdateAccountBalance(st->user, st->balance);
            wchar_t m[256]; swprintf_s(m, L"U terhoqen $%.2f nga llogaria juaj.", amt);
            MessageBoxW(hWnd, m, L"Terhiq", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == ID_DASH_TRANSFER_BTN)
        {
            wchar_t target[256] = {};
            GetWindowTextW(GetDlgItem(hWnd, ID_DASH_TRANSFER_TO), target, _countof(target));
            if (wcslen(target) == 0) { MessageBoxW(hWnd, L"Vendos emrin e klientit per transferte.", L"Invalid Target", MB_OK | MB_ICONWARNING); break; }
            if (st->balance < amt) { MessageBoxW(hWnd, L"Fonde te pamjaftueshme.", L"Transfer Failed", MB_OK | MB_ICONERROR); break; }

            std::wstring targetUser(target);
            int dest = FindAccountIndexByUsername(targetUser);
            if (dest < 0) { MessageBoxW(hWnd, L"Username nuk u gjet.", L"Transferta deshtoi!", MB_OK | MB_ICONERROR); break; }

            // perform transfer
            st->balance -= amt;
            // update dest
            auto destParts = SplitRecord(accounts[dest]);
            double db = (destParts.size()>=5) ? std::stod(destParts[4]) : 0.0;
            db += amt;
            destParts.resize(5);
            destParts[4] = std::to_string(db);
            std::ostringstream oss; for (size_t i=0;i<destParts.size();++i){ if (i) oss<<'|'; oss<<destParts[i]; }
            accounts[dest] = oss.str();
            // update source
            if (!st->user.empty()) UpdateAccountBalance(st->user, st->balance);
            SaveAccounts();
            wchar_t m[512]; swprintf_s(m, L"Transferoi $%.2f to %s.", amt, target);
            MessageBoxW(hWnd, m, L"Transfero", MB_OK | MB_ICONINFORMATION);
        }

        // refresh label
        wchar_t buf[128]; swprintf_s(buf, L"Balance: $%.2f", st->balance);
        SetWindowTextW(GetDlgItem(hWnd, ID_DASH_BAL_LABEL), buf);
        break;
    }

    case WM_DESTROY:
        {
            DashState* st = (DashState*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            if (st) LocalFree(st);
        }
        break;

    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK AdminWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hListBox = nullptr;

    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Registered Accounts:",
            WS_CHILD | WS_VISIBLE,
            20, 20, 460, 20,
            hWnd, nullptr, hInst, nullptr);

        hListBox = CreateWindowW(L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            20, 50, 460, 260,
            hWnd, (HMENU)ID_ADMIN_LISTBOX, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"Fshi perdoruesin e zgjedhur",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 320, 200, 32,
            hWnd, (HMENU)ID_ADMIN_DELETE_BTN, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"Rifresko listen",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            240, 320, 150, 32,
            hWnd, (HMENU)ID_ADMIN_REFRESH_BTN, hInst, nullptr);

        PopulateAdminList(hListBox);
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        if (id == ID_ADMIN_REFRESH_BTN)
        {
            LoadAccounts();
            PopulateAdminList(hListBox);
            MessageBoxW(hWnd, L"Lista u rifreskua.", L"Admin", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == ID_ADMIN_DELETE_BTN)
        {
            int sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR)
            {
                MessageBoxW(hWnd, L"Te lutem zgjidh nje perdorues per ta fshire.",
                    L"No Selection", MB_OK | MB_ICONWARNING);
                break;
            }

            if (sel >= 0 && sel < (int)accounts.size())
            {
                auto parts = SplitRecord(accounts[sel]);
                if (parts.size() >= 1)
                {
                    std::wstring username = ToW(parts[0]);
                    std::wstring confirmMsg = L"Je i sigurt qe do te fshish perdoruesin: " + username + L"?";

                    int result = MessageBoxW(hWnd, confirmMsg.c_str(),
                        L"Konfirmo fshirjen",
                        MB_YESNO | MB_ICONQUESTION);

                    if (result == IDYES)
                    {
                        DeleteAccount(username);
                        PopulateAdminList(hListBox);

                        std::wstring msg = L"Perdoruesi " + username + L" u fshi me sukses.";
                        MessageBoxW(hWnd, msg.c_str(), L"Deleted", MB_OK | MB_ICONINFORMATION);
                    }
                }
            }
        }
        break;
    }

    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    if (message == WM_INITDIALOG) return (INT_PTR)TRUE;
    if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
    {
        EndDialog(hDlg, LOWORD(wParam)); return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}
