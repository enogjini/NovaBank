#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#define MAX_LOADSTRING 100

// ID-të e kontrolleve
#define ID_EDIT_PERDORUESI 101
#define ID_EDIT_FJALKALIMI 102
#define ID_BTN_KLIENT 201
#define ID_BTN_ADMIN 202
#define ID_BTN_REGJISTROHU 203

// ID-të e dialogut të regjistrimit
#define ID_REG_EMRI 301
#define ID_REG_TELEFONI 302
#define ID_REG_BILANCI 303
#define ID_REG_OK 304
#define ID_REG_ANULO 305
#define ID_REG_PERDORUESI 306
#define ID_REG_FJALKALIMI 307

// ID-të e panelit të klientit
#define ID_PANEL_BILANCI_LABEL 401
#define ID_PANEL_TRANSFERO_TEK 402
#define ID_PANEL_TRANSFERO_SHUMA 403
#define ID_PANEL_TRANSFERO_BTN 404
#define ID_PANEL_SHTO_BTN 405
#define ID_PANEL_TERHIQ_BTN 406

// ID-të e panelit të adminit
#define ID_ADMIN_LISTA 501
#define ID_ADMIN_FSHI_BTN 502
#define ID_ADMIN_RIFRESKO_BTN 503

// Variablat globale
HINSTANCE hInst;                                      // instanca aktuale
WCHAR szTitulli[MAX_LOADSTRING] = L"NovaBank";       // titulli i aplikacionit
WCHAR szKlasaDritares[MAX_LOADSTRING] = L"NovaBankKlasaKryesore"; // klasa e dritares

static HWND editPerdoruesi = nullptr;                 // fusha e përdoruesit
static HWND editFjalkalimi = nullptr;                 // fusha e fjalëkalimit

static std::vector<std::wstring> dummy; // (për debug)
static const char* EMRI_SKEDARIT_LLOGARI = "accounts.txt";

// Deklarime paraprake
ATOM RegjistroKlasen(HINSTANCE hInstance);
BOOL FilloDritaren(HINSTANCE, int);
LRESULT CALLBACK ProceduraDritares(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RrethAplikacionit(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ProceduraRegjistrimit(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ProceduraPanelit(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ProceduraAdminit(HWND, UINT, WPARAM, LPARAM);

// -------------------------
// Ndihmëse: konvertime dhe parsim
// -------------------------
static std::string NeUtf8(const std::wstring& s)
{
    return std::string(s.begin(), s.end()); // thjesht për ASCII/UTF-8 të thjeshtë
}

static std::wstring NeWstring(const std::string& s)
{
    return std::wstring(s.begin(), s.end());
}


// Klasa Klient

class Klient {
private:
    std::wstring perdoruesi;
    std::wstring fjalkalimi;
    std::wstring emri;
    std::wstring telefoni;
    double bilanci;

public:
    Klient() : bilanci(0.0) {}
    Klient(const std::wstring& p, const std::wstring& f, const std::wstring& e, const std::wstring& t, double b)
        : perdoruesi(p), fjalkalimi(f), emri(e), telefoni(t), bilanci(b) {
    }

    std::wstring getPerdoruesi() const { return perdoruesi; }
    std::wstring getFjalkalimi() const { return fjalkalimi; }
    std::wstring getEmri() const { return emri; }
    std::wstring getTelefoni() const { return telefoni; }
    double getBilanci() const { return bilanci; }

    void setPerdoruesi(const std::wstring& p) { perdoruesi = p; }
    void setFjalkalimi(const std::wstring& f) { fjalkalimi = f; }
    void setEmri(const std::wstring& e) { emri = e; }
    void setTelefoni(const std::wstring& t) { telefoni = t; }
    void setBilanci(double b) { bilanci = b; }

    void shtoFonde(double shuma) { if (shuma > 0) bilanci += shuma; }
    bool terhiq(double shuma) { if (shuma > 0 && bilanci >= shuma) { bilanci -= shuma; return true; } return false; }

    bool transfero(Klient& destinacioni, double shuma) {
        if (shuma > 0 && bilanci >= shuma) { bilanci -= shuma; destinacioni.shtoFonde(shuma); return true; }
        return false;
    }

    std::string serializo() const {
        std::ostringstream oss;
        oss << NeUtf8(perdoruesi) << "|" << NeUtf8(fjalkalimi) << "|" << NeUtf8(emri) << "|" << NeUtf8(telefoni) << "|" << bilanci;
        return oss.str();
    }

    static Klient NgaRekordi(const std::string& rek) {
        Klient k;
        std::istringstream ss(rek);
        std::string tok;
        std::vector<std::string> p;
        while (std::getline(ss, tok, '|')) p.push_back(tok);
        if (p.size() >= 1) k.perdoruesi = NeWstring(p[0]);
        if (p.size() >= 2) k.fjalkalimi = NeWstring(p[1]);
        if (p.size() >= 3) k.emri = NeWstring(p[2]);
        if (p.size() >= 4) k.telefoni = NeWstring(p[3]);
        if (p.size() >= 5) {
            try { k.bilanci = std::stod(p[4]); }
            catch (...) { k.bilanci = 0.0; }
        }
        return k;
    }
};


// Strukturat dhe funksionet për ruajtje në vector<Klient>

static std::vector<Klient> klientet;

static void NgarkoLlogarite()
{
    klientet.clear();
    std::ifstream hyrje(EMRI_SKEDARIT_LLOGARI);
    if (!hyrje) return;
    std::string rresht;
    while (std::getline(hyrje, rresht)) {
        if (rresht.empty()) continue;
        Klient k = Klient::NgaRekordi(rresht);
        klientet.push_back(k);
    }
}

static void RuajLlogarite()
{
    std::ofstream dalje(EMRI_SKEDARIT_LLOGARI, std::ios::trunc);
    if (!dalje) return;
    for (const auto& k : klientet) {
        dalje << k.serializo() << '\n';
    }
}

static int GjejIndeksinSipasEmrit(const std::wstring& perdoruesi)
{
    for (size_t i = 0; i < klientet.size(); ++i) {
        if (klientet[i].getPerdoruesi() == perdoruesi) return (int)i;
    }
    return -1;
}

static void PerditesoBilancin(const std::wstring& perdoruesi, double bil)
{
    int idx = GjejIndeksinSipasEmrit(perdoruesi);
    if (idx < 0) return;
    klientet[idx].setBilanci(bil);
    RuajLlogarite();
}

static void FshiLlogarine(const std::wstring& perdoruesi)
{
    int idx = GjejIndeksinSipasEmrit(perdoruesi);
    if (idx >= 0) {
        klientet.erase(klientet.begin() + idx);
        RuajLlogarite();
    }
}

static void MbushListenAdmin(HWND hLista)
{
    SendMessage(hLista, LB_RESETCONTENT, 0, 0);
    for (const auto& k : klientet) {
        std::wostringstream wos;
        wos << L"Përdoruesi: " << k.getPerdoruesi()
            << L" | Emri: " << k.getEmri()
            << L" | Bilanci: $" << k.getBilanci();
        std::wstring hyrja = wos.str();
        SendMessageW(hLista, LB_ADDSTRING, 0, (LPARAM)hyrja.c_str());
    }
}


// Klasa Admin 

class Admin {
private:
    std::wstring perdoruesi;
    std::wstring fjalkalimi;

public:
    Admin(std::wstring p = L"admin", std::wstring f = L"admin")
        : perdoruesi(p), fjalkalimi(f) {
    }

    bool autentiko(const std::wstring& p, const std::wstring& f) const {
        return (p == perdoruesi && f == fjalkalimi);
    }

    void shfaqLlogarite(HWND hLista) const {
        MbushListenAdmin(hLista);
    }

    void fshiLlogari(const std::wstring& perd) const {
        FshiLlogarine(perd);
    }
};


// Implementimi i regjistrimit të klasave të dritareve

ATOM RegjistroKlasen(HINSTANCE hInstance)
{
    WNDCLASSEXW w = {};
    w.cbSize = sizeof(w);
    w.style = CS_HREDRAW | CS_VREDRAW;
    w.lpfnWndProc = ProceduraDritares;
    w.hInstance = hInstance;
    w.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    w.hCursor = LoadCursor(nullptr, IDC_ARROW);
    w.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    w.lpszMenuName = nullptr;
    w.lpszClassName = szKlasaDritares;
    w.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    ATOM a = RegisterClassExW(&w);

    // Klasa e regjistrimit
    WNDCLASSEXW s = {};
    s.cbSize = sizeof(s);
    s.style = CS_HREDRAW | CS_VREDRAW;
    s.lpfnWndProc = ProceduraRegjistrimit;
    s.hInstance = hInstance;
    s.hCursor = LoadCursor(nullptr, IDC_ARROW);
    s.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    s.lpszClassName = L"KlasaDritaresRegjistrimit";
    RegisterClassExW(&s);

    // Klasa e panelit
    WNDCLASSEXW d = {};
    d.cbSize = sizeof(d);
    d.style = CS_HREDRAW | CS_VREDRAW;
    d.lpfnWndProc = ProceduraPanelit;
    d.hInstance = hInstance;
    d.hCursor = LoadCursor(nullptr, IDC_ARROW);
    d.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    d.lpszClassName = L"KlasaDritaresPanelit";
    RegisterClassExW(&d);

    // Klasa e adminit
    WNDCLASSEXW admin = {};
    admin.cbSize = sizeof(admin);
    admin.style = CS_HREDRAW | CS_VREDRAW;
    admin.lpfnWndProc = ProceduraAdminit;
    admin.hInstance = hInstance;
    admin.hCursor = LoadCursor(nullptr, IDC_ARROW);
    admin.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    admin.lpszClassName = L"KlasaDritaresAdminit";
    RegisterClassExW(&admin);

    return a;
}


// Fillimi i dritares kryesore

BOOL FilloDritaren(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND h = CreateWindowW(szKlasaDritares, szTitulli, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 400, 260, nullptr, nullptr, hInstance, nullptr);
    if (!h) return FALSE;
    ShowWindow(h, nCmdShow);
    UpdateWindow(h);
    return TRUE;
}


// Procedura kryesore e dritares

LRESULT CALLBACK ProceduraDritares(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        // Etiketat
        CreateWindowW(L"STATIC", L"Përdoruesi:", WS_CHILD | WS_VISIBLE, 20, 20, 80, 20,
            hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Fjalëkalimi:", WS_CHILD | WS_VISIBLE, 20, 60, 80, 20,
            hWnd, nullptr, hInst, nullptr);
        // Fushat e tekstit
        editPerdoruesi = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            110, 18, 250, 24, hWnd, (HMENU)ID_EDIT_PERDORUESI, hInst, nullptr);
        editFjalkalimi = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
            110, 58, 250, 24, hWnd, (HMENU)ID_EDIT_FJALKALIMI, hInst, nullptr);
        // Butonat: hyrja si klient, hyrja si admin dhe regjistrimi
        CreateWindowW(L"BUTTON", L"Kyçu si Klient",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 110, 155, 30, hWnd, (HMENU)ID_BTN_KLIENT, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Kyçu si Admin",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            195, 110, 155, 30, hWnd, (HMENU)ID_BTN_ADMIN, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Nuk keni llogari? Krijoni një",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 150, 200, 30, hWnd, (HMENU)ID_BTN_REGJISTROHU, hInst, nullptr);
        break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int ev = HIWORD(wParam);
        if (ev == BN_CLICKED)
        {
            if (id == ID_BTN_REGJISTROHU)
            {
                RECT r; GetWindowRect(hWnd, &r);
                int w = 420, h = 320;
                int x = r.left + (r.right - r.left - w) / 2;
                int y = r.top + (r.bottom - r.top - h) / 2;
                CreateWindowW(L"KlasaDritaresRegjistrimit", L"Krijo Llogari",
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                    x, y, w, h, hWnd, nullptr, hInst, nullptr);
                return 0;
            }

            if (id == ID_BTN_KLIENT)
            {
                // lexo fushat
                wchar_t p[256] = {}, f[256] = {};
                GetWindowTextW(editPerdoruesi, p, _countof(p));
                GetWindowTextW(editFjalkalimi, f, _countof(f));
                if (wcslen(p) == 0 || wcslen(f) == 0)
                {
                    MessageBoxW(hWnd, L"Shkruani përdoruesin dhe fjalëkalimin për t'u kyçur.",
                        L"Kyçje", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                std::wstring perdoruesi(p), fjalkalimi(f);
                int idx = GjejIndeksinSipasEmrit(perdoruesi);
                if (idx < 0)
                {
                    MessageBoxW(hWnd, L"Përdoruesi ose fjalëkalimi i gabuar.", L"Kyçja Dështoi!",
                        MB_OK | MB_ICONERROR);
                    return 0;
                }
                std::wstring iRuajtur = klientet[idx].getFjalkalimi();
                if (iRuajtur != fjalkalimi)
                {
                    MessageBoxW(hWnd, L"Përdoruesi ose fjalëkalimi i gabuar.", L"Kyçja Dështoi!",
                        MB_OK | MB_ICONERROR);
                    return 0;
                }
                // hap panelin dhe kalo përdoruesin
                RECT r; GetWindowRect(hWnd, &r);
                int w = 420, h = 300;
                int x = r.left + (r.right - r.left - w) / 2;
                int y = r.top + (r.bottom - r.top - h) / 2;
                // kalojmë emrin si param
                wchar_t* emri = _wcsdup(perdoruesi.c_str());
                HWND hd = CreateWindowW(L"KlasaDritaresPanelit", L"Paneli i Klientit",
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                    x, y, w, h, hWnd, nullptr, hInst, (LPVOID)emri);
                if (!hd) {
                    MessageBoxW(hWnd, L"Dështoi krijimi i panelit.", L"Gabim",
                        MB_OK | MB_ICONERROR); free(emri);
                }
                return 0;
            }

            if (id == ID_BTN_ADMIN)
            {
                // lexo fushat
                wchar_t p[256] = {}, f[256] = {};
                GetWindowTextW(editPerdoruesi, p, _countof(p));
                GetWindowTextW(editFjalkalimi, f, _countof(f));
                if (wcslen(p) == 0 || wcslen(f) == 0)
                {
                    MessageBoxW(hWnd, L"Vendos kredencialet e administratorit.",
                        L"Kyçje", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                std::wstring perdoruesi(p), fjalkalimi(f);
                // Kontrollo kredencialet e koduara të adminit
                Admin admin;
                if (!admin.autentiko(perdoruesi, fjalkalimi))
                {
                    MessageBoxW(hWnd, L"Përdoruesi ose fjalëkalimi i administratorit i pavlefshëm.",
                        L"Kyçja Dështoi", MB_OK | MB_ICONERROR);
                    return 0;
                }
                // Kyçja si admin u krye me sukses, shfaq përmbledhjen e llogarive
                RECT r; GetWindowRect(hWnd, &r);
                int w = 500, h = 400;
                int x = r.left + (r.right - r.left - w) / 2;
                int y = r.top + (r.bottom - r.top - h) / 2;
                CreateWindowW(L"KlasaDritaresAdminit", L"Paneli i Administratorit",
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                    x, y, w, h, hWnd, nullptr, hInst, nullptr);
                return 0;
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

// Dritarja e regjistrimit: validim minimal dhe shtim llogarie të re
LRESULT CALLBACK ProceduraRegjistrimit(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Emri:", WS_CHILD | WS_VISIBLE, 20, 20, 100, 20,
            hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Celulari:", WS_CHILD | WS_VISIBLE, 20, 60, 100, 20,
            hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Bilanci:", WS_CHILD | WS_VISIBLE, 20, 100, 100, 20,
            hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Përdoruesi:", WS_CHILD | WS_VISIBLE, 20, 140, 100, 20,
            hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Fjalëkalimi:", WS_CHILD | WS_VISIBLE, 20, 180, 100, 20,
            hWnd, nullptr, hInst, nullptr);

        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            130, 18, 240, 24, hWnd, (HMENU)ID_REG_EMRI, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            130, 58, 240, 24, hWnd, (HMENU)ID_REG_TELEFONI, hInst, nullptr);
        CreateWindowW(L"EDIT", L"0.00", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            130, 98, 240, 24, hWnd, (HMENU)ID_REG_BILANCI, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            130, 138, 240, 24, hWnd, (HMENU)ID_REG_PERDORUESI, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
            130, 178, 240, 24, hWnd, (HMENU)ID_REG_FJALKALIMI, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"OK", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            130, 220, 100, 28, hWnd, (HMENU)ID_REG_OK, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Anulo", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 220, 100, 28, hWnd, (HMENU)ID_REG_ANULO, hInst, nullptr);
        SetFocus(GetDlgItem(hWnd, ID_REG_EMRI));
        break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == ID_REG_OK)
        {
            wchar_t emri[256] = {}, telefoni[256] = {}, bilanci[256] = {},
                perdoruesi[256] = {}, fjalkalimi[256] = {};
            GetWindowTextW(GetDlgItem(hWnd, ID_REG_EMRI), emri, _countof(emri));
            GetWindowTextW(GetDlgItem(hWnd, ID_REG_TELEFONI), telefoni, _countof(telefoni));
            GetWindowTextW(GetDlgItem(hWnd, ID_REG_BILANCI), bilanci, _countof(bilanci));
            GetWindowTextW(GetDlgItem(hWnd, ID_REG_PERDORUESI), perdoruesi, _countof(perdoruesi));
            GetWindowTextW(GetDlgItem(hWnd, ID_REG_FJALKALIMI), fjalkalimi, _countof(fjalkalimi));

            std::wstring wEmri(emri), wTelefoni(telefoni), wBilanci(bilanci),
                wPerdoruesi(perdoruesi), wFjalkalimi(fjalkalimi);
            if (wEmri.empty() || wTelefoni.empty() || wPerdoruesi.empty() || wFjalkalimi.empty())
            {
                MessageBoxW(hWnd, L"Të lutem plotëso të gjitha fushat.", L"Validim",
                    MB_OK | MB_ICONWARNING);
                break;
            }
            wchar_t* fund = nullptr;
            double bil = wcstod(wBilanci.c_str(), &fund);
            if (fund == wBilanci.c_str() || bil < 0.0)
            {
                MessageBoxW(hWnd, L"Vendos një bilanc jo-negativ.", L"Validim",
                    MB_OK | MB_ICONWARNING);
                break;
            }
            if (GjejIndeksinSipasEmrit(wPerdoruesi) != -1)
            {
                MessageBoxW(hWnd, L"Përdoruesi është i zënë. Zgjidh një tjetër.", L"Validim",
                    MB_OK | MB_ICONWARNING);
                break;
            }
            // ruaj rekordin si objekt Klient
            Klient k(wPerdoruesi, wFjalkalimi, wEmri, wTelefoni, bil);
            klientet.push_back(k);
            RuajLlogarite();
            wchar_t mesazhi[512];
            swprintf_s(mesazhi, L"Llogaria u krijua për: %s (përdoruesi: %s)",
                wEmri.c_str(), wPerdoruesi.c_str());
            MessageBoxW(hWnd, mesazhi, L"Regjistrimi", MB_OK | MB_ICONINFORMATION);
            DestroyWindow(hWnd);
        }
        else if (id == ID_REG_ANULO) DestroyWindow(hWnd);
        break;
    }

    case WM_DESTROY: break;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Paneli: shiko bilancin, shto, tërhiq, transfero
LRESULT CALLBACK ProceduraPanelit(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        wchar_t* param = cs ? (wchar_t*)cs->lpCreateParams : nullptr;
        Klient* klient = new Klient();
        if (!klient) {
            MessageBoxW(hWnd, L"Alokimi i memories deshtoi.", L"Gabim", MB_OK | MB_ICONERROR);
            DestroyWindow(hWnd);
            return -1;
        }
        if (param)
        {
            std::wstring perd = std::wstring(param);
            free(param); // liroj kopjen
            int idx = GjejIndeksinSipasEmrit(perd);
            if (idx >= 0) {
                *klient = klientet[idx];
            }
            else {
                klient->setPerdoruesi(perd);
            }
        }
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)klient);

        wchar_t buf[128]; swprintf_s(buf, L"Bilanci: $%.2f", klient->getBilanci());
        CreateWindowW(L"STATIC", buf, WS_CHILD | WS_VISIBLE, 20, 20, 380, 24,
            hWnd, (HMENU)ID_PANEL_BILANCI_LABEL, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Transfero tek (emri i klientit):", WS_CHILD | WS_VISIBLE,
            20, 60, 160, 30, hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            190, 58, 210, 24, hWnd, (HMENU)ID_PANEL_TRANSFERO_TEK, hInst, nullptr);
        CreateWindowW(L"STATIC", L"Shuma:", WS_CHILD | WS_VISIBLE, 20, 100, 80, 20,
            hWnd, nullptr, hInst, nullptr);
        CreateWindowW(L"EDIT", L"0.00", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            110, 98, 160, 24, hWnd, (HMENU)ID_PANEL_TRANSFERO_SHUMA, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Transfero", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            290, 96, 110, 28, hWnd, (HMENU)ID_PANEL_TRANSFERO_BTN, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Shto Fonde", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 150, 120, 32, hWnd, (HMENU)ID_PANEL_SHTO_BTN, hInst, nullptr);
        CreateWindowW(L"BUTTON", L"Tërhiq", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            160, 150, 120, 32, hWnd, (HMENU)ID_PANEL_TERHIQ_BTN, hInst, nullptr);
        SetFocus(GetDlgItem(hWnd, ID_PANEL_TRANSFERO_TEK));
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        Klient* klient = (Klient*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        if (!klient) return 0;
        if (id != ID_PANEL_SHTO_BTN && id != ID_PANEL_TERHIQ_BTN && id != ID_PANEL_TRANSFERO_BTN) break;

        wchar_t shumaStr[64] = {};
        GetWindowTextW(GetDlgItem(hWnd, ID_PANEL_TRANSFERO_SHUMA), shumaStr, _countof(shumaStr));
        double shuma = wcstod(shumaStr, nullptr);
        if (shuma <= 0.0)
        {
            MessageBoxW(hWnd, L"Vendos një vlerë pozitive.", L"Shuma e Gabuar", MB_OK | MB_ICONWARNING);
            break;
        }

        if (id == ID_PANEL_SHTO_BTN)
        {
            klient->shtoFonde(shuma);
            if (!klient->getPerdoruesi().empty()) PerditesoBilancin(klient->getPerdoruesi(), klient->getBilanci());
            wchar_t m[256]; swprintf_s(m, L"U shtuan $%.2f në llogarinë tuaj.", shuma);
            MessageBoxW(hWnd, m, L"Shto Fonde", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == ID_PANEL_TERHIQ_BTN)
        {
            if (!klient->terhiq(shuma)) {
                MessageBoxW(hWnd, L"Mungesë fondesh.", L"Tërheqja Dështoi!", MB_OK | MB_ICONERROR);
                break;
            }
            if (!klient->getPerdoruesi().empty()) PerditesoBilancin(klient->getPerdoruesi(), klient->getBilanci());
            wchar_t m[256]; swprintf_s(m, L"U tërhoqën $%.2f nga llogaria juaj.", shuma);
            MessageBoxW(hWnd, m, L"Tërhiq", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == ID_PANEL_TRANSFERO_BTN)
        {
            wchar_t caku[256] = {};
            GetWindowTextW(GetDlgItem(hWnd, ID_PANEL_TRANSFERO_TEK), caku, _countof(caku));
            if (wcslen(caku) == 0) {
                MessageBoxW(hWnd, L"Vendos emrin e klientit për transfertë.", L"Cak i Pavlefshëm",
                    MB_OK | MB_ICONWARNING);
                break;
            }
            if (klient->getBilanci() < shuma) {
                MessageBoxW(hWnd, L"Fonde të pamjaftueshme.", L"Transferta Dështoi",
                    MB_OK | MB_ICONERROR);
                break;
            }

            std::wstring perdCak(caku);
            int dest = GjejIndeksinSipasEmrit(perdCak);
            if (dest < 0) {
                MessageBoxW(hWnd, L"Përdoruesi nuk u gjet.", L"Transferta Dështoi!",
                    MB_OK | MB_ICONERROR);
                break;
            }

            // merr destinacionin nga vector, përditëso dhe ruaj
            Klient& destKl = klientet[dest];
            if (!klient->transfero(destKl, shuma)) {
                MessageBoxW(hWnd, L"Transferta dështoi.", L"Gabim", MB_OK | MB_ICONERROR);
                break;
            }
            // përditëso burimin në vector nëse ekziston
            if (!klient->getPerdoruesi().empty()) PerditesoBilancin(klient->getPerdoruesi(), klient->getBilanci());
            RuajLlogarite();
            wchar_t m[512]; swprintf_s(m, L"Transferoi $%.2f tek %s.", shuma, caku);
            MessageBoxW(hWnd, m, L"Transfero", MB_OK | MB_ICONINFORMATION);
        }

        // rifresko etiketën
        wchar_t buf[128]; swprintf_s(buf, L"Bilanci: $%.2f", klient->getBilanci());
        SetWindowTextW(GetDlgItem(hWnd, ID_PANEL_BILANCI_LABEL), buf);
        break;
    }

    case WM_DESTROY:
    {
        Klient* klient = (Klient*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        if (klient) delete klient;
    }
    break;

    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Paneli i adminit
LRESULT CALLBACK ProceduraAdminit(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hLista = nullptr;
    static Admin admin;

    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Llogaritë e Regjistruara:",
            WS_CHILD | WS_VISIBLE,
            20, 20, 460, 20,
            hWnd, nullptr, hInst, nullptr);

        hLista = CreateWindowW(L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            20, 50, 460, 260,
            hWnd, (HMENU)ID_ADMIN_LISTA, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"Fshi përdoruesin e zgjedhur",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 320, 200, 32,
            hWnd, (HMENU)ID_ADMIN_FSHI_BTN, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"Rifresko listën",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            240, 320, 150, 32,
            hWnd, (HMENU)ID_ADMIN_RIFRESKO_BTN, hInst, nullptr);

        admin.shfaqLlogarite(hLista);
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        if (id == ID_ADMIN_RIFRESKO_BTN)
        {
            NgarkoLlogarite();
            admin.shfaqLlogarite(hLista);
            MessageBoxW(hWnd, L"Lista u rifreskua.", L"Admin", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == ID_ADMIN_FSHI_BTN)
        {
            int sel = (int)SendMessage(hLista, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR)
            {
                MessageBoxW(hWnd, L"Të lutem zgjidh një përdorues për ta fshirë.",
                    L"Asnjë Zgjedhje", MB_OK | MB_ICONWARNING);
                break;
            }

            if (sel >= 0 && sel < (int)klientet.size())
            {
                std::wstring perdoruesi = klientet[sel].getPerdoruesi();
                std::wstring mesazhiKonfirmim = L"Je i sigurt që do të fshish përdoruesin: " +
                    perdoruesi + L"?";

                int rezultati = MessageBoxW(hWnd, mesazhiKonfirmim.c_str(),
                    L"Konfirmo Fshirjen",
                    MB_YESNO | MB_ICONQUESTION);

                if (rezultati == IDYES)
                {
                    admin.fshiLlogari(perdoruesi);
                    admin.shfaqLlogarite(hLista);

                    std::wstring msg = L"Përdoruesi " + perdoruesi + L" u fshi me sukses.";
                    MessageBoxW(hWnd, msg.c_str(), L"Fshirë", MB_OK | MB_ICONINFORMATION);
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

// Dialog rreth aplikacionit (opsional)
INT_PTR CALLBACK RrethAplikacionit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    if (message == WM_INITDIALOG) return (INT_PTR)TRUE;
    if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
    {
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}


// Entry point

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    RegjistroKlasen(hInstance);
    NgarkoLlogarite(); // rikthe llogaritë e ruajtura

    if (!FilloDritaren(hInstance, nCmdShow)) return FALSE;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
