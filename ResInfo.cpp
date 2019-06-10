////////////////////////////////////////////////////////////////////////////////////////////////
// ResInfo.cpp - Copyright (c) 2003-2011 by Wolfgang Rolke.
//
// Licensed under the EUPL, Version 1.2 or - as soon they will be approved by
// the European Commission - subsequent versions of the EUPL (the "Licence");
// You may not use this work except in compliance with the Licence.
// You may obtain a copy of the Licence at:
//
// https://joinup.ec.europa.eu/software/page/eupl
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Licence is distributed on an "AS IS" basis,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the Licence for the specific language governing permissions and
// limitations under the Licence.
//
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef STRICT
#define STRICT 1
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <prsht.h>
#include <dbt.h>
#include <tlhelp32.h>
#include <winsock.h>

#include "resource.h"
#include "cputype.h"
#include "cpuinfo.h"
#include "reboot.h"
#include "resinfo.h"

#if !defined(_countof)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#define HISTORY_MAX 256
#define TIMER1_PERIOD 5000          // 5 sec
#define TIMER2_PERIOD 1500          // 1.5 sec
#define XSCALE_ROLLOVER_MIN 1140000 // 19 min
#define XSCALE_ROLLOVER_MAX 1200000 // 20 min
#define WORKING_PERIOD 172800000    // 48 h
#define SAVE_PERIOD 120 // 120 * TIMER1_PERIOD = 10 min

#define WM_APP_ADDICON   WM_APP  + 40
#define WM_APP_DELICON   WM_APP  + 41
#define WM_APP_REFRESH   WM_APP  + 42
#define WM_PSPCB_RELEASE WM_USER + 1092
#define TRAY_NOTIFYICON  WM_USER + 4242
#define ID_TRAY1 4242
#define ID_TRAY2 4243
#define ID_TRAY3 4244
#define ID_TIMER1 1
#define ID_TIMER2 2

BOOL    CALLBACK EnumWindowsProc1(HWND, DWORD);
BOOL    CALLBACK EnumWindowsProc2(HWND, DWORD);
BOOL    CALLBACK EnumWindowsProc3(HWND, DWORD);
int     CALLBACK PropSheetProc(HWND, UINT, LPARAM);
UINT    CALLBACK PropSheetPageProc(HWND, UINT, LPPROPSHEETPAGE);
LRESULT CALLBACK PropSheetDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc1(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc2(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc3(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc4(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc5(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc6(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc7(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK ResInfoDlgProc8(HWND, UINT, WPARAM, LPARAM);

BOOL   TrayMessage(HWND, DWORD, UINT, HICON hIcon = NULL);
HICON  CreateTrayIcon1(BYTE cPcMem = 255, BYTE cPcObj = 255, BYTE cPcMemDiv = 255);
HICON  CreateTrayIcon2(LPCTSTR, BYTE cPercent = BATTERY_PERCENTAGE_UNKNOWN, BYTE cBatFlag = BATTERY_FLAG_UNKNOWN);
HICON  CreateTrayIcon3(BYTE cPcMem = 255);
HICON  CreateTrayIcon4(BYTE cPercent = BATTERY_PERCENTAGE_UNKNOWN, BYTE cBatFlag = BATTERY_FLAG_UNKNOWN);
HICON  CreateCpuTrayIcon(BYTE cPercent = 0, BOOL bPaused = FALSE);
UINT   GetTrayIconID(BYTE cPercent = 0, BOOL bPaused = FALSE);
BYTE   GetCpuLoadFillHeight(BYTE cPercent = 0, BOOL bPaused = FALSE);
LONG   LoadSettings(void);
LONG   SaveSetting(LPCTSTR, DWORD);
LONG   SaveRuntimeData(DWORD, DWORD, __int64, __int64);
LONG   LoadRuntimeData(LPDWORD, LPDWORD, LPBOOL, __int64*, __int64*);
BOOL   EnumDBase(PFILE_LIST);
int    SearchDirectory(LPTSTR, PFILE_LIST);
DWORD  UpdateStorageMedia(HWND, DWORD, BOOL bForceRefresh = TRUE);
void   UpdateSystemInfo(HWND);
void   UpdateWndList(HWND, BOOL bInit = FALSE, int nOldTopIndex = 0, int nOldCurSel = 0);
void   UpdateTaskList(HWND, BOOL bInit = FALSE, int nOldTopIndex = 0, int nOldCurSel = 0);
void   ShowTaskWindow(HWND);
void   ShowModuleList(HWND);
BOOL   GetCPULoad(BYTE* pcPercent, UINT* puThreads = NULL);
BOOL   ResizeWindow(HWND hwndClient);

static DWORD MyGetWindowText(HWND, LPTSTR, int);
static BOOL Stretch16x16To32x32(HDC, int, int, DWORD);
static BOOL GetProcessorInformation(LPTSTR, LPTSTR, LPDWORD);
static HFONT CreateSystemFont(int nDpi, LONG lHeight, LONG lWeight = FW_NORMAL);
static void KilobyteToString(DWORD, LPTSTR);
static void ByteToString(DWORD, LPTSTR);
static BOOL SetTextWithEllipsis(HWND, LPCTSTR);
static BOOL ExecPegHelp(LPCTSTR);

const TCHAR g_szTitle[]       = TEXT("WR-Tools ResInfo");
const TCHAR g_szTitle2[]      = TEXT("WR-Tools ResInfo 1.58");
const TCHAR g_szVersion[]     = TEXT("Version 1.58.246");
const TCHAR g_szCopyright[]   = TEXT("Copyright © 2003-2011");
const TCHAR g_szAuthor[]      = TEXT("by Wolfgang Rolke");
const TCHAR g_szHomepage[]    = TEXT("www.wolfgang-rolke.de");
const TCHAR g_szWinCESite[]   = TEXT("http://www.wolfgang-rolke.de/wince/");
const TCHAR g_szMutex[]       = TEXT("WRToolsResInfo");
const TCHAR g_szResInfo[]     = TEXT("ResInfo");
const TCHAR g_szPSDlgClass[]  = TEXT("Dialog");
const TCHAR g_szPalmPC2[]     = TEXT("Palm PC2");
const TCHAR g_szPocketPC[]    = TEXT("PocketPC");
const TCHAR g_szSmartPhone[]  = TEXT("SmartPhone");
const TCHAR g_szCoreDll[]     = TEXT("coredll.dll");
const TCHAR g_szCommDlg[]     = TEXT("commdlg.dll");
const TCHAR g_szToolHelp[]    = TEXT("toolhelp.dll");
const TCHAR g_szAygShell[]    = TEXT("aygshell.dll");
const TCHAR g_szPegHelp[]     = TEXT("peghelp.exe");
const TCHAR g_szUseDefault[]  = TEXT("UseDefault");
const TCHAR g_szShellWnd[]    = TEXT("HideShellWnd");
const TCHAR g_szClassName[]   = TEXT("ShowClassName");
const TCHAR g_szClassic[]     = TEXT("ClassicView");
const TCHAR g_szAdjustTemp[]  = TEXT("AdjustTemp");
const TCHAR g_szForceGerUI[]  = TEXT("ForceGerUI");
const TCHAR g_szRunHidden[]   = TEXT("RunHidden");
const TCHAR g_szInterval[]    = TEXT("Interval");
const TCHAR g_szCpuPause[]    = TEXT("CpuPause");
const TCHAR g_szCpuDetails[]  = TEXT("CpuDetails");
const TCHAR g_szAutoReset[]   = TEXT("AutoReset");
const TCHAR g_szNoCpuIcon[]   = TEXT("NoCpuTrayIcon");
const TCHAR g_szNoBatIcon[]   = TEXT("NoBatTrayIcon");
const TCHAR g_szNoMemIcon[]   = TEXT("NoMemTrayIcon");
const TCHAR g_szStaticIco[]   = TEXT("StaticIcons");
const TCHAR g_szAltColor[]    = TEXT("AltColor");
const TCHAR g_szIconColor[]   = TEXT("IconColor");
const TCHAR g_szOperation[]   = TEXT("Operation");
const TCHAR g_szTickCount[]   = TEXT("TickCount");
const TCHAR g_szSuspendLo[]   = TEXT("SuspendLo");
const TCHAR g_szSuspendHi[]   = TEXT("SuspendHi");
const TCHAR g_szSysTimeLo[]   = TEXT("SysTimeLo");
const TCHAR g_szSysTimeHi[]   = TEXT("SysTimeHi");
const TCHAR g_szBackup[]      = TEXT("Backup");
const TCHAR g_szLocalHost[]   = TEXT("127.0.0.1");
const TCHAR g_szRegPath[]     = TEXT("Software\\WR-Tools\\ResInfo");
const TCHAR g_szModName[]     = TEXT("Software\\HTC\\HTCSENSOR\\GSensor\\ModuleName");
const TCHAR g_szWhiteList[]   = TEXT("Software\\HTC\\HTCSENSOR\\GSensor\\WhiteList");
const TCHAR g_szRegHttp[]     = TEXT("http\\Shell\\Open\\Command");
const TCHAR g_szRegAku[]      = TEXT("System\\Versions");
const TCHAR g_szWorking[]     = TEXT("0 h, 00 min, 00 s");
const TCHAR g_szSuspend[]     = TEXT("0 d, 00 h, 00 min, 00 s");
const TCHAR g_szFmtWorking[]  = TEXT("%u h, %02u min, %02u s");
const TCHAR g_szFmtSuspend[]  = TEXT("%u d, %02u h, %02u min, %02u s");
const TCHAR g_szFmtTasks[]    = TEXT("%u %s - %u %s");
const TCHAR g_szFmtWin[]      = TEXT("Windows %u.%u");
const TCHAR g_szFmtWinCe[]    = TEXT("Windows CE %u.%u");
const TCHAR g_szFmtWinCeNet[] = TEXT("Windows CE .NET %u.%u");
const TCHAR g_szFmtWinEmbCe[] = TEXT("Windows Embedded CE %u.%u");
const TCHAR g_szAlkaline[]    = TEXT("\r\nAlkaline\r\n");
const TCHAR g_szNiCd[]        = TEXT("\r\nNickel Cadmium\r\n");
const TCHAR g_szNiMH[]        = TEXT("\r\nNickel Metal Hydride\r\n");
const TCHAR g_szLithIon[]     = TEXT("\r\nLithium Ion\r\n");
const TCHAR g_szLithPoly[]    = TEXT("\r\nLithium Polymer\r\n");
const TCHAR g_szZincAir[]     = TEXT("\r\nZinc Air\r\n");
const TCHAR g_szHelpCmdPart[] = TEXT("file:ResInfo.htm#Page_");
const TCHAR g_szHelpBattery[] = TEXT("Battery");
const TCHAR g_szHelpMemory[]  = TEXT("Memory");
const TCHAR g_szHelpStorage[] = TEXT("Storage");
const TCHAR g_szHelpTasks[]   = TEXT("Tasks");
const TCHAR g_szHelpCpu[]     = TEXT("Processor");
const TCHAR g_szHelpSystem[]  = TEXT("System");
const TCHAR g_szHelpSetting[] = TEXT("Settings");
const TCHAR g_szHelpAbout[]   = TEXT("About");
const TCHAR g_szHelpFAQ[]     = TEXT("FAQ");
const TCHAR g_szHelpGer[]     = TEXT("_Ger");
const TCHAR g_szHelpEng[]     = TEXT("_Eng");
const TCHAR g_szLinkTextGer[] = TEXT("WR-Tools ResInfo <file:peghelp.exe ResInfo.htm#Main_Contents_Ger{Hilfe}>");
const TCHAR g_szLinkTextEng[] = TEXT("WR-Tools ResInfo <file:peghelp.exe ResInfo.htm#Main_Contents_Eng{Help}>");
const TCHAR g_szErrorCreate[] = TEXT("Cannot create main window.");
const TCHAR g_szGetSystemPowerStatusEx[]   = TEXT("GetSystemPowerStatusEx");
const TCHAR g_szGetSystemPowerStatusEx2[]  = TEXT("GetSystemPowerStatusEx2");
const TCHAR g_szGetSystemMemoryDivision[]  = TEXT("GetSystemMemoryDivision");
const TCHAR g_szGetStoreInformation[]      = TEXT("GetStoreInformation");
const TCHAR g_szSendMessageTimeout[]       = TEXT("SendMessageTimeout");
const TCHAR g_szCreateToolhelp32Snapshot[] = TEXT("CreateToolhelp32Snapshot");
const TCHAR g_szCloseToolhelp32Snapshot[]  = TEXT("CloseToolhelp32Snapshot");
const TCHAR g_szProcess32First[]           = TEXT("Process32First");
const TCHAR g_szProcess32Next[]            = TEXT("Process32Next");
const TCHAR g_szThread32First[]            = TEXT("Thread32First");
const TCHAR g_szThread32Next[]             = TEXT("Thread32Next");
const TCHAR g_szModule32First[]            = TEXT("Module32First");
const TCHAR g_szModule32Next[]             = TEXT("Module32Next");
const TCHAR g_szSetProcPermissions[]       = TEXT("SetProcPermissions");
const TCHAR g_szGetThreadTimes[]           = TEXT("GetThreadTimes");
const TCHAR g_szSHCreateMenuBar[]          = TEXT("SHCreateMenuBar");
const TCHAR g_szChooseColor[]              = TEXT("ChooseColor");
const TCHAR g_szExitWindowsEx[]            = TEXT("ExitWindowsEx");
const TCHAR g_szKernelIoControl[]          = TEXT("KernelIoControl");
const TCHAR g_szSetSystemPowerState[]      = TEXT("SetSystemPowerState");
const TCHAR g_szCeFindFirstDatabase[]      = TEXT("CeFindFirstDatabase");
const TCHAR g_szCeFindNextDatabase[]       = TEXT("CeFindNextDatabase");
const TCHAR g_szCeOidGetInfo[]             = TEXT("CeOidGetInfo");
const TCHAR g_szEnumPnpIds[]               = TEXT("EnumPnpIds");
const LPTSTR g_aszClassMin[] = { TEXT("MS_SIPBUTTON"), TEXT("HHTaskBar"),
                                TEXT("MauiTaskBar"), TEXT("MS_SOFTKEY_CE_1.0"),
                                TEXT("MNU"), TEXT("Tray"), TEXT("Worker"), 0 };
const LPTSTR g_aszClassAll[] = { TEXT("DesktopExplorerWindow"), TEXT("MS_SIPBUTTON"),
                                TEXT("Explore"), TEXT("HHTaskBar"), TEXT("Manila"),
                                TEXT("MauiTaskBar"), TEXT("MS_SOFTKEY_CE_1.0"),
                                TEXT("MNU"), TEXT("Tray"), TEXT("Worker"), TEXT("static"),
                                TEXT("MSSTARTMENU"), TEXT("ConfettiLockScreen"), 0 };
const LPTSTR g_aszProcess[] = { TEXT("nk.exe"), TEXT("filesys.exe"), TEXT("gwes.exe"),
                                TEXT("device.exe"), TEXT("udevice.exe"),
                                TEXT("explorer.exe"), TEXT("shell32.exe"),
                                TEXT("services.exe"), TEXT("servicesd.exe"),
                                TEXT("srvtrust.exe"), TEXT("connmgr.exe"),
                                TEXT("telshell.exe"), TEXT("home.exe"), 0 };

HINSTANCE g_hInstance = NULL;
HINSTANCE g_hLibCoreDll = NULL;
HINSTANCE g_hLibToolhelp = NULL;
HINSTANCE g_hLibAygshell = NULL;
WNDPROC   g_lpPrevPropSheetDlgProc = NULL;
HWND      g_hwndPropSheet = NULL;
HWND      g_hwndPropPage1 = NULL;
HWND      g_hwndPropPage2 = NULL;
HWND      g_hwndPropPage3 = NULL;
HWND      g_hwndPropPage4 = NULL;
HWND      g_hwndPropPage8 = NULL;
HWND      g_hwndMenuBar = NULL;
DWORD     g_dwMajorVersion = 0;
DWORD     g_dwMinorVersion = 0;
BOOL      g_bPocketPC = FALSE;
BOOL      g_bGerUI = FALSE;
BOOL      g_bHideState = FALSE;
BOOL      g_bHideApplication = FALSE;
BOOL      g_bResetRuntimes = TRUE;
BOOL      g_bNoMemTrayIcon = FALSE;
BOOL      g_bNoBatTrayIcon = FALSE;
BOOL      g_bNoCpuTrayIcon = TRUE;
BOOL      g_bStaticIcons = FALSE;
BOOL      g_bAlternativeColor = FALSE;
BOOL      g_bClassicView = FALSE;
BOOL      g_bShowClassName = TRUE;
BOOL      g_bHideShellWnd = FALSE;
BOOL      g_bCpuPause = FALSE;
BOOL      g_bCpuDetails = FALSE;
UINT      g_uInterval = TIMER2_PERIOD;
COLORREF  g_crIconColor = RGB(0, 0, 0);

BOOL    (*g_fnGetSystemPowerStatusEx)(PSYSTEM_POWER_STATUS_EX, BOOL);
BOOL    (*g_fnGetSystemMemoryDivision)(LPDWORD, LPDWORD, LPDWORD);
BOOL    (*g_fnGetStoreInformation)(LPSTORE_INFORMATION);
LRESULT (*g_fnSendMessageTimeout)(HWND, UINT, WPARAM, LPARAM, UINT, UINT, LPDWORD);
HANDLE  (*g_fnCreateToolhelp32Snapshot)(DWORD, DWORD);
BOOL    (*g_fnCloseToolhelp32Snapshot)(HANDLE);
BOOL    (*g_fnProcess32First)(HANDLE, LPPROCESSENTRY32);
BOOL    (*g_fnProcess32Next)(HANDLE, LPPROCESSENTRY32);
BOOL    (*g_fnThread32First)(HANDLE, LPTHREADENTRY32);
BOOL    (*g_fnThread32Next)(HANDLE, LPTHREADENTRY32);
BOOL    (*g_fnModule32First)(HANDLE, LPMODULEENTRY32);
BOOL    (*g_fnModule32Next)(HANDLE, LPMODULEENTRY32);
BOOL    (*g_fnSetProcPermissions)(DWORD);
BOOL    (*g_fnGetThreadTimes)(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME, LPFILETIME);


__inline BYTE CalcRatio(DWORD dwNumerator, DWORD dwDenominator)
{
    return (BYTE)(dwDenominator ? (((100i64 * (__int64)(dwNumerator)) +
        ((__int64)(dwDenominator)/2i64)) / (__int64)(dwDenominator)) : 255);
}

__inline BYTE CalcRatioEx(DWORD dwNumerator, DWORD dwDenom1, DWORD dwDenom2)
{
    __int64 llDenominator = (__int64)(dwDenom1) + (__int64)(dwDenom2);
    return (BYTE)(llDenominator ? (((100i64 * (__int64)(dwNumerator)) +
        (llDenominator/2i64)) / llDenominator) : 255);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    g_bPocketPC = FALSE;
    {
        OSVERSIONINFO osvi;
        TCHAR szPlatform[128];
        UINT uPlatform = sizeof(szPlatform);
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);
        g_dwMajorVersion = osvi.dwMajorVersion;
        g_dwMinorVersion = osvi.dwMinorVersion;
        if (SystemParametersInfo(SPI_GETPLATFORMTYPE, uPlatform, (PVOID)szPlatform, 0) ||
            SystemParametersInfo(SPI_GETPROJECTNAME, uPlatform, (PVOID)szPlatform, 0))
            if (g_dwMajorVersion >= 3)
                if (!_tcsicmp(szPlatform, g_szPocketPC) ||
                    !_tcsicmp(szPlatform, g_szPalmPC2) ||
                    !_tcsicmp(szPlatform, g_szSmartPhone))
                    g_bPocketPC = TRUE;
    }

    HANDLE hMutex = CreateMutex(NULL, FALSE, g_szMutex);
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND hWnd = FindWindow(g_szPSDlgClass, g_szTitle);
        if (hWnd != NULL)
        {
            if (!IsWindowVisible(hWnd)) ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(g_bPocketPC ? (HWND)(((DWORD)hWnd) | 0x01) : hWnd);
            PostMessage(hWnd, WM_APP_REFRESH, 0, 0);
            if (hMutex != NULL) CloseHandle(hMutex);
            return 0;
        }
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_PROGRESS_CLASS | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    g_bGerUI = (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_GERMAN);
    if (!g_bGerUI)
    {
        HKEY hKey;
        LONG lStatus = RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegPath, 0, 0, &hKey);
        if (lStatus == ERROR_SUCCESS)
        {
            DWORD dwType = REG_DWORD;
            DWORD dwSize = sizeof(DWORD);
            DWORD dwData = (DWORD)FALSE;
            lStatus  = RegQueryValueEx(hKey, g_szForceGerUI, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
            if (lStatus == ERROR_SUCCESS)
                g_bGerUI = !!(BOOL)dwData;
            RegCloseKey(hKey);
        }
    }

    g_bStaticIcons = g_bPocketPC;
    if (!g_bPocketPC || g_dwMajorVersion < 5)
        g_crIconColor = GetSysColor(COLOR_MENUTEXT);
    if (g_bPocketPC && (g_dwMajorVersion > 5 || (g_dwMajorVersion == 5 && g_dwMinorVersion >= 2)))
    {
        g_bNoMemTrayIcon = TRUE;
        g_bNoBatTrayIcon = TRUE;
        g_bNoCpuTrayIcon = TRUE;
    }

    {
        HKEY hKey = NULL;
        DWORD Disp = 0;
        LONG lStatus = RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegPath, 0, NULL, 0, 0, NULL, &hKey, &Disp);
        if (lStatus == ERROR_SUCCESS)
        {
            DWORD dwType = REG_DWORD;
            DWORD dwSize = sizeof(DWORD);
            DWORD dwData = 0;
            lStatus  = RegQueryValueEx(hKey, g_szUseDefault, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
            if (lStatus == ERROR_SUCCESS && dwData != 0)
            {
                dwData = 0;
                lStatus = RegSetValueEx(hKey, g_szUseDefault, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));
                if (lStatus == ERROR_SUCCESS)
                {
                    dwData = (DWORD)g_crIconColor;
                    RegSetValueEx(hKey, g_szIconColor, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));
                    if (g_bPocketPC)
                    {
                        dwData = 1;
                        RegSetValueEx(hKey, g_szStaticIco, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));
                        if (g_dwMajorVersion > 5 || (g_dwMajorVersion == 5 && g_dwMinorVersion >= 2))
                        {
                            RegSetValueEx(hKey, g_szNoMemIcon, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));
                            RegSetValueEx(hKey, g_szNoBatIcon, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));
                            RegSetValueEx(hKey, g_szNoCpuIcon, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));
                        }
                        HKEY hKey2 = NULL;
                        lStatus = RegOpenKeyEx(HKEY_CURRENT_USER, g_szModName, 0, 0, &hKey2);
                        if (lStatus == ERROR_SUCCESS)
                        {
                            TCHAR szText[MAX_PATH];
                            szText[0] = TEXT('\0');
                            DWORD dwLen = GetModuleFileName(NULL, szText, _countof(szText));
                            if (dwLen > 0)
                                RegSetValueEx(hKey2, g_szResInfo, 0, REG_SZ,
                                    (LPBYTE)szText, (dwLen + 1) * sizeof(TCHAR));
                            RegCloseKey(hKey2); hKey2 = NULL;
                            lStatus = RegOpenKeyEx(HKEY_CURRENT_USER, g_szWhiteList, 0, 0, &hKey2);
                            if (lStatus == ERROR_SUCCESS)
                            {
                                _tcscpy(szText, g_szPSDlgClass);
                                DWORD dwLen = _tcslen(szText);
                                if (dwLen > 0)
                                    RegSetValueEx(hKey2, g_szResInfo, 0, REG_SZ,
                                        (LPBYTE)szText, (dwLen + 1) * sizeof(TCHAR));
                                RegCloseKey(hKey2);
                            }
                        }
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }

    LoadSettings();

    PROPSHEETPAGE psp[8];
    PROPSHEETHEADER psh;

    psp[0].dwSize      = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags     = PSP_USEICONID | PSP_PREMATURE | PSP_USECALLBACK | PSP_HASHELP;
    psp[0].hInstance   = g_hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO1 : IDD_ENG_RESINFO1);
    psp[0].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO1);
    psp[0].pfnDlgProc  = ResInfoDlgProc1;
    psp[0].pszTitle    = NULL;
    psp[0].lParam      = 0;
    psp[0].pfnCallback = PropSheetPageProc;

    psp[1].dwSize      = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags     = PSP_USEICONID | PSP_PREMATURE | PSP_USECALLBACK | PSP_HASHELP;
    psp[1].hInstance   = g_hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO2 : IDD_ENG_RESINFO2);
    psp[1].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO2);
    psp[1].pfnDlgProc  = ResInfoDlgProc2;
    psp[1].pszTitle    = NULL;
    psp[1].lParam      = 0;
    psp[1].pfnCallback = PropSheetPageProc;

    psp[2].dwSize      = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags     = PSP_USEICONID | PSP_USECALLBACK | PSP_HASHELP;
    psp[2].hInstance   = g_hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO3 : IDD_ENG_RESINFO3);
    psp[2].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO3);
    psp[2].pfnDlgProc  = ResInfoDlgProc3;
    psp[2].pszTitle    = NULL;
    psp[2].lParam      = 0;
    psp[2].pfnCallback = PropSheetPageProc;

    psp[3].dwSize      = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags     = PSP_USEICONID | PSP_USECALLBACK | PSP_HASHELP;
    psp[3].hInstance   = g_hInstance;
    psp[3].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO4 : IDD_ENG_RESINFO4);
    psp[3].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO4);
    psp[3].pfnDlgProc  = ResInfoDlgProc4;
    psp[3].pszTitle    = NULL;
    psp[3].lParam      = 0;
    psp[3].pfnCallback = PropSheetPageProc;

    psp[4].dwSize      = sizeof(PROPSHEETPAGE);
    psp[4].dwFlags     = PSP_USEICONID | PSP_PREMATURE | PSP_USECALLBACK | PSP_HASHELP;
    psp[4].hInstance   = g_hInstance;
    psp[4].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO8 : IDD_ENG_RESINFO8);
    psp[4].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO8);
    psp[4].pfnDlgProc  = ResInfoDlgProc8;
    psp[4].pszTitle    = NULL;
    psp[4].lParam      = 0;
    psp[4].pfnCallback = PropSheetPageProc;

    psp[5].dwSize      = sizeof(PROPSHEETPAGE);
    psp[5].dwFlags     = PSP_USEICONID | PSP_HASHELP;
    psp[5].hInstance   = g_hInstance;
    psp[5].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO5 : IDD_ENG_RESINFO5);
    psp[5].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO5);
    psp[5].pfnDlgProc  = ResInfoDlgProc5;
    psp[5].pszTitle    = NULL;
    psp[5].lParam      = 0;
    psp[5].pfnCallback = NULL;

    psp[6].dwSize      = sizeof(PROPSHEETPAGE);
    psp[6].dwFlags     = PSP_USEICONID | PSP_HASHELP;
    psp[6].hInstance   = g_hInstance;
    psp[6].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO6 : IDD_ENG_RESINFO6);
    psp[6].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO6);
    psp[6].pfnDlgProc  = ResInfoDlgProc6;
    psp[6].pszTitle    = NULL;
    psp[6].lParam      = 0;
    psp[6].pfnCallback = NULL;

    psp[7].dwSize      = sizeof(PROPSHEETPAGE);
    psp[7].dwFlags     = PSP_USEICONID | PSP_HASHELP;
    psp[7].hInstance   = g_hInstance;
    psp[7].pszTemplate = MAKEINTRESOURCE(g_bGerUI ? IDD_GER_RESINFO7 : IDD_ENG_RESINFO7);
    psp[7].pszIcon     = MAKEINTRESOURCE(IDI_RESINFO7);
    psp[7].pfnDlgProc  = ResInfoDlgProc7;
    psp[7].pszTitle    = NULL;
    psp[7].lParam      = 0;
    psp[7].pfnCallback = NULL;

    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_HASHELP | PSH_MODELESS;
    psh.hwndParent  = NULL;
    psh.hInstance   = g_hInstance;
    psh.pszIcon     = MAKEINTRESOURCE(IDI_RESINFO);
    psh.nStartPage  = 1;
    psh.ppsp        = (LPCPROPSHEETPAGE)&psp;
    psh.pszCaption  = g_szTitle;
    psh.nPages      = sizeof(psp)/sizeof(PROPSHEETPAGE);
    psh.pfnCallback = PropSheetProc;

    g_hLibAygshell = NULL;
    g_hLibCoreDll  = LoadLibrary(g_szCoreDll);
    g_hLibToolhelp = LoadLibrary(g_szToolHelp);

    if (g_bPocketPC && !g_bClassicView)
    {
        psh.dwFlags |= (PSH_USECALLBACK | PSH_MAXIMIZE);
        g_hLibAygshell = LoadLibrary(g_szAygShell);
    }

    g_fnGetSystemPowerStatusEx = NULL;
    g_fnGetSystemMemoryDivision = NULL;
    g_fnGetStoreInformation = NULL;
    g_fnSendMessageTimeout = NULL;
    g_fnCreateToolhelp32Snapshot = NULL;
    g_fnCloseToolhelp32Snapshot = NULL;
    g_fnProcess32First = NULL;
    g_fnProcess32Next = NULL;
    g_fnThread32First = NULL;
    g_fnThread32Next = NULL;
    g_fnModule32First = NULL;
    g_fnModule32Next = NULL;
    g_fnSetProcPermissions = NULL;
    g_fnGetThreadTimes = NULL;

    if (g_hLibCoreDll != NULL)
    {
        g_fnGetSystemPowerStatusEx = (BOOL(*)(PSYSTEM_POWER_STATUS_EX, BOOL))GetProcAddress(g_hLibCoreDll, g_szGetSystemPowerStatusEx);
        g_fnGetSystemMemoryDivision = (BOOL(*)(LPDWORD, LPDWORD, LPDWORD))GetProcAddress(g_hLibCoreDll, g_szGetSystemMemoryDivision);
        g_fnGetStoreInformation = (BOOL(*)(LPSTORE_INFORMATION))GetProcAddress(g_hLibCoreDll, g_szGetStoreInformation);
        g_fnSendMessageTimeout = (LRESULT(*)(HWND, UINT, WPARAM, LPARAM, UINT, UINT, LPDWORD))GetProcAddress(g_hLibCoreDll, g_szSendMessageTimeout);
        g_fnSetProcPermissions = (BOOL(*)(DWORD))GetProcAddress(g_hLibCoreDll, g_szSetProcPermissions);
        g_fnGetThreadTimes = (BOOL(*)(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME, LPFILETIME))GetProcAddress(g_hLibCoreDll, g_szGetThreadTimes);
    }

    if (g_hLibToolhelp != NULL)
    {
        g_fnCreateToolhelp32Snapshot = (HANDLE(*)(DWORD, DWORD))GetProcAddress(g_hLibToolhelp, g_szCreateToolhelp32Snapshot);
        g_fnCloseToolhelp32Snapshot = (BOOL(*)(HANDLE))GetProcAddress(g_hLibToolhelp, g_szCloseToolhelp32Snapshot);
        g_fnProcess32First = (BOOL(*)(HANDLE, LPPROCESSENTRY32))GetProcAddress(g_hLibToolhelp, g_szProcess32First);
        g_fnProcess32Next = (BOOL(*)(HANDLE, LPPROCESSENTRY32))GetProcAddress(g_hLibToolhelp, g_szProcess32Next);
        g_fnThread32First = (BOOL(*)(HANDLE, LPTHREADENTRY32))GetProcAddress(g_hLibToolhelp, g_szThread32First);
        g_fnThread32Next = (BOOL(*)(HANDLE, LPTHREADENTRY32))GetProcAddress(g_hLibToolhelp, g_szThread32Next);
        g_fnModule32First = (BOOL(*)(HANDLE, LPMODULEENTRY32))GetProcAddress(g_hLibToolhelp, g_szModule32First);
        g_fnModule32Next = (BOOL(*)(HANDLE, LPMODULEENTRY32))GetProcAddress(g_hLibToolhelp, g_szModule32Next);
    }

    __try
    {
        g_hwndPropSheet = (HWND)PropertySheet(&psh);
        if (g_hwndPropSheet == NULL || g_hwndPropSheet == (HWND)-1)
        {
            MessageBox(NULL, g_szErrorCreate, g_szTitle, MB_OK | MB_ICONSTOP);
            if (g_hLibAygshell != NULL) FreeLibrary(g_hLibAygshell);
            if (g_hLibToolhelp != NULL) FreeLibrary(g_hLibToolhelp);
            if (g_hLibCoreDll  != NULL) FreeLibrary(g_hLibCoreDll);
            if (hMutex != NULL) CloseHandle(hMutex);
            return -1;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        MessageBox(NULL, g_szErrorCreate, g_szTitle, MB_OK | MB_ICONSTOP);
        if (g_hLibAygshell != NULL) FreeLibrary(g_hLibAygshell);
        if (g_hLibToolhelp != NULL) FreeLibrary(g_hLibToolhelp);
        if (g_hLibCoreDll  != NULL) FreeLibrary(g_hLibCoreDll);
        if (hMutex != NULL) CloseHandle(hMutex);
        return -1;
    }

    if (g_bHideApplication)
    {
        ShowWindow(g_hwndPropSheet, SW_HIDE);
        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
        g_bHideState = TRUE;
    }
    else
    {
        if (SaveSetting(g_szRunHidden, (DWORD)TRUE) == ERROR_SUCCESS)
            g_bHideApplication = TRUE;
    }

    HACCEL hAccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
    HICON hIconBig = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_RESINFO));
    if (hIconBig != NULL)
        PostMessage(g_hwndPropSheet, WM_SETICON, (WPARAM)(BOOL)ICON_BIG, (LPARAM)(HICON)hIconBig);
    HICON hIconSmall = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_RESINFO), IMAGE_ICON, 16, 16, 0);
    if (hIconSmall != NULL)
        PostMessage(g_hwndPropSheet, WM_SETICON, (WPARAM)(BOOL)ICON_SMALL, (LPARAM)(HICON)hIconSmall);

    MSG msg;
    BOOL bProcessed;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        bProcessed = TranslateAccelerator(g_hwndPropSheet, hAccel, &msg);
        if (!bProcessed)
            bProcessed = PropSheet_IsDialogMessage(g_hwndPropSheet, &msg);

        if (PropSheet_GetCurrentPageHwnd(g_hwndPropSheet) == NULL)
        {
            DestroyWindow(g_hwndPropSheet);
            if (hIconSmall != NULL) DestroyIcon(hIconSmall);
            if (g_hLibAygshell != NULL) FreeLibrary(g_hLibAygshell);
            if (g_hLibToolhelp != NULL) FreeLibrary(g_hLibToolhelp);
            if (g_hLibCoreDll  != NULL) FreeLibrary(g_hLibCoreDll);
            if (hMutex != NULL) CloseHandle(hMutex);
            return msg.wParam;
        }

        if (!bProcessed)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (hIconSmall != NULL) DestroyIcon(hIconSmall);
    if (g_hLibAygshell != NULL) FreeLibrary(g_hLibAygshell);
    if (g_hLibToolhelp != NULL) FreeLibrary(g_hLibToolhelp);
    if (g_hLibCoreDll  != NULL) FreeLibrary(g_hLibCoreDll);
    if (hMutex != NULL) CloseHandle(hMutex);

    return msg.wParam;
}


BOOL CALLBACK EnumWindowsProc1(HWND hwnd, DWORD lParam)
{
    if (IsWindowVisible(hwnd))
    {
        TCHAR szTitle[128];
        szTitle[0] = TEXT('\0');

        if (MyGetWindowText(hwnd, szTitle, _countof(szTitle)) > 0 &&
            _tcsstr(szTitle, g_szBackup) != NULL)
        {
            *((BOOL*)lParam) = TRUE;
            return FALSE;
        }
    }
    return TRUE; 
}


BOOL CALLBACK EnumWindowsProc2(HWND hwnd, DWORD lParam)
{
    DWORD dwProcessId = 0;
    if (!IsWindowVisible(hwnd) ||
        GetWindow(hwnd, GW_OWNER) != NULL ||
        !GetWindowThreadProcessId(hwnd, &dwProcessId))
        return TRUE;

    PTASK_LIST_ENUM pTaskListEnum = (PTASK_LIST_ENUM)lParam;
    PTASK_LIST pTaskList = pTaskListEnum->pTaskList;
    DWORD dwNumTasks = pTaskListEnum->dwNumTasks;

    TCHAR szTitle[MAX_WND_TITLE];
    for (DWORD i = 0; i < dwNumTasks; i++)
    {
        if (pTaskList->dwProcessId == dwProcessId)
        {
            if (MyGetWindowText(hwnd, szTitle, _countof(szTitle)) > 0)
            {
                _tcsncat(pTaskList->szWindowTitle, TEXT("["),
                    MAX_PATH-_tcslen(pTaskList->szWindowTitle)-1);
                _tcsncat(pTaskList->szWindowTitle, szTitle,
                    MAX_PATH-_tcslen(pTaskList->szWindowTitle)-1);
                _tcsncat(pTaskList->szWindowTitle, TEXT("]"),
                    MAX_PATH-_tcslen(pTaskList->szWindowTitle)-1);
            }
            break;
        }
        pTaskList++;
    }
    return TRUE;
}


BOOL CALLBACK EnumWindowsProc3(HWND hwnd, DWORD lParam)
{
    PWND_LIST_ENUM pWndListEnum = (PWND_LIST_ENUM)lParam;
    PWINDOW_LIST pWindowList = pWndListEnum->pWindowList;

    if (pWndListEnum->uWndCount >= MAX_WND_ENUM)
        return FALSE;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    if (GetWindow(hwnd, GW_OWNER) != NULL)
        return TRUE;

    TCHAR szTitle[MAX_WND_TITLE];
    if (MyGetWindowText(hwnd, szTitle, _countof(szTitle)) == 0)
        return TRUE;

    TCHAR szClass[128];
    int nClassLen = GetClassName(hwnd, szClass, _countof(szClass));
    if (nClassLen > 0)
    {
        for (int i=0; g_bHideShellWnd ? g_aszClassAll[i] : g_aszClassMin[i]; i++)
        {
            if (_tcscmp(szClass, g_bHideShellWnd ? g_aszClassAll[i] : g_aszClassMin[i]) == 0)
                return TRUE;
        }
    }

    pWindowList += pWndListEnum->uWndCount;
    pWindowList->hwndMain = hwnd;
    _tcscpy(pWindowList->szWndTitle, szTitle);

    if (g_bShowClassName && nClassLen > 0)
    {
        _tcsncat(pWindowList->szWndTitle, TEXT(" ("),
            MAX_WND_TITLE-_tcslen(pWindowList->szWndTitle)-1);
        _tcsncat(pWindowList->szWndTitle, szClass,
            MAX_WND_TITLE-_tcslen(pWindowList->szWndTitle)-1);
        _tcsncat(pWindowList->szWndTitle, TEXT(")"),
            MAX_WND_TITLE-_tcslen(pWindowList->szWndTitle)-1);
    }

    pWndListEnum->uWndCount++;

    return TRUE; 
}


int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
        case PSCB_PRECREATE:
            {
                LPDLGTEMPLATE lpTemplate = (LPDLGTEMPLATE)lParam;
                __try { lpTemplate->style = (lpTemplate->style & ~WS_POPUP) | WS_OVERLAPPED; }
                __except (EXCEPTION_EXECUTE_HANDLER) { ; }
                break;
            }

        case PSCB_INITIALIZED:
            if (g_hLibAygshell != NULL)
            {
                SHMENUBARINFO mbi;
                memset(&mbi, 0, sizeof(SHMENUBARINFO));
                mbi.cbSize = sizeof(SHMENUBARINFO);
                mbi.hwndParent = hwndDlg;
                mbi.hInstRes = g_hInstance;
                mbi.nToolBarId = g_bGerUI ? IDR_GER_MENU : IDR_ENG_MENU;
                BOOL (*fnSHCreateMenuBar)(PSHMENUBARINFO);
                fnSHCreateMenuBar = (BOOL(*)(PSHMENUBARINFO))GetProcAddress(g_hLibAygshell,
                    g_szSHCreateMenuBar);
                if ((fnSHCreateMenuBar != NULL) && fnSHCreateMenuBar(&mbi))
                    g_hwndMenuBar = mbi.hwndMB;
            }
            g_lpPrevPropSheetDlgProc = (WNDPROC)SetWindowLong(hwndDlg, GWL_WNDPROC, (LONG)PropSheetDlgProc);
            break;

        case PSCB_GETVERSION:
            return (g_dwMajorVersion < 5 ? 0x020c : 0x020e);

        case PSCB_GETTITLE:
            _tcscpy((TCHAR*)lParam, g_szTitle2);
            break;

        case PSCB_GETLINKTEXT:
            _tcscpy((TCHAR*)lParam, g_bGerUI ? g_szLinkTextGer : g_szLinkTextEng);
            break;
            
        default:
            break;
    }

    return 0;
}


UINT CALLBACK PropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE lppsp)
{
    if (uMsg == PSPCB_RELEASE && lppsp->lParam != 0)
        SendMessage((HWND)(lppsp->lParam), WM_PSPCB_RELEASE, (WPARAM)uMsg, (LPARAM)hwnd);

    return 1;
}


LRESULT CALLBACK PropSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = CallWindowProc(g_lpPrevPropSheetDlgProc, hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_ACTIVATE:
            if (g_hwndMenuBar != NULL &&
                (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) &&
                IsWindowVisible(hwnd) && !IsWindowVisible(g_hwndMenuBar))
                    ShowWindow(g_hwndMenuBar, SW_SHOW);
            break;

        case WM_APP_REFRESH:
                PostMessage(PropSheet_GetCurrentPageHwnd(hwnd), WM_TIMER, 0, 0);
            break;

        case WM_DEVICECHANGE:
            {
                if (lResult == FALSE && (wParam == DBT_DEVICEARRIVAL ||
                    wParam == DBT_DEVICEREMOVECOMPLETE || wParam == DBT_MONITORCHANGE))
                    lResult = SendMessage(PropSheet_GetCurrentPageHwnd(hwnd), uMsg, wParam, lParam);
            }
            break;

        case WM_SETTINGCHANGE:
            {
                if (lResult == FALSE && wParam == SETTINGCHANGE_RESET)
                    lResult = SendMessage(PropSheet_GetCurrentPageHwnd(hwnd), uMsg, wParam, lParam);
            }
            break;
    }

    return lResult;
}


BOOL CALLBACK ResInfoDlgProc1(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HICON hIcon2 = NULL;
    static BYTE cPcBatOldSave = BATTERY_FLAG_UNKNOWN;
    static BYTE cPcBatOld = 0xF0;
    static BYTE cFlagBatOld = 0xF0;
    static BYTE cFlagBackupBatOld = 0xF0;
    static BYTE cACLineOld = 0xF0;
    static BOOL bEnable = TRUE;
    static UINT uResetCounter = 0;
    static UINT uSaveCounter = 0;
    static DWORD dwWorking = 0;
    static DWORD dwTickCountOld = 0;
    static BOOL bHandleRollover = FALSE;
    static __int64 llSuspend = 0i64;
    static __int64 llSysTimeOld = 0i64;

    switch(msg)
    {
        case WM_TIMER:
            {
                TCHAR szText[128];
                BYTE cPcBat = BATTERY_PERCENTAGE_UNKNOWN;
                BYTE cFlagBat = BATTERY_FLAG_UNKNOWN;
                BYTE cFlagBackupBat = BATTERY_FLAG_UNKNOWN;
                BYTE cACLine = AC_LINE_UNKNOWN;

                SYSTEM_POWER_STATUS_EX status;
                __try
                {
                    if (g_fnGetSystemPowerStatusEx != NULL &&
                        g_fnGetSystemPowerStatusEx(&status, TRUE))
                    {
                        cPcBat = status.BatteryLifePercent;
                        if (cPcBat > 100)
                            cPcBat = BATTERY_PERCENTAGE_UNKNOWN;
                        cFlagBat = status.BatteryFlag;
                        cFlagBackupBat = status.BackupBatteryFlag;
                        cACLine = status.ACLineStatus;
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) { ; }

                DWORD dwTickCount = GetTickCount();
                if (dwTickCountOld == 0)
                    dwTickCountOld = dwTickCount;

                DWORD dwDelta;
                if (dwTickCount < dwTickCountOld)
                    dwDelta = (0xFFFFFFFF - dwTickCountOld) + dwTickCount + 1;
                else
                    dwDelta = dwTickCount - dwTickCountOld;

                if (dwDelta > XSCALE_ROLLOVER_MIN && dwDelta < XSCALE_ROLLOVER_MAX)
                {
                    if (bHandleRollover)
                        dwDelta = TIMER1_PERIOD;
                    else
                        bHandleRollover = TRUE;
                }

                SYSTEMTIME st;
                FILETIME ft;
                GetSystemTime(&st);
                SystemTimeToFileTime(&st, &ft);

                __int64 llSysTime = ft.dwHighDateTime;
                llSysTime <<= 32;
                llSysTime |= ft.dwLowDateTime;

                if (llSysTimeOld == 0i64)
                    llSysTimeOld = llSysTime;

                __int64 llDelta = (llSysTime - llSysTimeOld) / 10000i64;
                if (llDelta > 31536000000i64)
                    llDelta = 0i64;

                if (cACLine != AC_LINE_ONLINE || uSaveCounter == 0)
                {
                    dwWorking += dwDelta;

                    DWORD dwDay    = 0;
                    DWORD dwHour   = (dwWorking / 3600000);
                    DWORD dwMinute = (dwWorking % 3600000 / 60000);
                    DWORD dwSecond = (dwWorking % 3600000 % 60000 / 1000);

                    if (!bEnable)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_TIME_WORKING), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_TIME_SUSPEND), TRUE);
                        bEnable = TRUE;
                    }

                    wsprintf(szText, g_szFmtWorking, dwHour, dwMinute, dwSecond);
                    SetDlgItemText(hDlg, IDC_TIME_WORKING, szText);

                    BOOL bSuspended = llDelta > (__int64)(2 * dwDelta) || llDelta < 0i64;
                    if (bSuspended || uSaveCounter == 0)
                    {
                        if (bSuspended)
                        {
                            if (llDelta < 0i64 && (-llDelta) > llSuspend)
                                llSuspend = 0i64;
                            else
                                llSuspend += llDelta;
                        }

                        dwDay    = (DWORD)(llSuspend / 86400000i64);
                        dwHour   = (DWORD)(llSuspend % 86400000i64 / 3600000i64);
                        dwMinute = (DWORD)(llSuspend % 86400000i64 % 3600000i64 / 60000i64);
                        dwSecond = (DWORD)(llSuspend % 86400000i64 % 3600000i64 % 60000i64 / 1000i64);

                        wsprintf(szText, g_szFmtSuspend, dwDay, dwHour, dwMinute, dwSecond);
                        SetDlgItemText(hDlg, IDC_TIME_SUSPEND, szText);

                        if (uSaveCounter != 0)
                            SaveRuntimeData(dwWorking, dwTickCount, llSuspend, llSysTime);
                    }

                    uSaveCounter += 1;
                    if (uSaveCounter > SAVE_PERIOD)
                    {
                        BOOL bBackupRunning = FALSE;

                        EnumWindows((WNDENUMPROC)EnumWindowsProc1, (LPARAM)&bBackupRunning);

                        if (!bBackupRunning)
                            SaveRuntimeData(dwWorking, dwTickCount, llSuspend, llSysTime);

                        uSaveCounter = 1;
                    }
                }
                else if (bEnable)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_TIME_WORKING), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_TIME_SUSPEND), FALSE);
                    bEnable = FALSE;
                }

                if (g_bResetRuntimes && ((cACLine == AC_LINE_ONLINE && cPcBat == 100 &&
                    (cFlagBat & BATTERY_FLAG_HIGH || cFlagBat & BATTERY_FLAG_NO_BATTERY) &&
                    !(cFlagBat & BATTERY_FLAG_CHARGING)) || (uResetCounter > 0) ||
                    (cPcBat > cPcBatOldSave && cPcBat <= 100 &&
                    ((cPcBat-cPcBatOldSave) >= 50 || (cPcBat == 100 && (100-cPcBatOldSave) >= 25)))))
                {
                    if (dwWorking != 0 || llSuspend != 0i64)
                    {
                        uResetCounter += 1;

                        if (cPcBat < cPcBatOldSave)
                            uResetCounter = 0;

                        if (uResetCounter > 3)
                        {
                            dwWorking = 0;
                            llSuspend = 0i64;
                            SetDlgItemText(hDlg, IDC_TIME_WORKING, g_szWorking);
                            SetDlgItemText(hDlg, IDC_TIME_SUSPEND, g_szSuspend);
                            SaveRuntimeData(0, dwTickCount, 0i64, llSysTime);
                            uResetCounter = 0;
                        }
                    }
                }

                dwTickCountOld = dwTickCount;
                llSysTimeOld  = llSysTime;

                if (cPcBat <= 100 && cFlagBat != BATTERY_FLAG_UNKNOWN && cPcBat != cPcBatOldSave)
                    cPcBatOldSave = cPcBat;

                if (cPcBat != cPcBatOld || cFlagBat != cFlagBatOld)
                {
                    TCHAR szPercent[64];
                    if (cPcBat == BATTERY_PERCENTAGE_UNKNOWN)
                    {
                        _tcscpy(szText, TEXT("?"));
                        szPercent[0] = TEXT('\0');
                        LoadString(g_hInstance, g_bGerUI ? IDS_GER_UNKNOWN : IDS_ENG_UNKNOWN, szPercent, _countof(szPercent));
                    }
                    else
                    {
                        wsprintf(szText, TEXT("%u"), (UINT)cPcBat);
                        _tcscpy(szPercent, szText);
                        _tcscat(szPercent, TEXT("%"));
                    }

                    SetDlgItemText(hDlg, IDC_BAT, szPercent);
                    SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_BAT), PBM_SETPOS,
                        (WPARAM)((cPcBat <= 100) ? cPcBat : 0), 0);

                    if (!g_bNoBatTrayIcon)
                    {
                        if (g_bStaticIcons)
                        {
                            if (hIcon2 != NULL) DestroyIcon(hIcon2);
                            hIcon2 = CreateTrayIcon4(cPcBat, cFlagBat);
                        }
                        else if (g_bPocketPC)
                        {
                            HICON hIconNew = CreateTrayIcon2(szText, cPcBat, cFlagBat);
                            if (hIcon2 != NULL) DestroyIcon(hIcon2);
                            hIcon2 = hIconNew;
                        }
                        else
                        {
                            if (hIcon2 != NULL) DestroyIcon(hIcon2);
                            hIcon2 = CreateTrayIcon2(szText, cPcBat, cFlagBat);
                        }
                        TrayMessage(hDlg, NIM_MODIFY, ID_TRAY2, hIcon2);
                    }
                    if (cPcBat != cPcBatOld)
                        cPcBatOld = cPcBat;
                }

                if (cFlagBat != cFlagBatOld || cFlagBackupBat != cFlagBackupBatOld || cACLine != cACLineOld)
                {
                    TCHAR szMessage[128];
                    szMessage[0] = TEXT('\0');
                    _tcscpy(szText, TEXT("?"));

                    cFlagBatOld       = cFlagBat;
                    cFlagBackupBatOld = cFlagBackupBat;
                    cACLineOld        = cACLine;

                    if (cFlagBat == BATTERY_FLAG_UNKNOWN && cFlagBackupBat == BATTERY_FLAG_UNKNOWN)
                    {
                        LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_AC : IDS_ENG_BAT_AC, szMessage, _countof(szMessage));
                        switch (cACLine)
                        {
                            case AC_LINE_OFFLINE:
                                LoadString(g_hInstance, g_bGerUI ? IDS_GER_AC_OFFLINE : IDS_ENG_AC_OFFLINE, szText, _countof(szText));
                                break;
                            case AC_LINE_ONLINE:
                                LoadString(g_hInstance, g_bGerUI ? IDS_GER_AC_ONLINE : IDS_ENG_AC_ONLINE, szText, _countof(szText));
                                break;
                            case AC_LINE_BACKUP_POWER:
                                LoadString(g_hInstance, g_bGerUI ? IDS_GER_AC_BACKUP : IDS_ENG_AC_BACKUP, szText, _countof(szText));
                                break;
                            case AC_LINE_UNKNOWN:
                            default:
                                LoadString(g_hInstance, g_bGerUI ? IDS_GER_UNKNOWN : IDS_ENG_UNKNOWN, szText, _countof(szText));
                                break;
                        }
                    }
                    else
                    {
                        if (cFlagBackupBat == BATTERY_FLAG_UNKNOWN)
                            cFlagBackupBat = 0x00;
                        else if (cFlagBat == BATTERY_FLAG_UNKNOWN)
                            cFlagBat = 0x00;

                        if (cFlagBat & BATTERY_FLAG_HIGH && cFlagBackupBat & BATTERY_FLAG_HIGH)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN_BACKUP : IDS_ENG_BAT_MAIN_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_HIGH : IDS_ENG_FLAG_HIGH, szText, _countof(szText));
                        }
                        else if (cFlagBat & BATTERY_FLAG_HIGH)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN : IDS_ENG_BAT_MAIN, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_HIGH : IDS_ENG_FLAG_HIGH, szText, _countof(szText));
                        }
                        else if (cFlagBackupBat & BATTERY_FLAG_HIGH)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_BACKUP : IDS_ENG_BAT_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_HIGH : IDS_ENG_FLAG_HIGH, szText, _countof(szText));
                        }

                        if (cFlagBat & BATTERY_FLAG_LOW && cFlagBackupBat & BATTERY_FLAG_LOW)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN_BACKUP : IDS_ENG_BAT_MAIN_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_LOW : IDS_ENG_FLAG_LOW, szText, _countof(szText));
                        }
                        else if (cFlagBat & BATTERY_FLAG_LOW)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN : IDS_ENG_BAT_MAIN, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_LOW : IDS_ENG_FLAG_LOW, szText, _countof(szText));
                        }
                        else if (cFlagBackupBat & BATTERY_FLAG_LOW)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_BACKUP : IDS_ENG_BAT_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_LOW : IDS_ENG_FLAG_LOW, szText, _countof(szText));
                        }

                        if (cFlagBat & BATTERY_FLAG_CRITICAL && cFlagBackupBat & BATTERY_FLAG_CRITICAL)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN_BACKUP : IDS_ENG_BAT_MAIN_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_CRITICAL : IDS_ENG_FLAG_CRITICAL, szText, _countof(szText));
                        }
                        else if (cFlagBat & BATTERY_FLAG_CRITICAL)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN : IDS_ENG_BAT_MAIN, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_CRITICAL : IDS_ENG_FLAG_CRITICAL, szText, _countof(szText));
                        }
                        else if (cFlagBackupBat & BATTERY_FLAG_CRITICAL)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_BACKUP : IDS_ENG_BAT_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_CRITICAL : IDS_ENG_FLAG_CRITICAL, szText, _countof(szText));
                        }

                        if (cFlagBat & BATTERY_FLAG_CHARGING && cFlagBackupBat & BATTERY_FLAG_CHARGING)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN_BACKUP : IDS_ENG_BAT_MAIN_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_CHARGING : IDS_ENG_FLAG_CHARGING, szText, _countof(szText));
                        }
                        else if (cFlagBat & BATTERY_FLAG_CHARGING)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN : IDS_ENG_BAT_MAIN, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_CHARGING : IDS_ENG_FLAG_CHARGING, szText, _countof(szText));
                        }
                        else if (cFlagBackupBat & BATTERY_FLAG_CHARGING)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_BACKUP : IDS_ENG_BAT_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_CHARGING : IDS_ENG_FLAG_CHARGING, szText, _countof(szText));
                        }

                        if (cFlagBat & BATTERY_FLAG_NO_BATTERY && cFlagBackupBat & BATTERY_FLAG_NO_BATTERY)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN_BACKUP : IDS_ENG_BAT_MAIN_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_NO_BATTERY : IDS_ENG_FLAG_NO_BATTERY, szText, _countof(szText));
                        }
                        else if (cFlagBat & BATTERY_FLAG_NO_BATTERY)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_MAIN : IDS_ENG_BAT_MAIN, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_NO_BATTERY : IDS_ENG_FLAG_NO_BATTERY, szText, _countof(szText));
                        }
                        else if (cFlagBackupBat & BATTERY_FLAG_NO_BATTERY)
                        {
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_BAT_BACKUP : IDS_ENG_BAT_BACKUP, szMessage, _countof(szMessage));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FLAG_NO_BATTERY : IDS_ENG_FLAG_NO_BATTERY, szText, _countof(szText));
                        }
                    }

                    _tcscat(szMessage, TEXT(" "));
                    _tcscat(szMessage, szText);
                    SetDlgItemText(hDlg, IDC_BAT_STATUS, szMessage);
                }
            }
            return FALSE;

        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpBattery);
                }
            }
            break;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_ADV_BAT:
                        {
                            TCHAR szText[256];
                            TCHAR szFormat[256];
                            SYSTEM_POWER_STATUS_EX2 sps;
                            memset(&sps, 0, sizeof(sps));

                            __try
                            {
                                if (g_hLibCoreDll != NULL)
                                {
                                    DWORD (*fnGetSystemPowerStatusEx2)(PSYSTEM_POWER_STATUS_EX2, DWORD, BOOL);
                                    fnGetSystemPowerStatusEx2 = (DWORD(*)(PSYSTEM_POWER_STATUS_EX2, DWORD, BOOL))GetProcAddress(g_hLibCoreDll,
                                        g_szGetSystemPowerStatusEx2);
                                    if (fnGetSystemPowerStatusEx2 != NULL)
                                        fnGetSystemPowerStatusEx2(&sps, sizeof(sps), TRUE);
                                    else if (g_fnGetSystemPowerStatusEx != NULL)
                                        g_fnGetSystemPowerStatusEx((SYSTEM_POWER_STATUS_EX*)&sps, TRUE);
                                }
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER) { ; }

                            if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_BATTERY : IDP_ENG_BATTERY, szFormat, _countof(szFormat)) > 0)
                            {
                                TCHAR szChem[64];
                                TCHAR szTitle[128];
                                szTitle[0] = TEXT('\0');
                                LoadString(g_hInstance, g_bGerUI ? IDS_GER_ELECTRIC : IDS_ENG_ELECTRIC, szTitle, _countof(szTitle));
                                szChem[0] = TEXT('\0');
                                switch (sps.BatteryChemistry)
                                {
                                    case BATTERY_CHEMISTRY_ALKALINE:
                                        _tcscpy(szChem, g_szAlkaline);
                                        break;
                                    case BATTERY_CHEMISTRY_NICD:
                                        _tcscpy(szChem, g_szNiCd);
                                        break;
                                    case BATTERY_CHEMISTRY_NIMH:
                                        _tcscpy(szChem, g_szNiMH);
                                        break;
                                    case BATTERY_CHEMISTRY_LION:
                                        _tcscpy(szChem, g_szLithIon);
                                        break;
                                    case BATTERY_CHEMISTRY_LIPOLY:
                                        _tcscpy(szChem, g_szLithPoly);
                                        break;
                                    case BATTERY_CHEMISTRY_ZINCAIR:
                                        _tcscpy(szChem, g_szZincAir);
                                        break;
                                    case BATTERY_CHEMISTRY_UNKNOWN:
                                    default:
                                        _tcscpy(szChem, TEXT("\r\n"));
                                        break;
                                }
                                HKEY hKey;
                                LONG lStatus = RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegPath, 0, 0, &hKey);
                                if (lStatus == ERROR_SUCCESS)
                                {
                                    DWORD dwType = REG_DWORD;
                                    DWORD dwSize = sizeof(DWORD);
                                    DWORD dwData = (DWORD)FALSE;
                                    lStatus  = RegQueryValueEx(hKey, g_szAdjustTemp, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
                                    if (lStatus == ERROR_SUCCESS && (BOOL)dwData)
                                        sps.BatteryTemperature *= 10;
                                    RegCloseKey(hKey);
                                }
                                if (sps.BackupBatteryLifePercent > 100) sps.BackupBatteryLifePercent = 0;
                                _stprintf(szText, szFormat, szChem, (double)(sps.BatteryVoltage / 1000.0),
                                    (int)sps.BatteryCurrent, (double)((int)sps.BatteryTemperature / 10.0),
                                    (double)(sps.BackupBatteryVoltage / 1000.0), sps.BackupBatteryLifePercent);
                                MessageBox(hDlg, szText, szTitle, MB_OK | MB_ICONINFORMATION);
                            }
                        }
                        return FALSE;

                    case IDC_RESET:
                        if (dwWorking > 60000 || llSuspend > 60000i64)
                        {
                            TCHAR szText[128];
                            if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_RESET : IDP_ENG_RESET, szText, _countof(szText)) > 0)
                                if (MessageBox(hDlg, szText, g_szTitle, MB_YESNO | MB_ICONQUESTION) != IDYES)
                                    return FALSE;
                        }
                        dwWorking  = 0;
                        llSuspend = 0i64;
                        SetDlgItemText(hDlg, IDC_TIME_WORKING, g_szWorking);
                        SetDlgItemText(hDlg, IDC_TIME_SUSPEND, g_szSuspend);
                        SaveRuntimeData(0, dwTickCountOld, 0i64, llSysTimeOld);
                        PostMessage(hDlg, WM_TIMER, ID_TIMER1, 0);
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;
                }
            }
            break;

        case WM_APP_ADDICON:
            {
                TCHAR szText[32];
                if (cPcBatOld == BATTERY_PERCENTAGE_UNKNOWN)
                    _tcscpy(szText, TEXT("?"));
                else
                    wsprintf(szText, TEXT("%u"), (UINT)cPcBatOld);

                if (hIcon2 != NULL)
                    DestroyIcon(hIcon2);
                if (g_bStaticIcons)
                    hIcon2 = CreateTrayIcon4(cPcBatOld, cFlagBatOld);
                else
                    hIcon2 = CreateTrayIcon2(szText, cPcBatOld, cFlagBatOld);
                TrayMessage(hDlg, NIM_ADD, ID_TRAY2, hIcon2);
            }
            return TRUE;

        case WM_APP_DELICON:
            TrayMessage(hDlg, NIM_DELETE, ID_TRAY2, hIcon2);
            if (hIcon2 != NULL)
            {
                DestroyIcon(hIcon2);
                hIcon2 = NULL;
            }
            return TRUE;

        case WM_SYSCOLORCHANGE:
            {
                if (!g_bNoBatTrayIcon)
                {
                    TCHAR szText[32];
                    if (cPcBatOld == BATTERY_PERCENTAGE_UNKNOWN)
                        _tcscpy(szText, TEXT("?"));
                    else
                        wsprintf(szText, TEXT("%u"), (UINT)cPcBatOld);

                    if (g_bStaticIcons)
                    {
                        if (hIcon2 != NULL) DestroyIcon(hIcon2);
                        hIcon2 = CreateTrayIcon4(cPcBatOld, cFlagBatOld);
                    }
                    else if (g_bPocketPC)
                    {
                        HICON hIconNew = CreateTrayIcon2(szText, cPcBatOld, cFlagBatOld);
                        if (hIcon2 != NULL) DestroyIcon(hIcon2);
                        hIcon2 = hIconNew;
                    }
                    else
                    {
                        if (hIcon2 != NULL) DestroyIcon(hIcon2);
                        hIcon2 = CreateTrayIcon2(szText, cPcBatOld, cFlagBatOld);
                    }
                    TrayMessage(hDlg, NIM_MODIFY, ID_TRAY2, hIcon2);
                }
            }
            return TRUE;

        case WM_INITDIALOG:
            {
                g_hwndPropPage1 = hDlg;
                ((LPPROPSHEETPAGE)lParam)->lParam = (LPARAM)hDlg;
                SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_BAT), PBM_SETPOS, 0, 0);
                SetDlgItemText(hDlg, IDC_TIME_WORKING, g_szWorking);
                SetDlgItemText(hDlg, IDC_TIME_SUSPEND, g_szSuspend);
                if (!g_bNoBatTrayIcon)
                {
                    if (g_bStaticIcons)
                        hIcon2 = CreateTrayIcon4();
                    else
                        hIcon2 = CreateTrayIcon2(TEXT("?"));
                    TrayMessage(hDlg, NIM_ADD, ID_TRAY2, hIcon2);
                }
                LoadRuntimeData(&dwWorking, &dwTickCountOld, &bHandleRollover, &llSuspend, &llSysTimeOld);
            }
            return TRUE;

        case WM_PSPCB_RELEASE:
            {
                SaveRuntimeData(dwWorking, dwTickCountOld, llSuspend, llSysTimeOld);
                if (!g_bNoBatTrayIcon)
                {
                    TrayMessage(hDlg, NIM_DELETE, ID_TRAY2, hIcon2);
                    if (hIcon2 != NULL)
                    {
                        DestroyIcon(hIcon2);
                        hIcon2 = NULL;
                    }
                }
                g_hwndPropPage1 = NULL;
            }
            return TRUE;

        case TRAY_NOTIFYICON:
            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    if (wParam == ID_TRAY2)
                    {
                        PropSheet_SetCurSel(g_hwndPropSheet, NULL, 0);
                        if (g_bHideState)
                        {
                            ShowWindow(g_hwndPropSheet, SW_SHOW);
                            if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_SHOW);
                            g_bHideState = FALSE;
                        }
                        SetForegroundWindow(g_bPocketPC ? (HWND)(((DWORD)g_hwndPropSheet) | 0x01) : g_hwndPropSheet);
                    }
            }
            return TRUE;
    }

    return FALSE;
}


BOOL CALLBACK ResInfoDlgProc2(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HICON hIcon1 = NULL;
    static DWORD dwMemTotalOld  = (DWORD)-1;
    static DWORD dwMemFreeOld   = (DWORD)-1;
    static DWORD dwMemFilledOld = (DWORD)-1;
    static DWORD dwObjTotalOld  = (DWORD)-1;
    static DWORD dwObjFreeOld   = (DWORD)-1;
    static DWORD dwObjFilledOld = (DWORD)-1;
    static BYTE  cPcMemOld = 255;
    static BYTE  cPcObjOld = 255;
    static BYTE  cPcMemDivOld = 255;

    switch(msg)
    {
        case WM_TIMER:
            {
                if (g_hwndPropPage1 != NULL)
                    PostMessage(g_hwndPropPage1, WM_TIMER, ID_TIMER1, 0);
                if (g_hwndPropPage3 != NULL)
                    PostMessage(g_hwndPropPage3, WM_TIMER, ID_TIMER1, 0);
                if (g_hwndPropPage4 != NULL)
                    PostMessage(g_hwndPropPage4, WM_TIMER, ID_TIMER1, 0);

                TCHAR szFilled[32];
                TCHAR szFree[32];
                TCHAR szText[64];

                MEMORYSTATUS MemoryStatus;
                MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
                GlobalMemoryStatus(&MemoryStatus);
                
                DWORD dwMemTotal  = MemoryStatus.dwTotalPhys >> 10;
                DWORD dwMemFree   = MemoryStatus.dwAvailPhys >> 10;
                DWORD dwMemFilled = dwMemTotal - dwMemFree;
                BYTE  cPcMem = CalcRatio(MemoryStatus.dwTotalPhys - MemoryStatus.dwAvailPhys, MemoryStatus.dwTotalPhys);

                BYTE cPcMemDiv = 255;
                DWORD dwKernelStoreSize = 0;
                if (g_fnGetSystemMemoryDivision != NULL)
                {
                    DWORD dwStorePages, dwRamPages, dwPageSize;
                    dwStorePages = dwRamPages = dwPageSize = 0;
                    if (g_fnGetSystemMemoryDivision(&dwStorePages, &dwRamPages, &dwPageSize))
                    {
                        dwKernelStoreSize = dwStorePages * dwPageSize;
                        cPcMemDiv = CalcRatioEx(dwRamPages, dwRamPages, dwStorePages);
                    }
                }

                DWORD dwObjTotal  = 0;
                DWORD dwObjFree   = 0;
                DWORD dwObjFilled = 0;
                BYTE  cPcObj = 255;
                STORE_INFORMATION si;
                si.dwStoreSize = 0;
                if (g_fnGetStoreInformation != NULL && g_fnGetStoreInformation(&si))
                {
                    if (dwKernelStoreSize != 0 && (!g_bPocketPC || g_dwMajorVersion < 5))
                    {
                        DWORD dwDiff = 0;
                        if (dwKernelStoreSize >= si.dwStoreSize)
                            dwDiff = dwKernelStoreSize - si.dwStoreSize;
                        else
                            dwDiff = si.dwStoreSize - dwKernelStoreSize;
                        if (dwDiff <= 0x100000)
                            si.dwStoreSize = dwKernelStoreSize;
                    }
                    dwObjTotal  = si.dwStoreSize >> 10;
                    dwObjFree   = si.dwFreeSize >> 10;
                    dwObjFilled = dwObjTotal - dwObjFree;
                    cPcObj = CalcRatio(si.dwStoreSize - si.dwFreeSize, si.dwStoreSize);
                }
#if !defined(_WIN32_WCE_EMULATION) || (_WIN32_WCE > 201)
                else
                {
                    ULARGE_INTEGER FreeBytesAvailableToCaller;
                    ULARGE_INTEGER TotalNumberOfBytes;
                    ULARGE_INTEGER TotalNumberOfFreeBytes;
                    if (GetDiskFreeSpaceEx(NULL, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
                    {
                        dwObjTotal  = TotalNumberOfBytes.HighPart > 1023 ?
                            0 : (DWORD)(TotalNumberOfBytes.QuadPart / 1024i64);
                        dwObjFree   = TotalNumberOfFreeBytes.HighPart > 1023 ?
                            0 : (DWORD)(TotalNumberOfFreeBytes.QuadPart / 1024i64);
                        dwObjFilled = dwObjTotal < dwObjFree ? 0 : dwObjTotal - dwObjFree;
                        cPcObj = (BYTE)100 - (BYTE)((TotalNumberOfBytes.QuadPart / 200i64 +
                            TotalNumberOfFreeBytes.QuadPart) / (TotalNumberOfBytes.QuadPart / 100i64));
                        si.dwStoreSize = (TotalNumberOfBytes.HighPart ? (DWORD)-1 : TotalNumberOfBytes.LowPart);
                    }
                }
#endif

                if (cPcMemDiv >= 100)
                    cPcMemDiv = CalcRatioEx(MemoryStatus.dwTotalPhys, MemoryStatus.dwTotalPhys, si.dwStoreSize);

                if (dwMemFilled != dwMemFilledOld || dwMemFree != dwMemFreeOld ||
                    dwObjFilled != dwObjFilledOld || dwObjFree != dwObjFreeOld)
                {
                    LoadString(g_hInstance,  g_bGerUI ? IDS_GER_FILLED : IDS_ENG_FILLED, szFilled, _countof(szFilled));
                    LoadString(g_hInstance,  g_bGerUI ? IDS_GER_FREE : IDS_ENG_FREE, szFree, _countof(szFree));
                }

                if (dwMemTotal != dwMemTotalOld)
                {
                    KilobyteToString(dwMemTotal, szText);
                    SetDlgItemText(hDlg, IDC_MEM, szText);
                    dwMemTotalOld = dwMemTotal;
                }
                if (dwMemFilled != dwMemFilledOld)
                {
                    KilobyteToString(dwMemFilled, szText);
                    _tcscat(szText, TEXT(" "));
                    _tcscat(szText, szFilled);
                    SetDlgItemText(hDlg, IDC_FILLED_MEM, szText);
                    dwMemFilledOld = dwMemFilled;
                }
                if (dwMemFree != dwMemFreeOld)
                {
                    KilobyteToString(dwMemFree, szText);
                    _tcscat(szText, TEXT(" "));
                    _tcscat(szText, szFree);
                    SetDlgItemText(hDlg, IDC_FREE_MEM, szText);
                    dwMemFreeOld = dwMemFree;
                }
                if (dwObjTotal != dwObjTotalOld)
                {
                    KilobyteToString(dwObjTotal, szText);
                    SetDlgItemText(hDlg, IDC_OBJ, szText);
                    dwObjTotalOld = dwObjTotal;
                }
                if (dwObjFilled != dwObjFilledOld)
                {
                    KilobyteToString(dwObjFilled, szText);
                    _tcscat(szText, TEXT(" "));
                    _tcscat(szText, szFilled);
                    SetDlgItemText(hDlg, IDC_FILLED_OBJ, szText);
                    dwObjFilledOld = dwObjFilled;
                }
                if (dwObjFree != dwObjFreeOld)
                {
                    KilobyteToString(dwObjFree, szText);
                    _tcscat(szText, TEXT(" "));
                    _tcscat(szText, szFree);
                    SetDlgItemText(hDlg, IDC_FREE_OBJ, szText);
                    dwObjFreeOld = dwObjFree;
                }

                if (cPcMem != cPcMemOld || cPcObj != cPcObjOld || cPcMemDiv != cPcMemDivOld)
                {
                    if (cPcMem != cPcMemOld)
                    {
                        SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_MEM), PBM_SETPOS,
                            (WPARAM)((cPcMem <= 100) ? cPcMem : 0), 0);
                        cPcMemOld = cPcMem;
                    }

                    if (cPcObj != cPcObjOld)
                    {
                        SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_OBJ), PBM_SETPOS,
                            (WPARAM)((cPcObj <= 100) ? cPcObj : 0), 0);
                        cPcObjOld = cPcObj;
                    }

                    if (cPcMemDiv != cPcMemDivOld)
                        cPcMemDivOld = cPcMemDiv;

                    if (!g_bNoMemTrayIcon)
                    {
                        if (g_bStaticIcons)
                        {
                            if (hIcon1 != NULL) DestroyIcon(hIcon1);
                            hIcon1 = CreateTrayIcon3(cPcMem);
                        }
                        else if (g_bPocketPC)
                        {
                            HICON hIconNew = CreateTrayIcon1(cPcMem, cPcObj, cPcMemDiv);
                            if (hIcon1 != NULL) DestroyIcon(hIcon1);
                            hIcon1 = hIconNew;
                        }
                        else
                        {
                            if (hIcon1 != NULL) DestroyIcon(hIcon1);
                            hIcon1 = CreateTrayIcon1(cPcMem, cPcObj, cPcMemDiv);
                        }
                        TrayMessage(hDlg, NIM_MODIFY, ID_TRAY1, hIcon1);
                    }
                }
            }
            return FALSE;

        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpMemory);
                }
            }
            break;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_DATABASE:
                        {
                            LPTSTR pszText = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(FILE_LIST));
                            if (pszText == NULL)
                            {
                                SHShowOutOfMemory(hDlg, 0);
                                return FALSE;
                            }
                            pszText[0] = TEXT('\0');
                            PFILE_LIST pFileList = (PFILE_LIST)LocalAlloc(LPTR, sizeof(FILE_LIST));
                            if (pFileList == NULL)
                            {
                                LocalFree((HLOCAL)pszText);
                                SHShowOutOfMemory(hDlg, 0);
                                return FALSE;
                            }
                            TCHAR szTitle[128];
                            szTitle[0] = TEXT('\0');
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_DBASE : IDS_ENG_DBASE, szTitle, _countof(szTitle));
                            pFileList->dwSize1 = pFileList->dwSize2 = pFileList->dwSize3 =
                            pFileList->dwSize4 = pFileList->dwSize5 = 0;
                            __try { EnumDBase(pFileList); }
                            __except (EXCEPTION_EXECUTE_HANDLER) { ; }
                            if (pFileList->dwSize1 || pFileList->dwSize2 || pFileList->dwSize3 ||
                            pFileList->dwSize4 || pFileList->dwSize5)
                            {
                                TCHAR szSize[32];
                                if (pFileList->dwSize1)
                                {
                                    ByteToString(pFileList->dwSize1, szSize);
                                    _tcscat(pszText, pFileList->szFileName1);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize2)
                                {
                                    ByteToString(pFileList->dwSize2, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName2);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize3)
                                {
                                    ByteToString(pFileList->dwSize3, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName3);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize4)
                                {
                                    ByteToString(pFileList->dwSize4, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName4);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize5)
                                {
                                    ByteToString(pFileList->dwSize5, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName5);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                MessageBox(hDlg, pszText, szTitle, MB_OK);
                            }
                            else
                            {
                                LoadString(g_hInstance, g_bGerUI ? IDP_GER_NOTFOUND : IDP_ENG_NOTFOUND,
                                    pszText, 128);
                                MessageBox(hDlg, pszText, szTitle, MB_OK | MB_ICONEXCLAMATION);
                            }
                            LocalFree((HLOCAL)pFileList);
                            LocalFree((HLOCAL)pszText);
                        }
                        return FALSE;

                    case IDC_SEARCH:
                        {
                            LPTSTR pszText = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(FILE_LIST));
                            if (pszText == NULL)
                            {
                                SHShowOutOfMemory(hDlg, 0);
                                return FALSE;
                            }
                            pszText[0] = TEXT('\0');
                            PFILE_LIST pFileList = (PFILE_LIST)LocalAlloc(LPTR, sizeof(FILE_LIST));
                            if (pFileList == NULL)
                            {
                                LocalFree((HLOCAL)pszText);
                                SHShowOutOfMemory(hDlg, 0);
                                return FALSE;
                            }
                            TCHAR szTitle[128];
                            szTitle[0] = TEXT('\0');
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_FILES : IDS_ENG_FILES, szTitle, _countof(szTitle));
                            pFileList->dwSize1 = pFileList->dwSize2 = pFileList->dwSize3 =
                            pFileList->dwSize4 = pFileList->dwSize5 = 0;
                            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                            SearchDirectory(TEXT("\\*"), pFileList);
                            SetCursor(hOldCursor);
                            if (pFileList->dwSize1 || pFileList->dwSize2 || pFileList->dwSize3 ||
                            pFileList->dwSize4 || pFileList->dwSize5)
                            {
                                TCHAR szSize[32];
                                if (pFileList->dwSize1)
                                {
                                    ByteToString(pFileList->dwSize1, szSize);
                                    _tcscat(pszText, pFileList->szFileName1);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize2)
                                {
                                    ByteToString(pFileList->dwSize2, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName2);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize3)
                                {
                                    ByteToString(pFileList->dwSize3, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName3);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize4)
                                {
                                    ByteToString(pFileList->dwSize4, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName4);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                if (pFileList->dwSize5)
                                {
                                    ByteToString(pFileList->dwSize5, szSize);
                                    if (pszText[0] != TEXT('\0'))
                                        _tcscat(pszText, TEXT("\r\n"));
                                    _tcscat(pszText, pFileList->szFileName5);
                                    _tcscat(pszText, TEXT(" ("));
                                    _tcscat(pszText, szSize);
                                    _tcscat(pszText, TEXT(")"));
                                }
                                MessageBox(hDlg, pszText, szTitle, MB_OK);
                            }
                            else
                            {
                                LoadString(g_hInstance, g_bGerUI ? IDP_GER_NOTFOUND : IDP_ENG_NOTFOUND,
                                    pszText, 128);
                                MessageBox(hDlg, pszText, szTitle, MB_OK | MB_ICONEXCLAMATION);
                            }
                            LocalFree((HLOCAL)pFileList);
                            LocalFree((HLOCAL)pszText);
                        }
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;
                }
            }
            break;

        case WM_APP_ADDICON:
            if (hIcon1 != NULL)
                DestroyIcon(hIcon1);
            if (g_bStaticIcons)
                hIcon1 = CreateTrayIcon3(cPcMemOld);
            else
                hIcon1 = CreateTrayIcon1(cPcMemOld, cPcObjOld, cPcMemDivOld);
            TrayMessage(hDlg, NIM_ADD, ID_TRAY1, hIcon1);
            return TRUE;

        case WM_APP_DELICON:
            TrayMessage(hDlg, NIM_DELETE, ID_TRAY1, hIcon1);
            if (hIcon1 != NULL)
            {
                DestroyIcon(hIcon1);
                hIcon1 = NULL;
            }
            return TRUE;

        case WM_SYSCOLORCHANGE:
            {
                if (!g_bNoMemTrayIcon)
                {
                    if (g_bStaticIcons)
                    {
                        if (hIcon1 != NULL) DestroyIcon(hIcon1);
                        hIcon1 = CreateTrayIcon3(cPcMemOld);
                    }
                    else if (g_bPocketPC)
                    {
                        HICON hIconNew = CreateTrayIcon1(cPcMemOld, cPcObjOld, cPcMemDivOld);
                        if (hIcon1 != NULL) DestroyIcon(hIcon1);
                        hIcon1 = hIconNew;
                    }
                    else
                    {
                        if (hIcon1 != NULL) DestroyIcon(hIcon1);
                        hIcon1 = CreateTrayIcon1(cPcMemOld, cPcObjOld, cPcMemDivOld);
                    }
                    TrayMessage(hDlg, NIM_MODIFY, ID_TRAY1, hIcon1);
                }
            }
            return TRUE;

        case WM_INITDIALOG:
            {
                g_hwndPropPage2 = hDlg;
                ((LPPROPSHEETPAGE)lParam)->lParam = (LPARAM)hDlg;
                SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_MEM), PBM_SETPOS, 0, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_OBJ), PBM_SETPOS, 0, 0);
                if (!g_bNoMemTrayIcon)
                {
                    if (g_bStaticIcons)
                        hIcon1 = CreateTrayIcon3();
                    else
                        hIcon1 = CreateTrayIcon1();
                    TrayMessage(hDlg, NIM_ADD, ID_TRAY1, hIcon1);
                }
                SetForegroundWindow(g_bPocketPC ? (HWND)(((DWORD)hDlg) | 0x01) : hDlg);
                SetTimer(hDlg, ID_TIMER1, TIMER1_PERIOD, NULL);
                PostMessage(hDlg, WM_TIMER, ID_TIMER1, 0);
            }
            return TRUE;

        case WM_PSPCB_RELEASE:
            {
                KillTimer(hDlg, ID_TIMER1);
                if (!g_bNoMemTrayIcon)
                {
                    TrayMessage(hDlg, NIM_DELETE, ID_TRAY1, hIcon1);
                    if (hIcon1 != NULL)
                    {
                        DestroyIcon(hIcon1);
                        hIcon1 = NULL;
                    }
                }
                g_hwndPropPage2 = NULL;
            }
            return TRUE;

        case TRAY_NOTIFYICON:
            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    if (wParam == ID_TRAY1)
                    {
                        PropSheet_SetCurSel(g_hwndPropSheet, NULL, 1);
                        if (g_bHideState)
                        {
                            ShowWindow(g_hwndPropSheet, SW_SHOW);
                            if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_SHOW);
                            g_bHideState = FALSE;
                        }
                        SetForegroundWindow(g_bPocketPC ? (HWND)(((DWORD)g_hwndPropSheet) | 0x01) : g_hwndPropSheet);
                    }
            }
            return TRUE;
    }

    return FALSE;
}


BOOL CALLBACK ResInfoDlgProc3(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static DWORD dwCount = 0;
    static DWORD dwPage = 1;

    switch(msg)
    {
        case WM_TIMER:
            if (IsWindowVisible(g_hwndPropSheet) &&
                GetWindow(hDlg, GW_HWNDFIRST) == hDlg)
                dwCount = UpdateStorageMedia(hDlg, dwPage, FALSE);
            return FALSE;

        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;

                    case PSN_SETACTIVE:
                        dwCount = UpdateStorageMedia(hDlg, dwPage);
                        return FALSE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpStorage);
                }
            }
            break;

        case WM_DEVICECHANGE:
            {
                if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
                    dwCount = UpdateStorageMedia(hDlg, dwPage);
            }
            return TRUE;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_NEXT:
                        dwPage++;
                        if ((2 * dwPage) > (dwCount + 1))
                            dwPage = 1;
                        dwCount = UpdateStorageMedia(hDlg, dwPage);
                        return FALSE;

                    case IDC_PC_CARDS:
                        {
                            TCHAR szTitle[128];
                            TCHAR szText[128];
                            szTitle[0] = TEXT('\0');
                            szText[0] = TEXT('\0');
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_PCMCIA : IDS_ENG_PCMCIA, szTitle, _countof(szTitle));
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_UNKNOWN : IDS_ENG_UNKNOWN, szText, _countof(szText));

                            if (g_hLibCoreDll == NULL)
                            {
                                MessageBox(hDlg, szText, szTitle, MB_OK | MB_ICONEXCLAMATION);
                                return FALSE;
                            }

                            DWORD (*fnEnumPnpIds)(LPTSTR, LPDWORD);
                            fnEnumPnpIds = (DWORD(*)(LPTSTR, LPDWORD))GetProcAddress(g_hLibCoreDll, g_szEnumPnpIds);
                            if (fnEnumPnpIds == NULL)
                            {
                                MessageBox(hDlg, szText, szTitle, MB_OK | MB_ICONEXCLAMATION);
                                return FALSE;
                            }

                            DWORD dwSize = MAX_PATH;
                            LPTSTR lpszPnpList = (LPTSTR)LocalAlloc(LPTR, dwSize);
                            if (lpszPnpList == NULL)
                            {
                                SHShowOutOfMemory(hDlg, 0);
                                return FALSE;
                            }
                            LPTSTR lpszMyList = (LPTSTR)LocalAlloc(LPTR, dwSize);
                            if (lpszMyList == NULL)
                            {
                                LocalFree((HLOCAL)lpszPnpList);
                                SHShowOutOfMemory(hDlg, 0);
                                return FALSE;
                            }

                            DWORD dwRet = ERROR_INVALID_FUNCTION;
                            __try { dwRet = fnEnumPnpIds(lpszPnpList, &dwSize); }
                            __except (EXCEPTION_EXECUTE_HANDLER) { ; }
                            if (dwRet == ERROR_MORE_DATA || dwRet == ERROR_INSUFFICIENT_BUFFER)
                            {
                                LocalFree((HLOCAL)lpszMyList);
                                LocalFree((HLOCAL)lpszPnpList);
                                lpszPnpList = (LPTSTR)LocalAlloc(LPTR, dwSize);
                                if (lpszPnpList == NULL)
                                {
                                    SHShowOutOfMemory(hDlg, 0);
                                    return FALSE;
                                }
                                lpszMyList = (LPTSTR)LocalAlloc(LPTR, dwSize);
                                if (lpszMyList == NULL)
                                {
                                    LocalFree((HLOCAL)lpszPnpList);
                                    SHShowOutOfMemory(hDlg, 0);
                                    return FALSE;
                                }
                                __try { dwRet = fnEnumPnpIds(lpszPnpList, &dwSize); }
                                __except (EXCEPTION_EXECUTE_HANDLER) { ; }
                            }

                            if (dwRet != ERROR_SUCCESS || lpszPnpList[0] == TEXT('\0'))
                            {
                                LocalFree((HLOCAL)lpszMyList);
                                LocalFree((HLOCAL)lpszPnpList);
                                if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_NOPCCARD : IDP_ENG_NOPCCARD,
                                    szText, _countof(szText)) > 0)
                                    MessageBox(hDlg, szText, szTitle, MB_OK | MB_ICONEXCLAMATION);
                                return FALSE;
                            }

                            LPTSTR lpsz = lpszPnpList;
                            int nLen = _tcslen(lpsz);
                            if (nLen > 5)
                                _tcsncpy(lpszMyList, lpsz, nLen-5);
                            else
                                _tcscpy(lpszMyList, lpsz);
                            lpsz += nLen + 1;

                            while (*lpsz)
                            {
                                nLen = _tcslen(lpsz);
                                if (nLen > 5)
                                {
                                    _tcscat(lpszMyList, TEXT("\r\n"));
                                    _tcsncat(lpszMyList, lpsz, nLen-5);
                                }
                                else
                                {
                                    _tcscat(lpszMyList, TEXT("\n"));
                                    _tcscat(lpszMyList, lpsz);
                                }
                                lpsz += nLen + 1;
                            }

                            MessageBox(hDlg, lpszMyList, szTitle, MB_OK | MB_ICONINFORMATION);

                            LocalFree((HLOCAL)lpszMyList);
                            LocalFree((HLOCAL)lpszPnpList);
                        }
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;
                }
            }
            break;

        case WM_INITDIALOG:
            {
                g_hwndPropPage3 = hDlg;
                ((LPPROPSHEETPAGE)lParam)->lParam = (LPARAM)hDlg;
                SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_CARD1), PBM_SETPOS, 0, 0);
                SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_CARD2), PBM_SETPOS, 0, 0);
            }
            return TRUE;

        case WM_PSPCB_RELEASE:
            g_hwndPropPage3 = NULL;
            return TRUE;
    }

    return FALSE;
}


BOOL CALLBACK ResInfoDlgProc4(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL bProcessList = FALSE;
    static int nOldTopIndex = 0;
    static int nOldCurSel = 0;

    switch(msg)
    {
        case WM_TIMER:
            if (IsWindowVisible(g_hwndPropSheet) && GetWindow(hDlg, GW_HWNDFIRST) == hDlg)
                bProcessList ? UpdateTaskList(hDlg) : UpdateWndList(hDlg);
            return FALSE;

        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;

                    case PSN_SETACTIVE:
                        bProcessList ? UpdateTaskList(hDlg) : UpdateWndList(hDlg);
                        return FALSE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpTasks);
                }
            }
            break;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_END_TASK:
                        {
                            TCHAR szText[256];
                            TCHAR szName[MAX_PATH];
                            DWORD dwProcessId = 0;

                            HWND hwndTaskList = GetDlgItem(hDlg, IDC_TASKLIST);
                            if (hwndTaskList == NULL)
                                return FALSE;

                            int nIndex = ListBox_GetCurSel(hwndTaskList);
                            if (nIndex == LB_ERR)
                                return FALSE;

                            if (bProcessList)
                            {
                                dwProcessId = ListBox_GetItemData(hwndTaskList, nIndex);
                                if (dwProcessId == LB_ERR)
                                    return FALSE;
                            }
                            else
                            {
                                HWND hwndMain = (HWND)ListBox_GetItemData(hwndTaskList, nIndex);
                                if ((DWORD)hwndMain == LB_ERR)
                                    return FALSE;

                                if (!IsWindow(hwndMain))
                                {
                                    if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_PROCESS : IDP_ENG_PROCESS,
                                        szText, _countof(szText)) > 0)
                                        MessageBox(hDlg, szText, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
                                    UpdateWndList(hDlg);
                                    return FALSE;
                                }

                                if (GetClassName(hwndMain, szName, _countof(szName)) > 0)
                                {
                                    for (int i=0; g_aszClassAll[i]; i++)
                                    {
                                        if (_tcscmp(szName, g_aszClassAll[i]) == 0)
                                        {
                                            if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_SYSTASK : IDP_ENG_SYSTASK,
                                                szText, _countof(szText)) > 0)
                                                MessageBox(hDlg, szText, szName, MB_OK | MB_ICONEXCLAMATION);
                                            return FALSE;
                                        }
                                    }
                                }

                                PostMessage(hwndMain, WM_CLOSE, 0, 0);
                                if (hwndMain == g_hwndPropSheet)
                                    return FALSE;

                                HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                                Sleep(50);
                                for (int i=0; IsWindow(hwndMain) && i<4; i++)
                                    Sleep(1250);
                                SetCursor(hOldCursor);

                                if (!IsWindow(hwndMain))
                                {
                                    UpdateWndList(hDlg);
                                    return FALSE;
                                }
                                
                                GetWindowThreadProcessId(hwndMain, &dwProcessId);
                            }

                            HANDLE hProcess = OpenProcess(0, FALSE, dwProcessId);
                            if (hProcess == NULL && GetLastError() != ERROR_ACCESS_DENIED)
                            {
                                if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_PROCESS : IDP_ENG_PROCESS,
                                    szText, _countof(szText)) > 0)
                                    MessageBox(hDlg, szText, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
                                bProcessList ? UpdateTaskList(hDlg) : UpdateWndList(hDlg);
                                return FALSE;
                            }

                            if (!GetModuleFileName(hProcess != NULL ? (HMODULE)hProcess : (HMODULE)dwProcessId,
                                szName, MAX_PATH) && bProcessList)
                                ListBox_GetText(hwndTaskList, nIndex, szName);

                            if (hProcess != NULL)
                                CloseHandle(hProcess);

                            LPTSTR pszExeFile = szName;
                            if (_tcsstr(szName, TEXT("\\")))
                            {
                                pszExeFile = _tcsrchr(szName, TEXT('\\'));
                                pszExeFile++;
                            }

                            for (int i=0; g_aszProcess[i]; i++)
                            {
                                if (_tcsicmp(pszExeFile, g_aszProcess[i]) == 0)
                                {
                                    if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_SYSTASK : IDP_ENG_SYSTASK,
                                        szText, _countof(szText)) > 0)
                                        MessageBox(hDlg, szText, pszExeFile, MB_OK | MB_ICONEXCLAMATION);
                                    return FALSE;
                                }
                            }

                            int nYesNo = IDYES;
                            if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_TERMINATE : IDP_ENG_TERMINATE,
                                szText, _countof(szText)) > 0)
                                nYesNo = MessageBox(hDlg, szText, pszExeFile, MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2);

                            if (nYesNo != IDYES)
                                return FALSE;

                            hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
                            if (hProcess != NULL)
                            {
                                TerminateProcess(hProcess, (DWORD)-1);
                                CloseHandle(hProcess);
                            }
                            else if (LoadString(g_hInstance, (GetLastError() == ERROR_ACCESS_DENIED) ?
                                (g_bGerUI ? IDP_GER_ACCESS : IDP_ENG_ACCESS) :
                                (g_bGerUI ? IDP_GER_PROCESS : IDP_ENG_PROCESS),
                                szText, _countof(szText)) > 0)
                                MessageBox(hDlg, szText, pszExeFile, MB_OK | MB_ICONEXCLAMATION);

                            bProcessList ? UpdateTaskList(hDlg) : UpdateWndList(hDlg);
                        }
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;

                    case IDC_TASKS:
                        {
                            TCHAR szText[64];
                            HWND hwndTaskList = GetDlgItem(hDlg, IDC_TASKLIST);
                            int nTopIndex = ListBox_GetTopIndex(hwndTaskList);
                            int nCurSel = ListBox_GetCurSel(hwndTaskList);
                            if (bProcessList)
                            {
                                bProcessList = FALSE;
                                UpdateWndList(hDlg, TRUE, nOldTopIndex, nOldCurSel);
                                nOldTopIndex = nTopIndex; nOldCurSel = nCurSel;
                                if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_BTN_PROC : IDS_ENG_BTN_PROC,
                                    szText, _countof(szText)) > 0)
                                    SetDlgItemText(hDlg, IDC_TASKS, szText);
                            }
                            else
                            {
                                bProcessList = TRUE;
                                UpdateTaskList(hDlg, TRUE, nOldTopIndex, nOldCurSel);
                                nOldTopIndex = nTopIndex; nOldCurSel = nCurSel;
                                if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_BTN_WND : IDS_ENG_BTN_WND,
                                    szText, _countof(szText)) > 0)
                                    SetDlgItemText(hDlg, IDC_TASKS, szText);
                            }
                        }
                        return FALSE;

                    case IDC_TASKLIST:
                        if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)
                            bProcessList ? ShowModuleList(hDlg) : ShowTaskWindow(hDlg);
                        return FALSE;
                }
            }
            break;

        case WM_INITDIALOG:
            {
                g_hwndPropPage4 = hDlg;
                ((LPPROPSHEETPAGE)lParam)->lParam = (LPARAM)hDlg;
                bProcessList = FALSE;
                TCHAR szText[64];
                if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_BTN_PROC : IDS_ENG_BTN_PROC,
                    szText, _countof(szText)) > 0)
                    SetDlgItemText(hDlg, IDC_TASKS, szText);
            }
            return TRUE;

        case WM_PSPCB_RELEASE:
            g_hwndPropPage4 = NULL;
            return TRUE;
    }

    return FALSE;
}


BOOL CALLBACK ResInfoDlgProc5(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;

                    case PSN_SETACTIVE:
                        UpdateSystemInfo(hDlg);
                        return FALSE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpSystem);
                }
            }
            break;

        case WM_DEVICECHANGE:
            {
                if (wParam == DBT_MONITORCHANGE)
                    UpdateSystemInfo(hDlg);
            }
            return TRUE;

        case WM_SETTINGCHANGE:
            {
                if (wParam == SETTINGCHANGE_RESET)
                    UpdateSystemInfo(hDlg);
            }
            return FALSE;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_NETWORK:
                        {
                            TCHAR szText[128];
                            szText[0] = TEXT('\0');
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_UNKNOWN : IDS_ENG_UNKNOWN, szText, _countof(szText));

                            WSADATA wsaData;
                            if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
                            {
                                MessageBox(hDlg, szText, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
                                return FALSE;
                            }

                            char chszHostName[64];
                            if (gethostname(chszHostName, sizeof(chszHostName)) == SOCKET_ERROR)
                            {
                                MessageBox(hDlg, szText, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
                                WSACleanup();
                                return FALSE;
                            }

                            HOSTENT* pHostent;
                            if ((pHostent = gethostbyname(chszHostName)) == NULL)
                            {
                                MessageBox(hDlg, szText, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
                                WSACleanup();
                                return FALSE;
                            }

                            TCHAR szHostName[80];
                            TCHAR szIPAddress[64];
                            TCHAR szIPAddressList[256];
                            szIPAddressList[0] = TEXT('\0');
                            mbstowcs(szHostName, chszHostName, _countof(szHostName));

                            for (int i=0; pHostent->h_addr_list[i] != 0; ++i)
                            {
                                IN_ADDR inetAddr;
                                memcpy((void*)&inetAddr, (void*)pHostent->h_addr_list[i], sizeof(IN_ADDR));
                                
                                char* pcDottedIP = NULL;
                                if ((pcDottedIP = inet_ntoa(inetAddr)) != NULL)
                                {
                                    mbstowcs(szIPAddress, pcDottedIP, _countof(szIPAddress));
                                    if (szIPAddressList[0] != TEXT('\0'))
                                        _tcscat(szIPAddressList, TEXT("\r\n"));
                                    _tcscat(szIPAddressList, szIPAddress);
                                }
                            }

                            if (szIPAddressList[0] != TEXT('\0') && _tcsncmp(szIPAddressList, g_szLocalHost, 4))
                                MessageBox(hDlg, szIPAddressList, szHostName, MB_OK | MB_ICONINFORMATION);
                            else
                            {
                                LoadString(g_hInstance, g_bGerUI ? IDP_GER_NETWORK : IDP_ENG_NETWORK,
                                    szIPAddressList, _countof(szIPAddressList));
                                MessageBox(hDlg, szIPAddressList, szHostName, MB_OK | MB_ICONEXCLAMATION);
                            }

                            WSACleanup();
                        }
                        return FALSE;

                    case IDC_REBOOT:
                        {
                            TCHAR szText[256];
                            TCHAR szTitle[128];
                            szText[0] = TEXT('\0'); szTitle[0] = TEXT('\0');
                            LoadString(g_hInstance, g_bGerUI ? IDS_GER_RESET : IDS_ENG_RESET, szTitle, _countof(szTitle));
                            LoadString(g_hInstance, g_bGerUI ? IDP_GER_REBOOT : IDP_ENG_REBOOT, szText, _countof(szText));

                            int nYesNo = MessageBox(hDlg, szText, szTitle,
                                MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2);
                            if (nYesNo != IDYES)
                                return FALSE;

                            OSVERSIONINFO osvi;
                            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                            GetVersionEx(&osvi);

                            if ((osvi.dwMajorVersion > 5) ||
                                (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1))
                            {
                                if (g_hLibAygshell != NULL)
                                {
                                    BOOL (*fnExitWindowsEx)(UINT, DWORD);
                                    fnExitWindowsEx = (BOOL(*)(UINT, DWORD))GetProcAddress(g_hLibAygshell,
                                        g_szExitWindowsEx);
                                    if (fnExitWindowsEx != NULL && fnExitWindowsEx(EWX_REBOOT, 0))
                                        return FALSE;
                                }
                            }

                            if (osvi.dwMajorVersion >= 4)
                            {
                                if (g_hLibCoreDll != NULL)
                                {
                                    DWORD (*fnSetSystemPowerState)(LPCWSTR, DWORD, DWORD);
                                    fnSetSystemPowerState = (DWORD(*)(LPCWSTR, DWORD, DWORD))GetProcAddress(g_hLibCoreDll,
                                        g_szSetSystemPowerState);
                                    if (fnSetSystemPowerState != NULL &&
                                        fnSetSystemPowerState(NULL, POWER_STATE_RESET, POWER_FORCE) == ERROR_SUCCESS)
                                        return FALSE;
                                }
                            }
                            
                            if (g_hLibCoreDll != NULL)
                            {
                                BOOL (*fnKernelIoControl)(DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD);
                                fnKernelIoControl = (BOOL(*)(DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD))GetProcAddress(g_hLibCoreDll,
                                    g_szKernelIoControl);
                                if (fnKernelIoControl != NULL)
                                {
                                    __try { fnKernelIoControl(IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL); }
                                    __except (EXCEPTION_EXECUTE_HANDLER) { ; }
                                }
                            }

                            Sleep(0);
                            if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_FAILBOOT : IDP_ENG_FAILBOOT,
                                szText, _countof(szText)) > 0)
                                MessageBox(hDlg, szText, szTitle, MB_OK | MB_ICONEXCLAMATION);
                        }
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;
                }
            }
            break;
    }

    return FALSE;
}


BOOL CALLBACK ResInfoDlgProc6(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static COLORREF crCustColors[16] =
    {
        { 0x00000000 },
        { 0x00000080 },
        { 0x00008080 },
        { 0x00008000 },
        { 0x00808000 },
        { 0x00800000 },
        { 0x00800080 },
        { 0x00808080 },
        { 0x00C0C0C0 },
        { 0x000000FF },
        { 0x0000FFFF },
        { 0x0000FF00 },
        { 0x00FFFF00 },
        { 0x00FF0000 },
        { 0x00FF00FF },
        { 0x00FFFFFF }
    };

    switch(msg)
    {
        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        {
                            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                                (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                            SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        }
                        return TRUE;

                    case PSN_SETACTIVE:
                        SendMessage(GetDlgItem(hDlg, IDC_OPT_RESET), BM_SETCHECK, g_bResetRuntimes ? BST_CHECKED : BST_UNCHECKED, 0);
                        SendMessage(GetDlgItem(hDlg, IDC_OPT_BATICON), BM_SETCHECK, g_bNoBatTrayIcon ? BST_UNCHECKED : BST_CHECKED, 0);
                        SendMessage(GetDlgItem(hDlg, IDC_OPT_MEMICON), BM_SETCHECK, g_bNoMemTrayIcon ? BST_UNCHECKED : BST_CHECKED, 0);
                        SendMessage(GetDlgItem(hDlg, IDC_OPT_CPUICON), BM_SETCHECK, g_bNoCpuTrayIcon ? BST_UNCHECKED : BST_CHECKED, 0);
                        SendMessage(GetDlgItem(hDlg, IDC_OPT_STATICON), BM_SETCHECK, g_bStaticIcons ? BST_CHECKED : BST_UNCHECKED, 0);
                        SendMessage(GetDlgItem(hDlg, IDC_OPT_COLOR), BM_SETCHECK, g_bAlternativeColor ? BST_CHECKED : BST_UNCHECKED, 0);
                        EnableWindow(GetDlgItem(hDlg, IDC_OPT_STATICON), !(g_bNoBatTrayIcon && g_bNoMemTrayIcon));
                        EnableWindow(GetDlgItem(hDlg, IDC_OPT_COLOR), !g_bNoMemTrayIcon || (g_bStaticIcons && !g_bNoBatTrayIcon));
                        EnableWindow(GetDlgItem(hDlg, IDC_COLOR), !(g_bStaticIcons || (g_bNoBatTrayIcon && g_bNoMemTrayIcon)));
                        return FALSE;

                    case PSN_KILLACTIVE:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLY, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        return FALSE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpSetting);
                }
            }
            break;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_APPLY:
                        {
                            BOOL bChecked = (SendMessage(GetDlgItem(hDlg, IDC_OPT_RESET), BM_GETCHECK, 0, 0) == BST_CHECKED);
                            if ((!bChecked && g_bResetRuntimes) || (bChecked && !g_bResetRuntimes))
                            {
                                g_bResetRuntimes = bChecked;
                                SaveSetting(g_szAutoReset, (DWORD)g_bResetRuntimes);
                            }

                            bChecked = (SendMessage(GetDlgItem(hDlg, IDC_OPT_BATICON), BM_GETCHECK, 0, 0) == BST_UNCHECKED);
                            if ((!bChecked && g_bNoBatTrayIcon) || (bChecked && !g_bNoBatTrayIcon))
                            {
                                g_bNoBatTrayIcon = bChecked;
                                SaveSetting(g_szNoBatIcon, (DWORD)g_bNoBatTrayIcon);
                                if (g_hwndPropPage1 != NULL)
                                    PostMessage(g_hwndPropPage1, g_bNoBatTrayIcon ? WM_APP_DELICON : WM_APP_ADDICON, 0, 0);
                            }

                            bChecked = (SendMessage(GetDlgItem(hDlg, IDC_OPT_MEMICON), BM_GETCHECK, 0, 0) == BST_UNCHECKED);
                            if ((!bChecked && g_bNoMemTrayIcon) || (bChecked && !g_bNoMemTrayIcon))
                            {
                                g_bNoMemTrayIcon = bChecked;
                                SaveSetting(g_szNoMemIcon, (DWORD)g_bNoMemTrayIcon);
                                if (g_hwndPropPage2 != NULL)
                                    PostMessage(g_hwndPropPage2, g_bNoMemTrayIcon ? WM_APP_DELICON : WM_APP_ADDICON, 0, 0);
                            }

                            bChecked = (SendMessage(GetDlgItem(hDlg, IDC_OPT_CPUICON), BM_GETCHECK, 0, 0) == BST_UNCHECKED);
                            if ((!bChecked && g_bNoCpuTrayIcon) || (bChecked && !g_bNoCpuTrayIcon))
                            {
                                g_bNoCpuTrayIcon = bChecked;
                                SaveSetting(g_szNoCpuIcon, (DWORD)g_bNoCpuTrayIcon);
                                if (g_hwndPropPage8 != NULL)
                                    PostMessage(g_hwndPropPage8, g_bNoCpuTrayIcon ? WM_APP_DELICON : WM_APP_ADDICON, 0, 0);
                            }

                            bChecked = (SendMessage(GetDlgItem(hDlg, IDC_OPT_STATICON), BM_GETCHECK, 0, 0) == BST_CHECKED);
                            if ((!bChecked && g_bStaticIcons) || (bChecked && !g_bStaticIcons))
                            {
                                g_bStaticIcons = bChecked;
                                SaveSetting(g_szStaticIco, (DWORD)g_bStaticIcons);
                                if (g_hwndPropPage8 != NULL)
                                    PostMessage(g_hwndPropPage8, WM_SYSCOLORCHANGE, 0, 0);
                                if (g_hwndPropPage2 != NULL)
                                    PostMessage(g_hwndPropPage2, WM_SYSCOLORCHANGE, 0, 0);
                                if (g_hwndPropPage1 != NULL)
                                    PostMessage(g_hwndPropPage1, WM_SYSCOLORCHANGE, 0, 0);
                            }

                            bChecked = (SendMessage(GetDlgItem(hDlg, IDC_OPT_COLOR), BM_GETCHECK, 0, 0) == BST_CHECKED);
                            if ((!bChecked && g_bAlternativeColor) || (bChecked && !g_bAlternativeColor))
                            {
                                g_bAlternativeColor = bChecked;
                                SaveSetting(g_szAltColor, (DWORD)g_bAlternativeColor);
                                if (g_hwndPropPage8 != NULL)
                                    PostMessage(g_hwndPropPage8, WM_SYSCOLORCHANGE, 0, 0);
                                if (g_hwndPropPage2 != NULL)
                                    PostMessage(g_hwndPropPage2, WM_SYSCOLORCHANGE, 0, 0);
                                if (g_hwndPropPage1 != NULL)
                                    PostMessage(g_hwndPropPage1, WM_SYSCOLORCHANGE, 0, 0);
                            }

                            EnableWindow(GetDlgItem(hDlg, IDC_OPT_STATICON), !(g_bNoBatTrayIcon && g_bNoMemTrayIcon & g_bNoCpuTrayIcon));
                            EnableWindow(GetDlgItem(hDlg, IDC_OPT_COLOR), !g_bNoMemTrayIcon || !g_bNoCpuTrayIcon || (g_bStaticIcons && !g_bNoBatTrayIcon));
                            EnableWindow(GetDlgItem(hDlg, IDC_COLOR), !(g_bStaticIcons || (g_bNoBatTrayIcon && g_bNoMemTrayIcon && g_bNoCpuTrayIcon)));
                        }
                        return FALSE;

                    case IDC_COLOR:
                        {
                            HINSTANCE hLibCommDlg = LoadLibrary(g_szCommDlg);
                            if (hLibCommDlg == NULL)
                                MessageBox(hDlg, g_szCommDlg, NULL, MB_OK | MB_ICONEXCLAMATION);
                            else
                            {
                                CHOOSECOLOR cc;
                                memset(&cc, 0, sizeof(cc));
                                cc.lStructSize = sizeof(CHOOSECOLOR);
                                cc.hwndOwner = hDlg;
                                cc.hInstance = g_hInstance;
                                cc.rgbResult = g_crIconColor;
                                cc.lpCustColors = crCustColors;
                                cc.Flags = CC_ANYCOLOR | CC_RGBINIT;
                                BOOL (*ChooseColor)(LPCHOOSECOLOR);
                                ChooseColor = (BOOL(*)(LPCHOOSECOLOR))GetProcAddress(hLibCommDlg, g_szChooseColor);
                                if (ChooseColor == NULL)
                                    MessageBox(hDlg, g_szChooseColor, NULL, MB_OK | MB_ICONEXCLAMATION);
                                else if (ChooseColor(&cc))
                                {
                                    g_crIconColor = cc.rgbResult;
                                    SaveSetting(g_szIconColor, (DWORD)g_crIconColor);
                                    if (g_hwndPropPage2 != NULL)
                                        PostMessage(g_hwndPropPage2, WM_SYSCOLORCHANGE, 0, 0);
                                    if (g_hwndPropPage1 != NULL)
                                        PostMessage(g_hwndPropPage1, WM_SYSCOLORCHANGE, 0, 0);
                                }
                                FreeLibrary(hLibCommDlg);
                            }
                        }
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;
                }
            }
            break;
    }

    return FALSE;
}


BOOL CALLBACK ResInfoDlgProc7(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpAbout);
                }
            }
            break;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_HOMEPAGE:
                        {
                            HKEY hKey;
                            LONG lStatus = RegOpenKeyEx(HKEY_CLASSES_ROOT, g_szRegHttp, 0, 0, &hKey);
                            if (lStatus == ERROR_SUCCESS)
                            {
                                DWORD dwType = REG_SZ;  
                                DWORD dwSize = MAX_PATH * sizeof(TCHAR);
                                TCHAR szBrowser[MAX_PATH];
                                lStatus = RegQueryValueEx(hKey, NULL, NULL, &dwType, (LPBYTE)szBrowser, &dwSize);
                                RegCloseKey(hKey);
                                if (lStatus == ERROR_SUCCESS)
                                {
                                    TCHAR *pch;
                                    if (pch = _tcsstr(szBrowser, TEXT("%1")))
                                    {
                                        for (pch--; *pch != TEXT(' '); pch--)
                                            ;
                                        *pch = TEXT('\0');
                                    }

                                    for (dwSize = _tcslen(szBrowser) - 1; dwSize >= 0 && szBrowser[dwSize] == TEXT(' '); dwSize--)
                                        szBrowser[dwSize] = TEXT('\0');

                                    pch = _tcschr(szBrowser, TEXT('\"'));
                                    if (pch != NULL)
                                    {
                                        TCHAR szTemp[2*MAX_PATH];
                                        _tcsncpy(szTemp, pch+1, _countof(szTemp));
                                        LPTSTR pch = _tcschr(szTemp, TEXT('\"'));
                                        if (pch != NULL)
                                            *pch = TEXT('\0');
                                        _tcsncpy(szBrowser, szTemp, _countof(szBrowser));
                                    }

                                    ShowWindow(g_hwndPropSheet, SW_HIDE);
                                    if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                                    g_bHideState = TRUE;

                                    PROCESS_INFORMATION ProcInfo;
                                    if (CreateProcess(szBrowser, g_szWinCESite, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &ProcInfo))
                                    {
                                        CloseHandle(ProcInfo.hThread);
                                        CloseHandle(ProcInfo.hProcess);
                                        return FALSE;
                                    }
                                }
                            }
                            MessageBox(hDlg, g_szWinCESite, g_szTitle, MB_OK);
                        }
                        return FALSE;

                    case IDC_FAQ:
                        ExecPegHelp((LPCTSTR)g_szHelpFAQ);
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;
                }
            }
            break;

        case WM_INITDIALOG:
            {
                TCHAR szRelease[128];
                TCHAR szVersion[32];
                wsprintf(szVersion, TEXT(".%d"), _WIN32_WCE);
                _tcscpy(szRelease, TEXT("[ResInfo"));
#if defined(ARM)
#if defined(THUMB)
                _tcscat(szRelease, TEXT(".THUMB"));
#elif defined(ARMV4I)
                _tcscat(szRelease, TEXT(".ARMV4I"));
#elif defined(ARMV4)
                _tcscat(szRelease, TEXT(".ARMV4"));
#else
                _tcscat(szRelease, TEXT(".ARM"));
#endif
#elif defined(SHx)
#if defined(SH3)
                _tcscat(szRelease, TEXT(".SH3"));
#elif defined(SH4)
                _tcscat(szRelease, TEXT(".SH4"));
#else
                _tcscat(szRelease, TEXT(".SHx"));
#endif
#elif defined(MIPS)
                _tcscat(szRelease, TEXT(".MIPS"));
#elif defined(PPC)
                _tcscat(szRelease, TEXT(".PPC"));
#elif defined(x86)
#if defined(_WIN32_WCE_EMULATION)
                _tcscat(szRelease, TEXT(".x86em"));
#else
                _tcscat(szRelease, TEXT(".x86"));
#endif
#elif defined(CEF)
                _tcscat(szRelease, TEXT(".CEF"));
#endif
                _tcscat(szRelease, szVersion);
#if defined(DEBUG)
                _tcscat(szRelease, TEXT(".Dbg"));
#endif
                _tcscat(szRelease, TEXT("]"));
                SetDlgItemText(hDlg, IDC_APPNAME,   g_szTitle);
                SetDlgItemText(hDlg, IDC_VERSION,   g_szVersion);
                SetDlgItemText(hDlg, IDC_RELEASE,   szRelease);
                SetDlgItemText(hDlg, IDC_COPYRIGHT, g_szCopyright);
                SetDlgItemText(hDlg, IDC_AUTHOR,    g_szAuthor);
                SetDlgItemText(hDlg, IDC_URL,       g_szHomepage);
            }
            return TRUE;
    }

    return FALSE;
}


BOOL CALLBACK ResInfoDlgProc8(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL  bPaused = FALSE;
    static BOOL  bShowDetails = FALSE;
    static UINT  uIconIdOld  = (UINT)-1;
    static BYTE  cFillHeightOld = (BYTE)-1;
    static UINT  uThreads = 0;
    static HICON hiconTray = NULL;
    static BYTE  achHistory[HISTORY_MAX];
    static BYTE  cPercentOld = (BYTE)-1;

    switch(msg)
    {
        case WM_TIMER:
            {
                BYTE cPercent = 0;

                GetCPULoad(&cPercent, &uThreads);

                memmove(achHistory+1, achHistory, HISTORY_MAX-1);
                achHistory[0] = cPercent;

                if (!g_bNoCpuTrayIcon)
                {
                    if (cPercent != cPercentOld)
                    {
                        BYTE cFillHeight = GetCpuLoadFillHeight(cPercent, bPaused);
                        if (g_bStaticIcons)
                        {
                            UINT uIconId = GetTrayIconID(cPercent, bPaused);
                            if (uIconId != uIconIdOld)
                            {
                                if (hiconTray != NULL) DestroyIcon(hiconTray);
                                hiconTray = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE((WORD)uIconId),
                                    IMAGE_ICON, 16, 16, 0);
                                uIconIdOld = uIconId;
                            }
                        }
                        else if (g_bPocketPC)
                        {
                            if (cFillHeight != cFillHeightOld)
                            {
                                HICON hIconNew = CreateCpuTrayIcon(cPercent, bPaused);
                                if (hiconTray != NULL) DestroyIcon(hiconTray);
                                hiconTray = hIconNew;
                                cFillHeightOld = cFillHeight;
                            }
                        }
                        else
                        {
                            if (cFillHeight != cFillHeightOld)
                            {
                                if (hiconTray != NULL) DestroyIcon(hiconTray);
                                hiconTray = CreateCpuTrayIcon(cPercent, bPaused);
                                cFillHeightOld = cFillHeight;
                            }
                        }
                        TrayMessage(hDlg, NIM_MODIFY, ID_TRAY3, hiconTray);

                        cPercentOld = cPercent;
                    }
                }

                if (IsWindowVisible(g_hwndPropSheet) && GetWindow(hDlg, GW_HWNDFIRST) == hDlg)
                {
                    HWND hwndHistory = GetDlgItem(hDlg, IDC_HISTORY);
                    InvalidateRect(hwndHistory, NULL, FALSE);
                    UpdateWindow(hwndHistory);
                }
            }
            break;

        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case PSN_APPLY:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_HIDE, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        SetDlgMsgResult(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;

                    case PSN_KILLACTIVE:
                        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_ACCEPT, wParam),
                            (LPARAM)((LPNMHDR)lParam)->hwndFrom);
                        return FALSE;

                    case PSN_HELP:
                        return ExecPegHelp((LPCTSTR)g_szHelpCpu);
                }
            }
            break;

        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                if (lpdis == NULL || wParam != IDC_HISTORY)
                    return FALSE;

                HDC hdc = lpdis->hDC;
                RECT rc = lpdis->rcItem;
                LONG cx = rc.right - rc.left;
                LONG cy = rc.bottom - rc.top;

                int nDpi = GetDeviceCaps(hdc, LOGPIXELSY);

                BOOL bGrayScale = FALSE;
                COLORREF colref = GetNearestColor(hdc, RGB(255, 0, 0));
                if ((GetRValue(colref) == GetBValue(colref)) && (GetRValue(colref) == GetGValue(colref)))
                    bGrayScale = TRUE;

                COLORREF rgbTextOld = SetTextColor(hdc, bGrayScale ? RGB(255, 255, 255) : RGB(255, 255, 0));
                COLORREF rgbBkOld = SetBkColor(hdc, RGB(0, 0, 0));
                int nBkModeOld = SetBkMode(hdc, TRANSPARENT);
                PatBlt(hdc, rc.left, rc.top, cx, cy, BLACKNESS);

                HPEN hpenOld = NULL;
                HPEN hpen = CreatePen(PS_SOLID, 1, bGrayScale ? RGB(128, 128, 128) : RGB(0, 128, 0));
                if (hpen != NULL)
                    hpenOld = (HPEN)SelectObject(hdc, hpen);

                LONG l, lGridSize;
                POINT pt[2];

                lGridSize = cy / 3;
                for (l = 0; l < cy; l += lGridSize)
                {
                    pt[0].x = rc.left;
                    pt[1].x = rc.right;
                    pt[0].y = pt[1].y = rc.top + l;

                    Polyline(hdc, pt, 2);
                }

                lGridSize = cx / 10;
                for (l = 0; l < cx; l += lGridSize)
                {
                    pt[0].x = pt[1].x = rc.left + l;
                    pt[0].y = rc.top;
                    pt[1].y = rc.bottom;

                    Polyline(hdc, pt, 2);
                }

                if (hpenOld != NULL)
                    SelectObject(hdc, hpenOld);
                if (hpen != NULL)
                    DeleteObject(hpen);

                hpenOld = NULL;
                hpen = CreatePen(PS_SOLID, 1, bGrayScale ? RGB(192, 192, 192) : RGB(0, 255, 0));
                if (hpen != NULL)
                    hpenOld = (HPEN)SelectObject(hdc, hpen);
                HBRUSH hbrOld = NULL;
                HBRUSH hbr = CreateSolidBrush(bGrayScale ? RGB(128, 128, 128) : RGB(0, 128, 0));
                if (hbr != NULL)
                    hbrOld = (HBRUSH)SelectObject(hdc, hbr);

                POINT ptGraph[HISTORY_MAX];
                POINT* pPoint = ptGraph;
                memset(ptGraph, 0, sizeof(ptGraph));

                LONG x = rc.right;
                pPoint->x = x;
                pPoint->y = rc.bottom;
                pPoint++;

                for (l = 0; (l < (HISTORY_MAX-2)) && (x >= (rc.left-1)); l++, x -= 2, pPoint++)
                {
                    pPoint->x = x;
                    pPoint->y = rc.top;
                    if (achHistory[l] > 100)
                        pPoint->y += cy;
                    else
                        pPoint->y += (100 - achHistory[l]) * cy / 100;
                }

                pPoint->x = x;
                pPoint->y = rc.bottom;

                Polygon(hdc, ptGraph, l+2);

                if (hbrOld != NULL)
                    SelectObject(hdc, hbrOld);
                if (hbr != NULL)
                    DeleteObject(hbr);
                if (hpenOld != NULL)
                    SelectObject(hdc, hpenOld);
                if (hpen != NULL)
                    DeleteObject(hpen);

                TCHAR szText[128];
                HFONT hfont = NULL;
                HFONT hfontOld = NULL;
                if (bShowDetails || lpdis->itemState & ODS_SELECTED)
                {
                    hfont = CreateSystemFont(nDpi, 7);
                    if (hfont != NULL)
                        hfontOld = (HFONT)SelectObject(hdc, hfont);

                    _tcscpy(szText, TEXT("0 %"));
                    DrawText(hdc, szText, -1, &rc, DT_LEFT | DT_BOTTOM |
                        DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

                    _tcscpy(szText, TEXT("50 %"));
                    DrawText(hdc, szText, -1, &rc, DT_LEFT | DT_VCENTER |
                        DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

                    _tcscpy(szText, TEXT("100 %"));
                    DrawText(hdc, szText, -1, &rc, DT_LEFT | DT_TOP |
                        DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

                    _stprintf(szText, TEXT("%.1f s/div"), (double)(cx/20 * g_uInterval/1000.0));
                    DrawText(hdc, szText, -1, &rc, DT_RIGHT | DT_TOP |
                        DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

                    if (bPaused)
                    {
                        SYSTEM_INFO si;
                        si.dwNumberOfProcessors = 0;
                        GetSystemInfo(&si);
                        DWORD dwNumCpu = si.dwNumberOfProcessors;
                        if (dwNumCpu > 0 && dwNumCpu <= 32)
                        {
                            _stprintf(szText, TEXT("CPUs: %u"), dwNumCpu);
                            DrawText(hdc, szText, -1, &rc, DT_RIGHT | DT_VCENTER |
                                DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);
                        }
                    }
                    else
                    {
                        _stprintf(szText, TEXT("%u Threads"), uThreads);
                        DrawText(hdc, szText, -1, &rc, DT_RIGHT | DT_VCENTER |
                            DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);
                    }

                    _stprintf(szText, TEXT("\x0394t: %.2f s"), (double)(g_uInterval/1000.0));
                    DrawText(hdc, szText, -1, &rc, DT_RIGHT | DT_BOTTOM |
                        DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

                    if (hfontOld != NULL)
                        SelectObject(hdc, hfontOld);
                    if (hfont != NULL)
                        DeleteObject(hfont);
                }

                hfontOld = NULL;
                hfont = CreateSystemFont(nDpi, 10, FW_BOLD);
                if (hfont != NULL)
                    hfontOld = (HFONT)SelectObject(hdc, hfont);

                if (!bPaused)
                {
                    if (achHistory[0] > 100)
                        _tcscpy(szText, TEXT("? %"));
                    else
                        wsprintf(szText, TEXT("%u %%"), achHistory[0]);
                }
                else
                    LoadString(g_hInstance, g_bGerUI ? IDS_GER_BTN_PAUSE : IDS_ENG_BTN_PAUSE,
                        szText, _countof(szText));

                DrawText(hdc, szText, -1, &rc, DT_CENTER | DT_VCENTER |
                    DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

                if (hfontOld != NULL)
                    SelectObject(hdc, hfontOld);
                if (hfont != NULL)
                    DeleteObject(hfont);

                if (nBkModeOld != 0)
                    SetBkMode(hdc, nBkModeOld);
                if (rgbBkOld != CLR_INVALID)
                    SetBkColor(hdc, rgbBkOld);
                if (rgbTextOld != CLR_INVALID)
                    SetTextColor(hdc, rgbTextOld);

                return TRUE;
            }

        case WM_CTLCOLORBTN:
            if (GetDlgItem(hDlg, IDC_HISTORY) == (HWND)lParam)
                return (BOOL)GetStockObject(BLACK_BRUSH);
            break;

        case WM_APP_ADDICON:
            if (hiconTray != NULL) DestroyIcon(hiconTray);
            if (g_bStaticIcons)
            {
                uIconIdOld = GetTrayIconID(cPercentOld, bPaused);
                hiconTray = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE((WORD)uIconIdOld), IMAGE_ICON, 16, 16, 0);
            }
            else
            {
                cFillHeightOld = GetCpuLoadFillHeight(cPercentOld, bPaused);
                hiconTray = CreateCpuTrayIcon(cPercentOld, bPaused);
            }
            TrayMessage(hDlg, NIM_ADD, ID_TRAY3, hiconTray);
            return TRUE;

        case WM_APP_DELICON:
            TrayMessage(hDlg, NIM_DELETE, ID_TRAY3, hiconTray);
            if (hiconTray != NULL)
            {
                DestroyIcon(hiconTray);
                hiconTray = NULL;
            }
            return TRUE;

        case WM_SYSCOLORCHANGE:
            {
                PostMessage(GetDlgItem(hDlg, IDC_SPEED), msg, wParam, lParam);
                
                if (!g_bNoCpuTrayIcon)
                {
                    if (g_bStaticIcons)
                    {
                        if (hiconTray != NULL) DestroyIcon(hiconTray);
                        uIconIdOld = GetTrayIconID(cPercentOld, bPaused);
                        hiconTray = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE((WORD)uIconIdOld), IMAGE_ICON, 16, 16, 0);
                    }
                    else if (g_bPocketPC)
                    {
                        cFillHeightOld = GetCpuLoadFillHeight(cPercentOld, bPaused);
                        HICON hIconNew = CreateCpuTrayIcon(cPercentOld, bPaused);
                        if (hiconTray != NULL) DestroyIcon(hiconTray);
                        hiconTray = hIconNew;
                    }
                    else
                    {
                        cFillHeightOld = GetCpuLoadFillHeight(cPercentOld, bPaused);
                        if (hiconTray != NULL) DestroyIcon(hiconTray);
                        hiconTray = CreateCpuTrayIcon(cPercentOld, bPaused);
                    }
                    TrayMessage(hDlg, NIM_MODIFY, ID_TRAY3, hiconTray);
                }
            }
            break;

        case WM_HSCROLL:
            if (GetDlgItem(hDlg, IDC_SPEED) == (HWND)lParam)
            {
                HWND hwndCtrl = GetDlgItem(hDlg, IDC_ACCEPT);
                if (!IsWindowEnabled(hwndCtrl))
                    EnableWindow(hwndCtrl, TRUE);
            }
            break;

        case TRAY_NOTIFYICON:
            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    if (wParam == ID_TRAY3)
                    {
                        PropSheet_SetCurSel(g_hwndPropSheet, NULL, 4);
                        if (g_bHideState)
                        {
                            ShowWindow(g_hwndPropSheet, SW_SHOW);
                            if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_SHOW);
                            g_bHideState = FALSE;
                        }
                        SetForegroundWindow(g_bPocketPC ? (HWND)(((DWORD)g_hwndPropSheet) | 0x01) : g_hwndPropSheet);
                    }
            }
            return TRUE;

        case WM_COMMAND:
            {
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_HISTORY:
                        {
                            bShowDetails = !bShowDetails;
                            HWND hwndHistory = GetDlgItem(hDlg, IDC_HISTORY);
                            InvalidateRect(hwndHistory, NULL, FALSE);
                            UpdateWindow(hwndHistory);
                        }
                        return FALSE;

                    case IDC_PAUSE:
                        {
                            TCHAR szText[128];

                            if (!g_bNoCpuTrayIcon)
                            {
                                if (g_bStaticIcons)
                                {
                                    if (hiconTray != NULL) DestroyIcon(hiconTray);
                                    uIconIdOld = GetTrayIconID(cPercentOld, !bPaused);
                                    hiconTray = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE((WORD)uIconIdOld), IMAGE_ICON, 16, 16, 0);
                                }
                                else if (g_bPocketPC)
                                {
                                    cFillHeightOld = GetCpuLoadFillHeight(cPercentOld, !bPaused);
                                    HICON hIconNew = CreateCpuTrayIcon(cPercentOld, !bPaused);
                                    if (hiconTray != NULL) DestroyIcon(hiconTray);
                                    hiconTray = hIconNew;
                                }
                                else
                                {
                                    cFillHeightOld = GetCpuLoadFillHeight(cPercentOld, !bPaused);
                                    if (hiconTray != NULL) DestroyIcon(hiconTray);
                                    hiconTray = CreateCpuTrayIcon(cPercentOld, !bPaused);
                                }
                                TrayMessage(hDlg, NIM_MODIFY, ID_TRAY3, hiconTray);
                            }

                            if (bPaused)
                            {
                                bPaused = FALSE;
                                SetTimer(hDlg, ID_TIMER2, g_uInterval, NULL);
                                PostMessage(hDlg, WM_TIMER, ID_TIMER2, 0);
                                if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_BTN_PAUSE : IDS_ENG_BTN_PAUSE,
                                    szText, _countof(szText)) > 0)
                                    SetDlgItemText(hDlg, IDC_PAUSE, szText);
                            }
                            else
                            {
                                bPaused = TRUE;
                                KillTimer(hDlg, ID_TIMER2);
                                if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_BTN_RESUME : IDS_ENG_BTN_RESUME,
                                    szText, _countof(szText)) > 0)
                                    SetDlgItemText(hDlg, IDC_PAUSE, szText);
                            }

                            HWND hwndHistory = GetDlgItem(hDlg, IDC_HISTORY);
                            InvalidateRect(hwndHistory, NULL, FALSE);
                            UpdateWindow(hwndHistory);
                        }
                        return FALSE;

                    case IDC_ACCEPT:
                        {
                            HWND hwndHistory = GetDlgItem(hDlg, IDC_HISTORY);
                            HWND hwndPause   = GetDlgItem(hDlg, IDC_PAUSE);
                            HWND hwndAccept  = GetDlgItem(hDlg, IDC_ACCEPT);

                            if (IsWindowEnabled(hwndAccept))
                            {
                                if (GetFocus() == hwndAccept)
                                {
                                    if (IsWindowEnabled(hwndPause))
                                        PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hwndPause, 1);
                                    else
                                        PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hwndHistory, 1);
                                }
                                EnableWindow(hwndAccept, FALSE);
                            }

                            UINT uInterval = (UINT)SendDlgItemMessage(hDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
                            if (uInterval >= 500 && uInterval <= 3000 && uInterval != g_uInterval)
                            {
                                if (!bPaused)
                                {
                                    if (KillTimer(hDlg, ID_TIMER2))
                                        SetTimer(hDlg, ID_TIMER2, uInterval, NULL);
                                }
                                g_uInterval = uInterval;

                                InvalidateRect(hwndHistory, NULL, FALSE);
                                UpdateWindow(hwndHistory);

                                HKEY hKey; DWORD Disp;
                                if (RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegPath,
                                    0, NULL, 0, 0, NULL, &hKey, &Disp) == ERROR_SUCCESS)
                                {
                                    DWORD dwInterval = (DWORD)uInterval;
                                    RegSetValueEx(hKey, g_szInterval, 0, REG_DWORD, (LPBYTE)&dwInterval, sizeof(DWORD));
                                    RegCloseKey(hKey);
                                }
                            }
                        }
                        return FALSE;

                    case IDC_HIDE:
                        ShowWindow(g_hwndPropSheet, SW_HIDE);
                        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
                        g_bHideState = TRUE;
                        return FALSE;
                }
            }
            break;

        case WM_PSPCB_RELEASE:
            {
                if (bShowDetails != g_bCpuDetails)
                {
                    HKEY hKey; DWORD Disp;
                    if (RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegPath,
                        0, NULL, 0, 0, NULL, &hKey, &Disp) == ERROR_SUCCESS)
                    {
                        DWORD dwDetails = (DWORD)bShowDetails;
                        RegSetValueEx(hKey, g_szCpuDetails, 0, REG_DWORD, (LPBYTE)&dwDetails, sizeof(DWORD));
                        RegCloseKey(hKey);
                    }
                }
                if (bPaused != g_bCpuPause)
                {
                    HKEY hKey; DWORD Disp;
                    if (RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegPath,
                        0, NULL, 0, 0, NULL, &hKey, &Disp) == ERROR_SUCCESS)
                    {
                        DWORD dwPaused = (DWORD)bPaused;
                        RegSetValueEx(hKey, g_szCpuPause, 0, REG_DWORD, (LPBYTE)&dwPaused, sizeof(DWORD));
                        RegCloseKey(hKey);
                    }
                }
                if (!bPaused)
                    KillTimer(hDlg, ID_TIMER2);
                if (!g_bNoCpuTrayIcon)
                {
                    TrayMessage(hDlg, NIM_DELETE, ID_TRAY3, hiconTray);
                    if (hiconTray != NULL)
                    {
                        DestroyIcon(hiconTray);
                        hiconTray = NULL;
                    }
                }
                g_hwndPropPage8 = NULL;
            }
            return TRUE;

        case WM_INITDIALOG:
            {
                g_hwndPropPage8 = hDlg;
                ((LPPROPSHEETPAGE)lParam)->lParam = (LPARAM)hDlg;

                memset(achHistory, -1, HISTORY_MAX);

                bShowDetails = g_bCpuDetails;
                bPaused = g_bCpuPause;
                if (g_fnGetThreadTimes == NULL)
                    bPaused = TRUE;
                
                ResizeWindow(GetDlgItem(hDlg, IDC_HISTORY));

                HWND hwndCtrl = GetDlgItem(hDlg, IDC_SPEED);
                if (hwndCtrl != NULL)
                {
                    SendMessage(hwndCtrl, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(500, 3000));
                    SendMessage(hwndCtrl, TBM_SETTICFREQ, 500, 0);
                    SendMessage(hwndCtrl, TBM_SETLINESIZE, 0, 250);
                    SendMessage(hwndCtrl, TBM_SETPAGESIZE, 0, 500);
                    SendMessage(hwndCtrl, TBM_SETPOS, TRUE, g_uInterval);
                }

                EnableWindow(GetDlgItem(hDlg, IDC_ACCEPT), FALSE);

                if (!g_bNoCpuTrayIcon)
                {
                    if (g_bStaticIcons)
                    {
                        uIconIdOld = GetTrayIconID(0, bPaused);
                        hiconTray = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE((WORD)uIconIdOld), IMAGE_ICON, 16, 16, 0);
                    }
                    else
                    {
                        cFillHeightOld = GetCpuLoadFillHeight((BYTE)-1, bPaused);
                        hiconTray = CreateCpuTrayIcon((BYTE)-1, bPaused);
                    }
                    TrayMessage(hDlg, NIM_ADD, ID_TRAY3, hiconTray);
                }

                if (!bPaused)
                {
                    SetTimer(hDlg, ID_TIMER2, g_uInterval, NULL);
                    PostMessage(hDlg, WM_TIMER, ID_TIMER2, 0);
                }
                else
                {
                    TCHAR szText[128];
                    if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_BTN_RESUME : IDS_ENG_BTN_RESUME,
                        szText, _countof(szText)) > 0)
                        SetDlgItemText(hDlg, IDC_PAUSE, szText);
                    if (g_fnGetThreadTimes == NULL)
                        EnableWindow(GetDlgItem(hDlg, IDC_PAUSE), FALSE);
                }
            }
            return TRUE;
    }

    return FALSE;
}


BOOL TrayMessage(HWND hwnd, DWORD dwMessage, UINT uID, HICON hIcon)
{
    NOTIFYICONDATA tnd;

    tnd.cbSize = sizeof(NOTIFYICONDATA);
    tnd.hWnd = hwnd;
    tnd.uID = uID;
    tnd.uFlags = NIF_MESSAGE;
    if (hIcon != NULL)
        tnd.uFlags |= NIF_ICON;
    tnd.uCallbackMessage = TRAY_NOTIFYICON;
    tnd.hIcon = hIcon;
    tnd.szTip[0] = TEXT('\0');

    return Shell_NotifyIcon(dwMessage, &tnd);
}


HICON CreateTrayIcon1(BYTE cPcMem, BYTE cPcObj, BYTE cPcMemDiv)
{
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);

    RECT rc;
    rc.top    = (cyIcon - 16) / 2;
    rc.left   = (cxIcon - 16) / 2;
    rc.bottom = rc.top  + 16;
    rc.right  = rc.left + 16;

    HDC hdcScreen = GetDC(NULL);
    if (hdcScreen == NULL)
        return NULL;
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (hdcMem == NULL)
    {
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }
    HDC hdcMemMask = CreateCompatibleDC(hdcMem);
    if (hdcMemMask == NULL)
    {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    ICONINFO iconinfo;
    iconinfo.fIcon = TRUE;
    iconinfo.xHotspot = cxIcon / 2;
    iconinfo.yHotspot = cyIcon / 2;
    iconinfo.hbmMask = CreateBitmap(cxIcon, cyIcon, 1, 1, NULL);
    if (iconinfo.hbmMask == NULL)
    {
        DeleteDC(hdcMemMask);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }
    iconinfo.hbmColor = CreateCompatibleBitmap(hdcScreen, cxIcon, cyIcon);
    if (iconinfo.hbmColor == NULL)
    {
        DeleteObject(iconinfo.hbmMask);
        DeleteDC(hdcMemMask);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    COLORREF colFilled = (g_bAlternativeColor ? RGB(  0,   0, 255) : RGB(255,   0, 0));
    COLORREF colFree   = (g_bAlternativeColor ? RGB(255, 255,   0) : RGB(  0, 255, 0));
    COLORREF colref = GetNearestColor(hdcScreen, RGB(255, 0, 0));
    if ((GetRValue(colref) == GetBValue(colref)) && (GetRValue(colref) == GetGValue(colref)))
    {
        colFilled = (g_bAlternativeColor ? RGB(0, 0, 0) : RGB(128, 128, 128));
        colFree = RGB(255, 255, 255);
    }

    ReleaseDC(NULL, hdcScreen);
    
    HBITMAP hbmMaskOld = (HBITMAP)SelectObject(hdcMemMask, iconinfo.hbmMask);

    PatBlt(hdcMemMask, 0, 0, cxIcon, cyIcon, WHITENESS);

    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, iconinfo.hbmColor);
    PatBlt(hdcMem, 0, 0, cxIcon, cyIcon, BLACKNESS);

    HPEN hpenOld = NULL;
    HPEN hpenMaskOld = NULL;
    HPEN hpen = CreatePen(PS_SOLID, 0, g_crIconColor);
    HPEN hpenMask = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
    if (hpen != NULL)
        hpenOld = (HPEN)SelectObject(hdcMem, hpen);
    if (hpenMask != NULL)
        hpenMaskOld = (HPEN)SelectObject(hdcMemMask, hpenMask);

    POINT apt[2];
    apt[0].x = rc.left;
    apt[0].y = rc.bottom - 15;
    apt[1].x = rc.left;
    apt[1].y = rc.bottom - 12;

    Polyline(hdcMemMask, apt, 2);
    Polyline(hdcMem, apt, 2);

    apt[0].x = rc.left   +  1;
    apt[0].y = rc.bottom - 14;
    apt[1].x = rc.right  -  1;
    apt[1].y = rc.bottom - 14;

    Polyline(hdcMemMask, apt, 2);
    Polyline(hdcMem, apt, 2);

    apt[0].x = rc.right  -  1;
    apt[0].y = rc.bottom - 15;
    apt[1].x = rc.right  -  1;
    apt[1].y = rc.bottom - 12;

    Polyline(hdcMemMask, apt, 2);
    Polyline(hdcMem, apt, 2);

    if (cPcMemDiv <= 100)
    {
        int yPos = ((cPcMemDiv * 15) / 10 + 5) / 10;

        apt[0].x = rc.left + yPos;
        apt[0].y = rc.bottom - 15;
        apt[1].x = rc.left + yPos;
        apt[1].y = rc.bottom - 12;

        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);
    }

    HBRUSH hbr = NULL;
    HBRUSH hbrOld = NULL;
    HBRUSH hbrMask = NULL;
    HBRUSH hbrMaskOld = NULL;

    if (cPcMem > 100)
    {
        hbr = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        hbrMask = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    }
    else
    {
        hbr = CreateSolidBrush(colFree);
        hbrMask = CreateSolidBrush(RGB(0, 0, 0));
    }
    if (hbr != NULL)
        hbrOld = (HBRUSH)SelectObject(hdcMem, hbr);
    if (hbrMask != NULL)
        hbrMaskOld = (HBRUSH)SelectObject(hdcMemMask, hbrMask);

    Rectangle(hdcMemMask, rc.left, rc.bottom-11, rc.right, rc.bottom-6);
    Rectangle(hdcMem, rc.left, rc.bottom-11, rc.right, rc.bottom-6);

    if (hbrOld != NULL)
        SelectObject(hdcMem, hbrOld);
    if (hbrMaskOld != NULL)
        SelectObject(hdcMemMask, hbrMaskOld);
    if (!(cPcMem > 100))
    {
        if (hbr != NULL)
            DeleteObject(hbr);
        if (hbrMask != NULL)
            DeleteObject(hbrMask);
    }

    hbrOld = NULL;
    hbrMaskOld = NULL;

    if (cPcObj > 100)
    {
        hbr = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        hbrMask = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    }
    else
    {
        hbr = CreateSolidBrush(colFree);
        hbrMask = CreateSolidBrush(RGB(0, 0, 0));
    }
    
    if (hbr != NULL)
        hbrOld = (HBRUSH)SelectObject(hdcMem, hbr);
    if (hbrMask != NULL)
        hbrMaskOld = (HBRUSH)SelectObject(hdcMemMask, hbrMask);

    Rectangle(hdcMemMask, rc.left, rc.bottom-5, rc.right, rc.bottom);
    Rectangle(hdcMem, rc.left, rc.bottom-5, rc.right, rc.bottom);

    if (hbrOld != NULL)
        SelectObject(hdcMem, hbrOld);
    if (hbrMaskOld != NULL)
        SelectObject(hdcMemMask, hbrMaskOld);
    if (!(cPcObj > 100))
    {
        if (hbr != NULL)
            DeleteObject(hbr);
        if (hbrMask != NULL)
            DeleteObject(hbrMask);
    }

    if (hpenOld != NULL)
        SelectObject(hdcMem, hpenOld);
    if (hpenMaskOld != NULL)
        SelectObject(hdcMemMask, hpenMaskOld);
    if (hpen != NULL)
        DeleteObject(hpen);
    if (hpenMask != NULL)
        DeleteObject(hpenMask);

    hbrOld = NULL;
    hbrMaskOld = NULL;
    hbr = CreateSolidBrush(colFilled);
    hbrMask = CreateSolidBrush(RGB(0, 0, 0));
    if (hbr != NULL)
        hbrOld = (HBRUSH)SelectObject(hdcMem, hbr);
    if (hbrMask != NULL)
        hbrMaskOld = (HBRUSH)SelectObject(hdcMemMask, hbrMask);

    int cxRed;
    RECT rcRed;
    rcRed.left = rc.left + 1;

    if (cPcMem <= 100)
    {
        if (cxRed = ((cPcMem * 14) / 10 + 5) / 10)
        {
            rcRed.top    = rc.bottom - 10;
            rcRed.right  = rc.left + cxRed + 1;
            rcRed.bottom = rc.bottom - 7;

            FillRect(hdcMemMask, &rcRed, hbrMask);
            FillRect(hdcMem, &rcRed, hbr);
        }
    }

    if (cPcObj <= 100)
    {
        if (cxRed = ((cPcObj * 14) / 10 + 5) / 10)
        {
            rcRed.top    = rc.bottom - 4;
            rcRed.right  = rc.left + cxRed + 1;
            rcRed.bottom = rc.bottom - 1;

            FillRect(hdcMemMask, &rcRed, hbrMask);
            FillRect(hdcMem, &rcRed, hbr);
        }
    }
    
    if (hbrOld != NULL)
        SelectObject(hdcMem, hbrOld);
    if (hbrMaskOld != NULL)
        SelectObject(hdcMemMask, hbrMaskOld);
    if (hbr != NULL)
        DeleteObject(hbr);
    if (hbrMask != NULL)
        DeleteObject(hbrMask);

    if (cxIcon >= 32 && cyIcon >= 32)
    {
        Stretch16x16To32x32(hdcMemMask, cxIcon, cyIcon, WHITENESS);
        Stretch16x16To32x32(hdcMem, cxIcon, cyIcon, BLACKNESS);
    }

    if (hbmMaskOld != NULL)
        SelectObject(hdcMemMask, hbmMaskOld);
    if (hbmOld != NULL)
        SelectObject(hdcMem, hbmOld);

    DeleteDC(hdcMemMask);
    DeleteDC(hdcMem);

    HICON hIcon = CreateIconIndirect(&iconinfo);

    DeleteObject(iconinfo.hbmColor);
    DeleteObject(iconinfo.hbmMask);

    return hIcon;
}


HICON CreateTrayIcon2(LPCTSTR lpszText, BYTE cPercent, BYTE cBatFlag)
{
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);

    RECT rc;
    rc.top    = (cyIcon - 16) / 2;
    rc.left   = (cxIcon - 16) / 2;
    rc.bottom = rc.top  + 16;
    rc.right  = rc.left + 16;

    HDC hdcScreen = GetDC(NULL);
    if (hdcScreen == NULL)
        return NULL;
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (hdcMem == NULL)
    {
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }
    HDC hdcMemMask = CreateCompatibleDC(hdcMem);
    if (hdcMemMask == NULL)
    {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    ICONINFO iconinfo;
    iconinfo.fIcon = TRUE;
    iconinfo.xHotspot = cxIcon / 2;
    iconinfo.yHotspot = cyIcon / 2;
    iconinfo.hbmMask = CreateBitmap(cxIcon, cyIcon, 1, 1, NULL);
    if (iconinfo.hbmMask == NULL)
    {
        DeleteDC(hdcMemMask);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }
    iconinfo.hbmColor = CreateCompatibleBitmap(hdcScreen, cxIcon, cyIcon);
    if (iconinfo.hbmColor == NULL)
    {
        DeleteObject(iconinfo.hbmMask);
        DeleteDC(hdcMemMask);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    BOOL bGrayScale = FALSE;
    COLORREF colref = GetNearestColor(hdcScreen, RGB(255, 0, 0));
    if ((GetRValue(colref) == GetBValue(colref)) && (GetRValue(colref) == GetGValue(colref)))
        bGrayScale = TRUE;

    ReleaseDC(NULL, hdcScreen);

    HBITMAP hbmMaskOld = (HBITMAP)SelectObject(hdcMemMask, iconinfo.hbmMask);

    PatBlt(hdcMemMask, 0, 0, cxIcon, cyIcon, WHITENESS);

    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, iconinfo.hbmColor);
    PatBlt(hdcMem, 0, 0, cxIcon, cyIcon, BLACKNESS);

    HPEN hpenOld = NULL;
    HPEN hpenMaskOld = NULL;
    HPEN hpen = CreatePen(PS_SOLID, 0, g_crIconColor);
    HPEN hpenMask = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
    if (hpen != NULL)
        hpenOld = (HPEN)SelectObject(hdcMem, hpen);
    if (hpenMask != NULL)
        hpenMaskOld = (HPEN)SelectObject(hdcMemMask, hpenMask);

    POINT apt[9];
    apt[0].x = rc.right  - 3;
    apt[0].y = rc.bottom - 1;
    apt[1].x = rc.left   + 2;
    apt[1].y = rc.bottom - 1;
    apt[2].x = rc.left   + 2;
    apt[2].y = rc.bottom - 2;
    apt[3].x = rc.left   + 1;
    apt[3].y = rc.bottom - 2;
    apt[4].x = rc.left   + 1;
    apt[4].y = rc.bottom - 4;
    apt[5].x = rc.left   + 2;
    apt[5].y = rc.bottom - 4;
    apt[6].x = rc.left   + 2;
    apt[6].y = rc.bottom - 5;
    apt[7].x = rc.right  - 3;
    apt[7].y = rc.bottom - 5;
    apt[8].x = rc.right  - 3;
    apt[8].y = rc.bottom - 1;

    Polyline(hdcMemMask, apt, 9);
    Polyline(hdcMem, apt, 9);

    BOOL bFill = TRUE;
    COLORREF crPen = GetSysColor(COLOR_MENU);
    if (cPercent <= 10)
        crPen = (bGrayScale ? RGB(255, 255, 255) : RGB(255, 0, 0));
    else if (cPercent <= 30)
        crPen = (bGrayScale ? RGB(192, 192, 192) : RGB(255, 255, 0));
    else if (cPercent <= 100)
        crPen = (bGrayScale ? RGB(128, 128, 128) : RGB(0, 255, 0));
    else
        bFill = FALSE;

    if (cBatFlag != BATTERY_FLAG_UNKNOWN)
    {
        if (cBatFlag & BATTERY_FLAG_CHARGING)
            crPen = (bGrayScale ? RGB(0, 0, 0) : RGB(0, 0, 255));
        if (cBatFlag & BATTERY_FLAG_NO_BATTERY)
            bFill = FALSE;
    }

    if (bFill)
    {
        if (hpen != NULL)
            DeleteObject(hpen);
        hpen = CreatePen(PS_SOLID, 0, crPen);
        if (hpen != NULL)
            SelectObject(hdcMem, hpen);

        apt[0].x = rc.right  - 4;
        apt[0].y = rc.bottom - 2;
        apt[1].x = rc.left   + 2;
        apt[1].y = rc.bottom - 2;
        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        apt[0].x = rc.right  - 4;
        apt[0].y = rc.bottom - 3;
        apt[1].x = rc.left   + 1;
        apt[1].y = rc.bottom - 3;
        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        apt[0].x = rc.right  - 4;
        apt[0].y = rc.bottom - 4;
        apt[1].x = rc.left   + 2;
        apt[1].y = rc.bottom - 4;
        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);
    }

    rc.bottom = rc.bottom - 5;

    if (hpenOld != NULL)
        SelectObject(hdcMem, hpenOld);
    if (hpenMaskOld != NULL)
        SelectObject(hdcMemMask, hpenMaskOld);
    if (hpen != NULL)
        DeleteObject(hpen);
    if (hpenMask != NULL)
        DeleteObject(hpenMask);

    LOGFONT lf;
    TCHAR lfFaceName[LF_FACESIZE];
    lfFaceName[0] = TEXT('\0');
    GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);
    if (lf.lfFaceName[0] != TEXT('\0'))
        _tcscpy(lfFaceName, lf.lfFaceName);
    memset((void *)&lf, 0, sizeof(lf));
    if (lfFaceName[0] != TEXT('\0'))
        _tcscpy(lf.lfFaceName, lfFaceName);
    lf.lfHeight  = 11;
    lf.lfWeight  = FW_NORMAL;
#if (_WIN32_WCE > 200)
    lf.lfQuality = NONANTIALIASED_QUALITY;
#endif

    HFONT hfontMaskOld = NULL;
    HFONT hfontOld = NULL;
    HFONT hfont = CreateFontIndirect(&lf);
    if (hfont != NULL)
    {
        hfontMaskOld = (HFONT)SelectObject(hdcMemMask, hfont);
        hfontOld = (HFONT)SelectObject(hdcMem, hfont);
    }
    COLORREF rgbOld = SetTextColor(hdcMem, g_crIconColor);
    int nBkModeMaskOld = SetBkMode(hdcMemMask, TRANSPARENT);
    int nBkModeOld = SetBkMode(hdcMem, TRANSPARENT);

    UINT uFormat = DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_TOP;
    DrawText(hdcMem, lpszText, -1, &rc,  uFormat);

    if (cPercent == 100)
        rc.right = rc.left + 10;

    if (rc.right-rc.left+5 < 16)
    {
        hpenOld = NULL;
        hpenMaskOld = NULL;
        hpen = CreatePen(PS_SOLID, 0, g_crIconColor);
        hpenMask = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
        if (hpen != NULL)
            hpenOld = (HPEN)SelectObject(hdcMem, hpen);
        if (hpenMask != NULL)
            hpenMaskOld = (HPEN)SelectObject(hdcMemMask, hpenMask);

        int cxOffset = (rc.left-rc.right+11) / 2;
        rc.left = ((cxIcon - 16) / 2) + cxOffset;
        rc.right += cxOffset;

        if (cPercent == 100)
        {
            apt[0].x = rc.left;
            apt[0].y = rc.bottom - 8;
            apt[1].x = rc.left   + 1;
            apt[1].y = rc.bottom - 8;

            Polyline(hdcMemMask, apt, 2);
            Polyline(hdcMem, apt, 2);

            apt[0].x = rc.left   + 1;
            apt[0].y = rc.bottom - 9;
            apt[1].x = rc.left   + 1;
            apt[1].y = rc.bottom - 2;

            Polyline(hdcMemMask, apt, 2);
            Polyline(hdcMem, apt, 2);

            apt[0].x = rc.left   + 3;
            apt[0].y = rc.bottom - 9;
            apt[1].x = rc.left   + 3;
            apt[1].y = rc.bottom - 3;
            apt[2].x = rc.left   + 5;
            apt[2].y = rc.bottom - 3;
            apt[3].x = rc.left   + 5;
            apt[3].y = rc.bottom - 9;
            apt[4].x = rc.left   + 3;
            apt[4].y = rc.bottom - 9;

            Polyline(hdcMemMask, apt, 5);
            Polyline(hdcMem, apt, 5);

            apt[0].x = rc.left   + 7;
            apt[0].y = rc.bottom - 9;
            apt[1].x = rc.left   + 7;
            apt[1].y = rc.bottom - 3;
            apt[2].x = rc.left   + 9;
            apt[2].y = rc.bottom - 3;
            apt[3].x = rc.left   + 9;
            apt[3].y = rc.bottom - 9;
            apt[4].x = rc.left   + 7;
            apt[4].y = rc.bottom - 9;

            Polyline(hdcMemMask, apt, 5);
            Polyline(hdcMem, apt, 5);
        }

        apt[0].x = rc.right  + 1;
        apt[0].y = rc.bottom - 8;
        apt[1].x = rc.right  + 1;
        apt[1].y = rc.bottom - 6;

        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        apt[0].x = rc.right  + 4;
        apt[0].y = rc.bottom - 3;
        apt[1].x = rc.right  + 4;
        apt[1].y = rc.bottom - 5;

        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        apt[0].x = rc.right  + 1;
        apt[0].y = rc.bottom - 3;
        apt[1].x = rc.right  + 1;
        apt[1].y = rc.bottom - 4;

        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        apt[0].x = rc.right  + 2;
        apt[0].y = rc.bottom - 4;
        apt[1].x = rc.right  + 2;
        apt[1].y = rc.bottom - 6;

        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        apt[0].x = rc.right  + 4;
        apt[0].y = rc.bottom - 8;
        apt[1].x = rc.right  + 4;
        apt[1].y = rc.bottom - 7;

        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        apt[0].x = rc.right  + 3;
        apt[0].y = rc.bottom - 7;
        apt[1].x = rc.right  + 3;
        apt[1].y = rc.bottom - 5;

        Polyline(hdcMemMask, apt, 2);
        Polyline(hdcMem, apt, 2);

        if (hpenOld != NULL)
            SelectObject(hdcMem, hpenOld);
        if (hpenMaskOld != NULL)
            SelectObject(hdcMemMask, hpenMaskOld);
        if (hpen != NULL)
            DeleteObject(hpen);
        if (hpenMask != NULL)
            DeleteObject(hpenMask);

        uFormat = DT_LEFT | DT_SINGLELINE | DT_TOP;
    }
    else
    {
        rc.top    = (cyIcon - 16) / 2;
        rc.left   = (cxIcon - 16) / 2;
        rc.bottom = rc.top  + 11;
        rc.right  = rc.left + 16;
        uFormat = DT_CENTER | DT_SINGLELINE | DT_TOP;
    }

    if (cPercent != 100 || uFormat & DT_CENTER)
    {
        DrawText(hdcMemMask, lpszText, -1, &rc,  uFormat);
        DrawText(hdcMem, lpszText, -1, &rc,  uFormat);
    }

    SetBkMode(hdcMemMask, nBkModeMaskOld);
    SetBkMode(hdcMem, nBkModeOld);
    SetTextColor(hdcMem, rgbOld);

    if (hfontMaskOld != NULL)
        SelectObject(hdcMemMask, hfontMaskOld);
    if (hfontOld != NULL)
        SelectObject(hdcMem, hfontOld);
    if (hfont != NULL)
        DeleteObject(hfont);

    if (cxIcon >= 32 && cyIcon >= 32)
    {
        Stretch16x16To32x32(hdcMemMask, cxIcon, cyIcon, WHITENESS);
        Stretch16x16To32x32(hdcMem, cxIcon, cyIcon, BLACKNESS);
    }

    if (hbmMaskOld != NULL)
        SelectObject(hdcMemMask, hbmMaskOld);
    if (hbmOld != NULL)
        SelectObject(hdcMem, hbmOld);

    DeleteDC(hdcMemMask);
    DeleteDC(hdcMem);

    HICON hIcon = CreateIconIndirect(&iconinfo);

    DeleteObject(iconinfo.hbmColor);
    DeleteObject(iconinfo.hbmMask);

    return hIcon;
}


HICON CreateCpuTrayIcon(BYTE cPercent, BOOL bPaused)
{
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);

    RECT rc;
    rc.top    = (cyIcon - 16) / 2;
    rc.left   = (cxIcon - 16) / 2;
    rc.bottom = rc.top  + 16;
    rc.right  = rc.left + 16;

    HDC hdcScreen = GetDC(NULL);
    if (hdcScreen == NULL)
        return NULL;
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (hdcMem == NULL)
    {
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }
    HDC hdcMemMask = CreateCompatibleDC(hdcMem);
    if (hdcMemMask == NULL)
    {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    ICONINFO iconinfo;
    iconinfo.fIcon = TRUE;
    iconinfo.xHotspot = cxIcon / 2;
    iconinfo.yHotspot = cyIcon / 2;
    iconinfo.hbmMask = CreateBitmap(cxIcon, cyIcon, 1, 1, NULL);
    if (iconinfo.hbmMask == NULL)
    {
        DeleteDC(hdcMemMask);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }
    iconinfo.hbmColor = CreateCompatibleBitmap(hdcScreen, cxIcon, cyIcon);
    if (iconinfo.hbmColor == NULL)
    {
        DeleteObject(iconinfo.hbmMask);
        DeleteDC(hdcMemMask);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    COLORREF colBack = (g_bAlternativeColor ? RGB(0,   0, 255) : RGB(0,  64, 0));
    COLORREF colLoad = (g_bAlternativeColor ? RGB(0, 255, 255) : RGB(0, 255, 0));
    COLORREF colref = GetNearestColor(hdcScreen, RGB(255, 0, 0));
    if ((GetRValue(colref) == GetBValue(colref)) && (GetRValue(colref) == GetGValue(colref)))
    {
        colBack = RGB(255, 255, 255);
        colLoad = (g_bAlternativeColor ? RGB(0, 0, 0) : RGB(128, 128, 128));
    }

    ReleaseDC(NULL, hdcScreen);

    HBITMAP hbmMaskOld = (HBITMAP)SelectObject(hdcMemMask, iconinfo.hbmMask);

    PatBlt(hdcMemMask, 0, 0, cxIcon, cyIcon, WHITENESS);

    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, iconinfo.hbmColor);
    PatBlt(hdcMem, 0, 0, cxIcon, cyIcon, BLACKNESS);

    HPEN hpenOld = NULL;
    HPEN hpenMaskOld = NULL;
    HPEN hpen = CreatePen(PS_SOLID, 0, g_crIconColor);
    HPEN hpenMask = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
    if (hpen != NULL)
        hpenOld = (HPEN)SelectObject(hdcMem, hpen);
    if (hpenMask != NULL)
        hpenMaskOld = (HPEN)SelectObject(hdcMemMask, hpenMask);

    HBRUSH hbrOld = NULL;
    HBRUSH hbrMaskOld = NULL;
    HBRUSH hbr = CreateSolidBrush(colBack);
    HBRUSH hbrMask = CreateSolidBrush(RGB(0, 0, 0));
    if (hbr != NULL)
        hbrOld = (HBRUSH)SelectObject(hdcMem, hbr);
    if (hbrMask != NULL)
        hbrMaskOld = (HBRUSH)SelectObject(hdcMemMask, hbrMask);

    RECT rcLoad = rc;
    rcLoad.top += 3;
    rcLoad.left += 2;
    rcLoad.right -= 2;

    Rectangle(hdcMemMask, rcLoad.left, rcLoad.top, rcLoad.right, rcLoad.bottom);
    Rectangle(hdcMem, rcLoad.left, rcLoad.top, rcLoad.right, rcLoad.bottom);

    if (hbrOld != NULL)
        SelectObject(hdcMem, hbrOld);
    if (hbrMaskOld != NULL)
        SelectObject(hdcMemMask, hbrMaskOld);
    if (hbr != NULL)
        DeleteObject(hbr);
    if (hbrMask != NULL)
        DeleteObject(hbrMask);

    if (hpenOld != NULL)
        SelectObject(hdcMem, hpenOld);
    if (hpenMaskOld != NULL)
        SelectObject(hdcMemMask, hpenMaskOld);
    if (hpen != NULL)
        DeleteObject(hpen);
    if (hpenMask != NULL)
        DeleteObject(hpenMask);

    hbrOld = NULL;
    hbrMaskOld = NULL;
    hbr = CreateSolidBrush(colLoad);
    hbrMask = CreateSolidBrush(RGB(0, 0, 0));
    if (hbr != NULL)
        hbrOld = (HBRUSH)SelectObject(hdcMem, hbr);
    if (hbrMask != NULL)
        hbrMaskOld = (HBRUSH)SelectObject(hdcMemMask, hbrMask);

    InflateRect(&rcLoad, -1, -1);

    if (cPercent <= 100 && !bPaused)
    {
        BYTE cyLoad = GetCpuLoadFillHeight(cPercent, bPaused);
        if (cyLoad > 0)
        {
            rcLoad.top = rcLoad.bottom - cyLoad;

            FillRect(hdcMemMask, &rcLoad, hbrMask);
            FillRect(hdcMem, &rcLoad, hbr);
        }
    }
    else
    {
        LOGFONT lf;
        TCHAR lfFaceName[LF_FACESIZE];
        lfFaceName[0] = TEXT('\0');
        GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);
        if (lf.lfFaceName[0] != TEXT('\0'))
            _tcscpy(lfFaceName, lf.lfFaceName);
        memset((void *)&lf, 0, sizeof(lf));
        if (lfFaceName[0] != TEXT('\0'))
            _tcscpy(lf.lfFaceName, lfFaceName);
        lf.lfHeight  = -11;
        lf.lfWeight  = FW_BOLD;
#if (_WIN32_WCE > 200)
        lf.lfQuality = NONANTIALIASED_QUALITY;
#endif
        
        HFONT hfontMaskOld = NULL;
        HFONT hfontOld = NULL;
        HFONT hfont = CreateFontIndirect(&lf);
        if (hfont != NULL)
        {
            hfontMaskOld = (HFONT)SelectObject(hdcMemMask, hfont);
            hfontOld = (HFONT)SelectObject(hdcMem, hfont);
        }

        COLORREF rgbOld = SetTextColor(hdcMem, colLoad);
        int nBkModeMaskOld = SetBkMode(hdcMemMask, TRANSPARENT);
        int nBkModeOld = SetBkMode(hdcMem, TRANSPARENT);

        rcLoad.left += 1;
        DrawText(hdcMemMask, bPaused ? TEXT("P") : TEXT("?"), -1, &rcLoad,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);
        DrawText(hdcMem, bPaused ? TEXT("P") : TEXT("?"), -1, &rcLoad,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);

        SetBkMode(hdcMemMask, nBkModeMaskOld);
        SetBkMode(hdcMem, nBkModeOld);
        SetTextColor(hdcMem, rgbOld);

        if (hfontMaskOld != NULL)
            SelectObject(hdcMemMask, hfontMaskOld);
        if (hfontOld != NULL)
            SelectObject(hdcMem, hfontOld);
        if (hfont != NULL)
            DeleteObject(hfont);
    }

    if (hbrOld != NULL)
        SelectObject(hdcMem, hbrOld);
    if (hbrMaskOld != NULL)
        SelectObject(hdcMemMask, hbrMaskOld);
    if (hbr != NULL)
        DeleteObject(hbr);
    if (hbrMask != NULL)
        DeleteObject(hbrMask);

    if (cxIcon >= 32 && cyIcon >= 32)
    {
        Stretch16x16To32x32(hdcMemMask, cxIcon, cyIcon, WHITENESS);
        Stretch16x16To32x32(hdcMem, cxIcon, cyIcon, BLACKNESS);
    }

    if (hbmMaskOld != NULL)
        SelectObject(hdcMemMask, hbmMaskOld);
    if (hbmOld != NULL)
        SelectObject(hdcMem, hbmOld);

    DeleteDC(hdcMemMask);
    DeleteDC(hdcMem);

    HICON hIcon = CreateIconIndirect(&iconinfo);

    DeleteObject(iconinfo.hbmColor);
    DeleteObject(iconinfo.hbmMask);

    return hIcon;
}


HICON CreateTrayIcon3(BYTE cPcMem)
{
    WORD wType;

    if (cPcMem > 100)
        wType = g_bAlternativeColor ? IDI_MEMUNKNB : IDI_MEMUNKNC;
    else if (cPcMem > 90)
        wType = g_bAlternativeColor ? IDI_MEM100B : IDI_MEM100C;
    else if (cPcMem > 80)
        wType = g_bAlternativeColor ? IDI_MEM090B : IDI_MEM090C;
    else if (cPcMem > 70)
        wType = g_bAlternativeColor ? IDI_MEM080B : IDI_MEM080C;
    else if (cPcMem > 60)
        wType = g_bAlternativeColor ? IDI_MEM070B : IDI_MEM070C;
    else if (cPcMem > 50)
        wType = g_bAlternativeColor ? IDI_MEM060B : IDI_MEM060C;
    else if (cPcMem > 40)
        wType = g_bAlternativeColor ? IDI_MEM050B : IDI_MEM050C;
    else if (cPcMem > 30)
        wType = g_bAlternativeColor ? IDI_MEM040B : IDI_MEM040C;
    else if (cPcMem > 20)
        wType = g_bAlternativeColor ? IDI_MEM030B : IDI_MEM030C;
    else if (cPcMem > 10)
        wType = g_bAlternativeColor ? IDI_MEM020B : IDI_MEM020C;
    else if (cPcMem > 0)
        wType = g_bAlternativeColor ? IDI_MEM010B : IDI_MEM010C;
    else
        wType = g_bAlternativeColor ? IDI_MEM000B : IDI_MEM000C;

    return (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(wType), IMAGE_ICON, 16, 16, 0);
}


HICON CreateTrayIcon4(BYTE cPercent, BYTE cBatFlag)
{
    WORD wType;

    if (cBatFlag != BATTERY_FLAG_UNKNOWN && (cBatFlag & BATTERY_FLAG_CHARGING))
        wType = g_bAlternativeColor ? IDI_BATLOADB : IDI_BATLOADC;
    else if (cBatFlag != BATTERY_FLAG_UNKNOWN && (cBatFlag & BATTERY_FLAG_NO_BATTERY))
        wType = g_bAlternativeColor ? IDI_BATNONEB : IDI_BATNONEC;
    else if (cPercent > 100)
        wType = g_bAlternativeColor ? IDI_BATUNKNB : IDI_BATUNKNC;
    else if (cPercent > 90)
        wType = g_bAlternativeColor ? IDI_BAT100B : IDI_BAT100C;
    else if (cPercent > 80)
        wType = g_bAlternativeColor ? IDI_BAT090B : IDI_BAT090C;
    else if (cPercent > 70)
        wType = g_bAlternativeColor ? IDI_BAT080B : IDI_BAT080C;
    else if (cPercent > 60)
        wType = g_bAlternativeColor ? IDI_BAT070B : IDI_BAT070C;
    else if (cPercent > 50)
        wType = g_bAlternativeColor ? IDI_BAT060B : IDI_BAT060C;
    else if (cPercent > 40)
        wType = g_bAlternativeColor ? IDI_BAT050B : IDI_BAT050C;
    else if (cPercent > 30)
        wType = g_bAlternativeColor ? IDI_BAT040B : IDI_BAT040C;
    else if (cPercent > 20)
        wType = g_bAlternativeColor ? IDI_BAT030B : IDI_BAT030C;
    else if (cPercent > 10)
        wType = g_bAlternativeColor ? IDI_BAT020B : IDI_BAT020C;
    else if (cPercent > 0)
        wType = g_bAlternativeColor ? IDI_BAT010B : IDI_BAT010C;
    else
        wType = g_bAlternativeColor ? IDI_BAT000B : IDI_BAT000C;

    return (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(wType), IMAGE_ICON, 16, 16, 0);
}


UINT GetTrayIconID(BYTE cPercent, BOOL bPaused)
{
    if (bPaused)
        return g_bAlternativeColor ? IDI_CPUUNKNB : IDI_CPUUNKNC;

    UINT uIconId;
    if (cPercent < 5)
        uIconId = g_bAlternativeColor ? IDI_CPU000B : IDI_CPU000C;
    else if (cPercent < 14)
        uIconId = g_bAlternativeColor ? IDI_CPU001B : IDI_CPU001C;
    else if (cPercent < 23)
        uIconId = g_bAlternativeColor ? IDI_CPU002B : IDI_CPU002C;
    else if (cPercent < 32)
        uIconId = g_bAlternativeColor ? IDI_CPU003B : IDI_CPU003C;
    else if (cPercent < 41)
        uIconId = g_bAlternativeColor ? IDI_CPU004B : IDI_CPU004C;
    else if (cPercent < 50)
        uIconId = g_bAlternativeColor ? IDI_CPU005B : IDI_CPU005C;
    else if (cPercent < 59)
        uIconId = g_bAlternativeColor ? IDI_CPU006B : IDI_CPU006C;
    else if (cPercent < 68)
        uIconId = g_bAlternativeColor ? IDI_CPU007B : IDI_CPU007C;
    else if (cPercent < 77)
        uIconId = g_bAlternativeColor ? IDI_CPU008B : IDI_CPU008C;
    else if (cPercent < 86)
        uIconId = g_bAlternativeColor ? IDI_CPU009B : IDI_CPU009C;
    else if (cPercent < 95)
        uIconId = g_bAlternativeColor ? IDI_CPU010B : IDI_CPU010C;
    else if (cPercent <= 100)
        uIconId = g_bAlternativeColor ? IDI_CPU011B : IDI_CPU011C;
    else
        uIconId = g_bAlternativeColor ? IDI_CPU000B : IDI_CPU000C;

    return uIconId;
}


__inline BYTE GetCpuLoadFillHeight(BYTE cPercent, BOOL bPaused)
{
    return (bPaused || cPercent > 100) ? (BYTE)-1 : (((UINT)cPercent * 11) / 10 + 5) / 10;
}


LONG LoadSettings(void)
{
    HKEY hKey;
    LONG lStatus = RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegPath, 0, 0, &hKey);
    if (lStatus != ERROR_SUCCESS)
        return lStatus;

    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    DWORD dwData = (DWORD)g_bHideApplication;
    lStatus  = RegQueryValueEx(hKey, g_szRunHidden, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bHideApplication = (BOOL)dwData;

    dwData = (DWORD)g_bResetRuntimes;
    lStatus  = RegQueryValueEx(hKey, g_szAutoReset, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bResetRuntimes = (BOOL)dwData;

    dwData = (DWORD)g_bNoMemTrayIcon;
    lStatus  = RegQueryValueEx(hKey, g_szNoMemIcon, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bNoMemTrayIcon = (BOOL)dwData;

    dwData = (DWORD)g_bNoBatTrayIcon;
    lStatus  = RegQueryValueEx(hKey, g_szNoBatIcon, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bNoBatTrayIcon = (BOOL)dwData;

    dwData = (DWORD)g_bNoCpuTrayIcon;
    lStatus  = RegQueryValueEx(hKey, g_szNoCpuIcon, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bNoCpuTrayIcon = (BOOL)dwData;

    dwData = (DWORD)g_bStaticIcons;
    lStatus  = RegQueryValueEx(hKey, g_szStaticIco, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bStaticIcons = (BOOL)dwData;

    dwData = (DWORD)g_bAlternativeColor;
    lStatus  = RegQueryValueEx(hKey, g_szAltColor, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bAlternativeColor = (BOOL)dwData;

    dwData = (DWORD)g_crIconColor;
    lStatus  = RegQueryValueEx(hKey, g_szIconColor, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_crIconColor = (COLORREF)dwData;

    dwData = (DWORD)g_bClassicView;
    lStatus  = RegQueryValueEx(hKey, g_szClassic, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bClassicView = (BOOL)dwData;

    dwData = (DWORD)g_bShowClassName;
    lStatus  = RegQueryValueEx(hKey, g_szClassName, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bShowClassName = (BOOL)dwData;

    dwData = (DWORD)g_bHideShellWnd;
    lStatus  = RegQueryValueEx(hKey, g_szShellWnd, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bHideShellWnd = (BOOL)dwData;

    dwData = (DWORD)g_bCpuPause;
    lStatus  = RegQueryValueEx(hKey, g_szCpuPause, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bCpuPause = (BOOL)dwData;

    dwData = (DWORD)g_bCpuDetails;
    lStatus  = RegQueryValueEx(hKey, g_szCpuDetails, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
        g_bCpuDetails = (BOOL)dwData;

    dwData = (DWORD)g_uInterval;
    lStatus  = RegQueryValueEx(hKey, g_szInterval, NULL, &dwType, (LPBYTE)&dwData, &dwSize);
    if (lStatus == ERROR_SUCCESS)
    {
        g_uInterval = (UINT)dwData;
        if (g_uInterval < 500 || g_uInterval > 3000)
            g_uInterval = TIMER2_PERIOD;
    }

    RegCloseKey(hKey);

    return lStatus;
}


LONG SaveSetting(LPCTSTR lpValueName, DWORD dwData)
{
    HKEY hKey;
    DWORD Disp;

    LONG lStatus = RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegPath, 0, NULL, 0, 0, NULL, &hKey, &Disp);
    if (lStatus != ERROR_SUCCESS)
        return lStatus;

    lStatus = RegSetValueEx(hKey, lpValueName, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));

    RegCloseKey(hKey);

    return lStatus;
}


LONG SaveRuntimeData(DWORD dwWorking, DWORD dwTickCount, __int64 llSuspend, __int64 llSysTime)
{
    HKEY hKey;
    DWORD Disp;

    LONG lStatus = RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegPath, 0, NULL, 0, 0, NULL, &hKey, &Disp);
    if (lStatus != ERROR_SUCCESS)
        return lStatus;

    DWORD dwTimeLo = (DWORD)(llSuspend & 0x00000000FFFFFFFF);
    DWORD dwTimeHi = (DWORD)((llSuspend >> 32) & 0x00000000FFFFFFFF);

    lStatus  = RegSetValueEx(hKey, g_szOperation, 0, REG_DWORD, (LPBYTE)&dwWorking, sizeof(DWORD));
    lStatus |= RegSetValueEx(hKey, g_szSuspendLo, 0, REG_DWORD, (LPBYTE)&dwTimeLo, sizeof(DWORD));
    lStatus |= RegSetValueEx(hKey, g_szSuspendHi, 0, REG_DWORD, (LPBYTE)&dwTimeHi, sizeof(DWORD));
    if (lStatus != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return lStatus;
    }

    dwTimeLo = (DWORD)(llSysTime & 0x00000000FFFFFFFF);
    dwTimeHi = (DWORD)((llSysTime >> 32) & 0x00000000FFFFFFFF);

    lStatus  = RegSetValueEx(hKey, g_szTickCount, 0, REG_DWORD, (LPBYTE)&dwTickCount, sizeof(DWORD));
    lStatus |= RegSetValueEx(hKey, g_szSysTimeLo, 0, REG_DWORD, (LPBYTE)&dwTimeLo, sizeof(DWORD));
    lStatus |= RegSetValueEx(hKey, g_szSysTimeHi, 0, REG_DWORD, (LPBYTE)&dwTimeHi, sizeof(DWORD));
    if (lStatus != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return lStatus;
    }

    RegCloseKey(hKey);

    return lStatus;
}


LONG LoadRuntimeData(LPDWORD pdwWorking, LPDWORD pdwTickCount, LPBOOL pbHandleRollover,
    __int64 *pllSuspend, __int64 *pllSysTime)
{
    HKEY hKey;
    LONG lStatus = RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegPath, 0, 0, &hKey);
    if (lStatus != ERROR_SUCCESS)
    {
        *pdwWorking = 0;
        *pllSuspend = 0i64;
        *pdwTickCount = 0;
        *pllSysTime = 0i64;
        *pbHandleRollover = TRUE;
        return lStatus;
    }

    DWORD dwTimeLo = 0;
    DWORD dwTimeHi = 0;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    lStatus  = RegQueryValueEx(hKey, g_szOperation, NULL, &dwType, (LPBYTE)pdwWorking, &dwSize);
    lStatus |= RegQueryValueEx(hKey, g_szSuspendLo, NULL, &dwType, (LPBYTE)&dwTimeLo, &dwSize);
    lStatus |= RegQueryValueEx(hKey, g_szSuspendHi, NULL, &dwType, (LPBYTE)&dwTimeHi, &dwSize);
    if (lStatus != ERROR_SUCCESS)
    {
        *pdwWorking = 0;
        *pllSuspend = 0i64;
        *pdwTickCount = 0;
        *pllSysTime = 0i64;
        *pbHandleRollover = TRUE;
        RegCloseKey(hKey);
        return lStatus;
    }

    *pllSuspend = dwTimeHi;
    *pllSuspend <<= 32;
    *pllSuspend |= dwTimeLo;

    dwTimeLo = 0;
    dwTimeHi = 0;
    lStatus  = RegQueryValueEx(hKey, g_szTickCount, NULL, &dwType, (LPBYTE)pdwTickCount, &dwSize);
    lStatus |= RegQueryValueEx(hKey, g_szSysTimeLo, NULL, &dwType, (LPBYTE)&dwTimeLo, &dwSize);
    lStatus |= RegQueryValueEx(hKey, g_szSysTimeHi, NULL, &dwType, (LPBYTE)&dwTimeHi, &dwSize);
    if (lStatus != ERROR_SUCCESS)
    {
        *pdwTickCount = 0;
        *pllSysTime = 0i64;
        *pbHandleRollover = TRUE;
        RegCloseKey(hKey);
        return lStatus;
    }

    DWORD dwTickCurrent = GetTickCount();
    DWORD dwTickDelta = dwTickCurrent - *pdwTickCount;
    if (dwTickCurrent < *pdwTickCount || dwTickDelta > WORKING_PERIOD)
    {
        *pdwTickCount = 0;
        *pllSysTime = 0i64;
        *pbHandleRollover = TRUE;
        RegCloseKey(hKey);
        return lStatus;
    }

    if (dwTickDelta > XSCALE_ROLLOVER_MIN && dwTickDelta < XSCALE_ROLLOVER_MAX)
        *pbHandleRollover = FALSE;
    else
        *pbHandleRollover = TRUE;

    *pllSysTime = dwTimeHi;
    *pllSysTime <<= 32;
    *pllSysTime |= dwTimeLo;

    RegCloseKey(hKey);

    return lStatus;
}


BOOL EnumDBase(PFILE_LIST pFileList)
{
    if (g_hLibCoreDll == NULL)
        return FALSE;

    HANDLE (*fnCeFindFirstDatabase)(DWORD);
    CEOID (*fnCeFindNextDatabase)(HANDLE);
    BOOL (*fnCeOidGetInfo)(CEOID, CEOIDINFO*);
    fnCeFindFirstDatabase = (HANDLE(*)(DWORD))GetProcAddress(g_hLibCoreDll, g_szCeFindFirstDatabase);
    fnCeFindNextDatabase = (CEOID(*)(HANDLE))GetProcAddress(g_hLibCoreDll, g_szCeFindNextDatabase);
    fnCeOidGetInfo = (BOOL(*)(CEOID, CEOIDINFO*))GetProcAddress(g_hLibCoreDll, g_szCeOidGetInfo);
    if (fnCeFindFirstDatabase == NULL || fnCeFindNextDatabase == NULL || fnCeOidGetInfo == NULL)
        return FALSE;

    CEOID ObjectID;
    CEOIDINFO CeObject;

    HANDLE hEnumObject = fnCeFindFirstDatabase(0);
    if (hEnumObject == INVALID_HANDLE_VALUE)
        return FALSE;

    while ((ObjectID = fnCeFindNextDatabase(hEnumObject)) != 0)
    {
        if (fnCeOidGetInfo(ObjectID, &CeObject) &&
            (CeObject.wObjType == OBJTYPE_DATABASE) &&
            (CeObject.infDatabase.dwFlags & CEDB_VALIDNAME))
        {
            if (CeObject.infDatabase.dwSize > pFileList->dwSize1)
            {
                pFileList->dwSize5 = pFileList->dwSize4;
                pFileList->dwSize4 = pFileList->dwSize3;
                pFileList->dwSize3 = pFileList->dwSize2;
                pFileList->dwSize2 = pFileList->dwSize1;
                pFileList->dwSize1 = CeObject.infDatabase.dwSize;
                _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                _tcscpy(pFileList->szFileName4, pFileList->szFileName3);
                _tcscpy(pFileList->szFileName3, pFileList->szFileName2);
                _tcscpy(pFileList->szFileName2, pFileList->szFileName1);
                if (CeObject.infDatabase.szDbaseName[0] == TEXT('\\'))
                    _tcscpy(pFileList->szFileName1, CeObject.infDatabase.szDbaseName + 1);
                else
                    _tcscpy(pFileList->szFileName1, CeObject.infDatabase.szDbaseName);
            }
            else if (CeObject.infDatabase.dwSize > pFileList->dwSize2)
            {
                pFileList->dwSize5 = pFileList->dwSize4;
                pFileList->dwSize4 = pFileList->dwSize3;
                pFileList->dwSize3 = pFileList->dwSize2;
                pFileList->dwSize2 = CeObject.infDatabase.dwSize;
                _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                _tcscpy(pFileList->szFileName4, pFileList->szFileName3);
                _tcscpy(pFileList->szFileName3, pFileList->szFileName2);
                if (CeObject.infDatabase.szDbaseName[0] == TEXT('\\'))
                    _tcscpy(pFileList->szFileName2, CeObject.infDatabase.szDbaseName + 1);
                else
                    _tcscpy(pFileList->szFileName2, CeObject.infDatabase.szDbaseName);
            }
            else if (CeObject.infDatabase.dwSize > pFileList->dwSize3)
            {
                pFileList->dwSize5 = pFileList->dwSize4;
                pFileList->dwSize4 = pFileList->dwSize3;
                pFileList->dwSize3 = CeObject.infDatabase.dwSize;
                _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                _tcscpy(pFileList->szFileName4, pFileList->szFileName3);
                if (CeObject.infDatabase.szDbaseName[0] == TEXT('\\'))
                    _tcscpy(pFileList->szFileName3, CeObject.infDatabase.szDbaseName + 1);
                else
                    _tcscpy(pFileList->szFileName3, CeObject.infDatabase.szDbaseName);
            }
            else if (CeObject.infDatabase.dwSize > pFileList->dwSize4)
            {
                pFileList->dwSize5 = pFileList->dwSize4;
                pFileList->dwSize4 = CeObject.infDatabase.dwSize;
                _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                if (CeObject.infDatabase.szDbaseName[0] == TEXT('\\'))
                    _tcscpy(pFileList->szFileName4, CeObject.infDatabase.szDbaseName + 1);
                else
                    _tcscpy(pFileList->szFileName4, CeObject.infDatabase.szDbaseName);
            }
            else if (CeObject.infDatabase.dwSize > pFileList->dwSize5)
            {
                pFileList->dwSize5 = CeObject.infDatabase.dwSize;
                if (CeObject.infDatabase.szDbaseName[0] == TEXT('\\'))
                    _tcscpy(pFileList->szFileName5, CeObject.infDatabase.szDbaseName + 1);
                else
                    _tcscpy(pFileList->szFileName5, CeObject.infDatabase.szDbaseName);
            }
        }
    }

    return TRUE;
}


int SearchDirectory(LPTSTR pszDir, PFILE_LIST pFileList)
{
    int nErr = 0;
    TCHAR szNew[MAX_PATH];
    TCHAR *pPtr, *pSrcSpec;

    for (pSrcSpec = pszDir + _tcslen(pszDir); pSrcSpec >= pszDir; pSrcSpec--)
        if (*pSrcSpec == TEXT('\\'))
            break;

    if (pSrcSpec <= pszDir)
        _tcscpy(szNew, TEXT("\\"));
    else
    {
        for (int i = 0; (i < (_countof(szNew))-10) && ((pszDir+i) <= pSrcSpec); i++)
            szNew[i] = *(pszDir+i);
        szNew[i] = TEXT('\0');
    }
    pPtr = szNew + _tcslen(szNew);

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(pszDir, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_INROM)))
            {
                if (fd.nFileSizeLow > pFileList->dwSize1)
                {
                    pFileList->dwSize5 = pFileList->dwSize4;
                    pFileList->dwSize4 = pFileList->dwSize3;
                    pFileList->dwSize3 = pFileList->dwSize2;
                    pFileList->dwSize2 = pFileList->dwSize1;
                    pFileList->dwSize1 = fd.nFileSizeLow;
                    _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                    _tcscpy(pFileList->szFileName4, pFileList->szFileName3);
                    _tcscpy(pFileList->szFileName3, pFileList->szFileName2);
                    _tcscpy(pFileList->szFileName2, pFileList->szFileName1);
                    _tcscpy(pFileList->szFileName1, szNew);
                    _tcscat(pFileList->szFileName1, fd.cFileName);
                }
                else if (fd.nFileSizeLow > pFileList->dwSize2)
                {
                    pFileList->dwSize5 = pFileList->dwSize4;
                    pFileList->dwSize4 = pFileList->dwSize3;
                    pFileList->dwSize3 = pFileList->dwSize2;
                    pFileList->dwSize2 = fd.nFileSizeLow;
                    _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                    _tcscpy(pFileList->szFileName4, pFileList->szFileName3);
                    _tcscpy(pFileList->szFileName3, pFileList->szFileName2);
                    _tcscpy(pFileList->szFileName2, szNew);
                    _tcscat(pFileList->szFileName2, fd.cFileName);
                }
                else if (fd.nFileSizeLow > pFileList->dwSize3)
                {
                    pFileList->dwSize5 = pFileList->dwSize4;
                    pFileList->dwSize4 = pFileList->dwSize3;
                    pFileList->dwSize3 = fd.nFileSizeLow;
                    _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                    _tcscpy(pFileList->szFileName4, pFileList->szFileName3);
                    _tcscpy(pFileList->szFileName3, szNew);
                    _tcscat(pFileList->szFileName3, fd.cFileName);
                }
                else if (fd.nFileSizeLow > pFileList->dwSize4)
                {
                    pFileList->dwSize5 = pFileList->dwSize4;
                    pFileList->dwSize4 = fd.nFileSizeLow;
                    _tcscpy(pFileList->szFileName5, pFileList->szFileName4);
                    _tcscpy(pFileList->szFileName4, szNew);
                    _tcscat(pFileList->szFileName4, fd.cFileName);
                }
                else if (fd.nFileSizeLow > pFileList->dwSize5)
                {
                    pFileList->dwSize5 = fd.nFileSizeLow;
                    _tcscpy(pFileList->szFileName5, szNew);
                    _tcscat(pFileList->szFileName5, fd.cFileName);
                }
            }
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
    else
    {
        int rc = GetLastError();
        if ((rc != ERROR_FILE_NOT_FOUND) && (rc != ERROR_NO_MORE_FILES))
            return -1;
    }

    _tcscat(szNew, TEXT("*"));
    hFind = FindFirstFile(szNew, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                !(fd.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY))
            {
                _tcscpy(pPtr, fd.cFileName); _tcscat(pPtr, pSrcSpec);
                nErr = SearchDirectory(szNew, pFileList);
                if (nErr) break;
                *pPtr = TEXT('\0');
            }
        } while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }
    else
    {
        int rc = GetLastError();
        if ((rc != ERROR_FILE_NOT_FOUND) && (rc != ERROR_NO_MORE_FILES))
            return -1;
    }

    return nErr;
}


DWORD UpdateStorageMedia(HWND hDlg, DWORD dwPage, BOOL bForceRefresh)
{
    static DWORD dwPageOld = 0;
    static DWORD dwCountOld = 0;
    static DWORD dwTotal0Old = 0;
    static DWORD dwTotal1Old = 0;
    static DWORD dwFilled0Old = 0;
    static DWORD dwFilled1Old = 0;
    static DWORD dwFree0Old = 0;
    static DWORD dwFree1Old = 0;
    static BYTE  cPercent0Old = 0;
    static BYTE  cPercent1Old = 0;

    DWORD dwCount = 0;

#if !defined(_WIN32_WCE_EMULATION) || (_WIN32_WCE > 201)
    DWORD dwTotal[2];
    dwTotal[0] = 0;
    dwTotal[1] = 0;

    BYTE cPercent[2];
    cPercent[0] = 0;
    cPercent[1] = 0;

    DWORD dwFilled[2];
    dwFilled[0] = 0;
    dwFilled[1] = 0;

    DWORD dwFree[2];
    dwFree[0] = 0;
    dwFree[1] = 0;

    TCHAR szDirectory[2][MAX_PATH];
    szDirectory[0][0] = TEXT('\0');
    szDirectory[1][0] = TEXT('\0');

    TCHAR szFilled[32];
    TCHAR szFree[32];
    TCHAR szText[64];

    WIN32_FIND_DATA fd;
    memset(&fd, 0, sizeof(fd));

    HANDLE hFind = FindFirstFile(TEXT("\\*"), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes &
                (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_TEMPORARY)) ==
                (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_TEMPORARY))
            {
                ULARGE_INTEGER FreeBytesAvailableToCaller;
                ULARGE_INTEGER TotalNumberOfBytes;
                ULARGE_INTEGER TotalNumberOfFreeBytes;

                if (GetDiskFreeSpaceEx(fd.cFileName, &FreeBytesAvailableToCaller,
                    &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
                {
                    if ((dwCount + 1) <= (2 * dwPage))
                    {
                        UINT uPos = (UINT)(dwCount % 2);
                        dwTotal[uPos] = TotalNumberOfBytes.HighPart > 1023 ?
                            0 : (DWORD)(TotalNumberOfBytes.QuadPart / 1024i64);
                        dwFree[uPos] = TotalNumberOfFreeBytes.HighPart > 1023 ?
                            0 : (DWORD)(TotalNumberOfFreeBytes.QuadPart / 1024i64);
                        dwFilled[uPos] = dwTotal[uPos] < dwFree[uPos] ?
                            0 : dwTotal[uPos] - dwFree[uPos];
                        cPercent[uPos] = (BYTE)100 - (BYTE)((TotalNumberOfBytes.QuadPart / 200i64 +
                            TotalNumberOfFreeBytes.QuadPart) / (TotalNumberOfBytes.QuadPart / 100i64));
                        _tcscpy(szDirectory[uPos], fd.cFileName);
                        _tcscat(szDirectory[uPos], TEXT(":"));
                    }
                    dwCount++;
                }
            }
        } while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

    if (!bForceRefresh && dwFree0Old == dwFree[0] && dwFree1Old == dwFree[1] &&
        dwFilled0Old == dwFilled[0] && dwFilled1Old == dwFilled[1] &&
        cPercent0Old == cPercent[0] && cPercent1Old == cPercent[1] &&
        dwPageOld == dwPage && dwCountOld == dwCount &&
        dwTotal0Old == dwTotal[0] && dwTotal1Old == dwTotal[1])
        return dwCount;

    LoadString(g_hInstance,  g_bGerUI ? IDS_GER_FILLED : IDS_ENG_FILLED, szFilled, _countof(szFilled));
    LoadString(g_hInstance,  g_bGerUI ? IDS_GER_FREE : IDS_ENG_FREE, szFree, _countof(szFree));

    BOOL bEnable1 = (dwCount == 0);
    BOOL bEnable2 = (dwCount == 0 || ((dwCount % 2) && ((dwCount + 1) <= (2 * dwPage))));

    if (bEnable1)
    {
        TCHAR szCard[32];
        LoadString(g_hInstance,  g_bGerUI ? IDS_GER_STORAGECARD : IDS_ENG_STORAGECARD, szDirectory[0], _countof(szDirectory[0]));
        wsprintf(szCard, TEXT(" %u:"), dwCount + 1);
        _tcscat(szDirectory[0], szCard);
        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_DIR_CARD1), szDirectory[0]);

        _tcscpy(szText, TEXT("---"));
        SetDlgItemText(hDlg, IDC_PERCENT_CARD1, szText);
        SetDlgItemText(hDlg, IDC_FILLED_CARD1, szFilled);
        SetDlgItemText(hDlg, IDC_FREE_CARD1, szFree);

        SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_CARD1), PBM_SETPOS, 0, 0);
    }
    else
    {
        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_DIR_CARD1), szDirectory[0]);

        KilobyteToString(dwTotal[0], szText);
        SetDlgItemText(hDlg, IDC_PERCENT_CARD1, szText);
        
        KilobyteToString(dwFilled[0], szText);
        _tcscat(szText, TEXT(" "));
        _tcscat(szText, szFilled);
        SetDlgItemText(hDlg, IDC_FILLED_CARD1, szText);

        KilobyteToString(dwFree[0], szText);
        _tcscat(szText, TEXT(" "));
        _tcscat(szText, szFree);
        SetDlgItemText(hDlg, IDC_FREE_CARD1, szText);

        SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_CARD1), PBM_SETPOS, (WPARAM)cPercent[0], 0);
    }

    if (bEnable2)
    {
        TCHAR szCard[32];
        LoadString(g_hInstance,  g_bGerUI ? IDS_GER_STORAGECARD : IDS_ENG_STORAGECARD, szDirectory[1], _countof(szDirectory[1]));
        wsprintf(szCard, TEXT(" %u:"), max(2, dwCount + 1));
        _tcscat(szDirectory[1], szCard);
        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_DIR_CARD2), szDirectory[1]);

        _tcscpy(szText, TEXT("---"));
        SetDlgItemText(hDlg, IDC_PERCENT_CARD2, szText);
        SetDlgItemText(hDlg, IDC_FILLED_CARD2, szFilled);
        SetDlgItemText(hDlg, IDC_FREE_CARD2, szFree);

        SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_CARD2), PBM_SETPOS, 0, 0);
    }
    else
    {
        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_DIR_CARD2), szDirectory[1]);

        KilobyteToString(dwTotal[1], szText);
        SetDlgItemText(hDlg, IDC_PERCENT_CARD2, szText);

        KilobyteToString(dwFilled[1], szText);
        _tcscat(szText, TEXT(" "));
        _tcscat(szText, szFilled);
        SetDlgItemText(hDlg, IDC_FILLED_CARD2, szText);

        KilobyteToString(dwFree[1], szText);
        _tcscat(szText, TEXT(" "));
        _tcscat(szText, szFree);
        SetDlgItemText(hDlg, IDC_FREE_CARD2, szText);

        SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_CARD2), PBM_SETPOS, (WPARAM)cPercent[1], 0);
    }

    bEnable1 = !bEnable1; bEnable2 = !bEnable2;
    EnableWindow(GetDlgItem(hDlg, IDC_DIR_CARD1), bEnable1);
    EnableWindow(GetDlgItem(hDlg, IDC_PERCENT_CARD1), bEnable1);
    EnableWindow(GetDlgItem(hDlg, IDC_PROGRESS_CARD1), bEnable1);
    EnableWindow(GetDlgItem(hDlg, IDC_FILLED_CARD1), bEnable1);
    EnableWindow(GetDlgItem(hDlg, IDC_FREE_CARD1), bEnable1);
    EnableWindow(GetDlgItem(hDlg, IDC_DIR_CARD2), bEnable2);
    EnableWindow(GetDlgItem(hDlg, IDC_PERCENT_CARD2), bEnable2);
    EnableWindow(GetDlgItem(hDlg, IDC_PROGRESS_CARD2), bEnable2);
    EnableWindow(GetDlgItem(hDlg, IDC_FILLED_CARD2), bEnable2);
    EnableWindow(GetDlgItem(hDlg, IDC_FREE_CARD2), bEnable2);
    EnableWindow(GetDlgItem(hDlg, IDC_NEXT), dwCount > 2);

    dwPageOld = dwPage;
    dwCountOld = dwCount;
    dwTotal0Old = dwTotal[0];
    dwTotal1Old = dwTotal[1];
    dwFilled0Old = dwFilled[0];
    dwFilled1Old = dwFilled[1];
    dwFree0Old = dwFree[0];
    dwFree1Old = dwFree[1];
    cPercent0Old = cPercent[0];
    cPercent1Old = cPercent[1];
#endif

    return dwCount;
}


void UpdateWndList(HWND hDlg, BOOL bInit, int nOldTopIndex, int nOldCurSel)
{
    static UINT uWndCountOld = (UINT)-1;
    int nTopIndex = 0;
    int nCurSel = 0;
    TCHAR szText[MAX_PATH];
    HWND hwndTaskList = GetDlgItem(hDlg, IDC_TASKLIST);
    if (bInit)
        nTopIndex = nOldTopIndex;
    else
        nTopIndex = ListBox_GetTopIndex(hwndTaskList);
    if (nTopIndex == LB_ERR) nTopIndex = 0;
    if (bInit)
        nCurSel = nOldCurSel;
    else
        nCurSel = ListBox_GetCurSel(hwndTaskList);
    if (nCurSel == LB_ERR) nCurSel = 0;

    PWINDOW_LIST pWndList = (PWINDOW_LIST)LocalAlloc(LPTR,
        sizeof(WINDOW_LIST) * MAX_WND_ENUM);
    if (pWndList == NULL)
    {
        uWndCountOld = (UINT)-1;
        ListBox_ResetContent(hwndTaskList);
        ListBox_SetCurSel(hwndTaskList, -1);
        LoadString(g_hInstance, g_bGerUI ? IDS_GER_MEMORY : IDS_ENG_MEMORY, szText, _countof(szText));
        SetDlgItemText(hDlg, IDC_TASKINFO, szText);
        return;
    }

    PWINDOW_LIST pWindowList = pWndList;

    WND_LIST_ENUM WndListEnum;
    WndListEnum.pWindowList = pWindowList;
    WndListEnum.uWndCount = 0;
    EnumWindows((WNDENUMPROC)EnumWindowsProc3, (LPARAM)&WndListEnum);

    SendMessage(hwndTaskList, WM_SETREDRAW, FALSE, 0);
    ListBox_ResetContent(hwndTaskList);
    ListBox_SetCurSel(hwndTaskList, -1);

    pWindowList = pWndList;
    UINT uWndCount = WndListEnum.uWndCount;
    if (uWndCount > 0)
    {
        for (UINT i = 0; i < uWndCount; i++)
        {
            int nIndex = ListBox_AddString(hwndTaskList,
                pWindowList->szWndTitle);
            ListBox_SetItemData(hwndTaskList, nIndex,
                (DWORD)pWindowList->hwndMain);

            pWindowList++;
        }

        if (nCurSel != LB_ERR)
            ListBox_SetCurSel(hwndTaskList, min(nCurSel, (int)(uWndCount-1)));
        ListBox_SetTopIndex(hwndTaskList, min(nTopIndex, (int)(uWndCount-1)));
    }

    SendMessage(hwndTaskList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndTaskList, NULL, TRUE);

    LocalFree((HLOCAL)pWndList);

    if (bInit || uWndCount != uWndCountOld)
    {
        TCHAR szWindows[64];
        LoadString(g_hInstance, g_bGerUI ? IDS_GER_WINDOWS : IDS_ENG_WINDOWS, szWindows, _countof(szWindows));
        wsprintf(szText, TEXT("%s: %u"), (LPTSTR)szWindows, uWndCount);

        uWndCountOld = uWndCount;

        SetDlgItemText(hDlg, IDC_TASKINFO, szText);
    }
}


void UpdateTaskList(HWND hDlg, BOOL bInit, int nOldTopIndex, int nOldCurSel)
{
    static DWORD dwProcessesOld = (DWORD)-1;
    static DWORD dwThreadsOld = (DWORD)-1;
    DWORD dwProcesses = 0;
    DWORD dwThreads = 0;

    int nTopIndex = 0;
    int nCurSel = 0;
    HWND hwndTaskList = GetDlgItem(hDlg, IDC_TASKLIST);
    if (bInit)
        nTopIndex = nOldTopIndex;
    else
        nTopIndex = ListBox_GetTopIndex(hwndTaskList);
    if (nTopIndex == LB_ERR) nTopIndex = 0;
    if (bInit)
        nCurSel = nOldCurSel;
    else
        nCurSel = ListBox_GetCurSel(hwndTaskList);
    if (nCurSel == LB_ERR) nCurSel = 0;

    TCHAR szText[MAX_PATH];
    szText[0] = TEXT('\0');

    if (g_fnCreateToolhelp32Snapshot == NULL || g_fnCloseToolhelp32Snapshot == NULL ||
        g_fnProcess32First == NULL || g_fnProcess32Next == NULL)
    {
        dwProcessesOld = (DWORD)-1; dwThreadsOld = (DWORD)-1;
        ListBox_ResetContent(hwndTaskList);
        ListBox_SetCurSel(hwndTaskList, -1);
        LoadString(g_hInstance, g_bGerUI ? IDS_GER_TOOLHELP : IDS_ENG_TOOLHELP, szText, _countof(szText));
        SetDlgItemText(hDlg, IDC_TASKINFO, szText);
        return;
    }

    PTASK_LIST pTasks = (PTASK_LIST)LocalAlloc(LPTR, sizeof(TASK_LIST) * MAX_TASKS);
    if (pTasks == NULL)
    {
        dwProcessesOld = (DWORD)-1; dwThreadsOld = (DWORD)-1;
        ListBox_ResetContent(hwndTaskList);
        ListBox_SetCurSel(hwndTaskList, -1);
        LoadString(g_hInstance, g_bGerUI ? IDS_GER_MEMORY : IDS_ENG_MEMORY, szText, _countof(szText));
        SetDlgItemText(hDlg, IDC_TASKINFO, szText);
        return;
    }

    HANDLE hSnapshot = g_fnCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPNOHEAPS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        LocalFree((HLOCAL)pTasks);
        dwProcessesOld = (DWORD)-1; dwThreadsOld = (DWORD)-1;
        ListBox_ResetContent(hwndTaskList);
        ListBox_SetCurSel(hwndTaskList, -1);
        LoadString(g_hInstance, g_bGerUI ? IDS_GER_SNAPSHOT : IDS_ENG_SNAPSHOT, szText, _countof(szText));
        SetDlgItemText(hDlg, IDC_TASKINFO, szText);
        return;
    }

    LPTSTR pszExeFile;
    PTASK_LIST pTaskList = pTasks;
    PROCESSENTRY32 ProcessEntry;
    memset(&ProcessEntry, 0, sizeof(ProcessEntry));
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

    BOOL bLoop = g_fnProcess32First(hSnapshot, &ProcessEntry);
    while (bLoop && (dwProcesses < MAX_TASKS))
    {
        pszExeFile = ProcessEntry.szExeFile;
        if (_tcsstr(ProcessEntry.szExeFile, TEXT("\\")))
        {
            pszExeFile = _tcsrchr(ProcessEntry.szExeFile, TEXT('\\'));
            pszExeFile++;
        }

        _tcscpy(pTaskList->szProcessName, pszExeFile);
        pTaskList->dwProcessId = ProcessEntry.th32ProcessID;

        pTaskList++;
        dwProcesses++;
        dwThreads += ProcessEntry.cntThreads;

        bLoop = g_fnProcess32Next(hSnapshot, &ProcessEntry);
    }

    g_fnCloseToolhelp32Snapshot(hSnapshot);

    SendMessage(hwndTaskList, WM_SETREDRAW, FALSE, 0);
    ListBox_ResetContent(hwndTaskList);
    ListBox_SetCurSel(hwndTaskList, -1);

    if (dwProcesses > 0)
    {
        pTaskList = pTasks;

        TASK_LIST_ENUM TaskListEnum;
        TaskListEnum.pTaskList = pTaskList;
        TaskListEnum.dwNumTasks = dwProcesses;

        EnumWindows((WNDENUMPROC)EnumWindowsProc2, (LPARAM)&TaskListEnum);

        for (DWORD i = 0; i < dwProcesses; i++)
        {
            _tcscpy(szText, pTaskList->szProcessName);
            if (pTaskList->szWindowTitle[0] != TEXT('\0'))
            {
                _tcsncat(szText, TEXT(" - "), MAX_PATH-_tcslen(szText)-1);
                _tcsncat(szText, pTaskList->szWindowTitle, MAX_PATH-_tcslen(szText)-1);
            }

            int nIndex = ListBox_AddString(hwndTaskList, szText);
            ListBox_SetItemData(hwndTaskList, nIndex, pTaskList->dwProcessId);

            pTaskList++;
        }

        if (nCurSel != LB_ERR)
            ListBox_SetCurSel(hwndTaskList, min(nCurSel, (int)(dwProcesses-1)));
        ListBox_SetTopIndex(hwndTaskList, min(nTopIndex, (int)(dwProcesses-1)));
    }

    SendMessage(hwndTaskList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndTaskList, NULL, TRUE);

    LocalFree((HLOCAL)pTasks);

    if (bInit || dwProcesses != dwProcessesOld || dwThreads != dwThreadsOld)
    {
        if (dwProcesses < MAX_TASKS)
        {
            TCHAR szProcesses[64]; TCHAR szThreads[64];
            LoadString(g_hInstance, g_bGerUI ? IDS_GER_PROCESSES : IDS_ENG_PROCESSES, szProcesses, _countof(szProcesses));
            LoadString(g_hInstance, g_bGerUI ? IDS_GER_THREADS : IDS_ENG_THREADS, szThreads, _countof(szThreads));
            wsprintf(szText, g_szFmtTasks, dwProcesses, (LPTSTR)szProcesses, dwThreads, (LPTSTR)szThreads);
        }
        else
        {
            TCHAR szMaxTasks[128];
            LoadString(g_hInstance, g_bGerUI ? IDS_GER_MAXTASKS : IDS_ENG_MAXTASKS, szMaxTasks, _countof(szMaxTasks));
            wsprintf(szText, szMaxTasks, dwProcesses);
        }
        
        dwProcessesOld = dwProcesses;
        dwThreadsOld = dwThreads;
        
        SetDlgItemText(hDlg, IDC_TASKINFO, szText);
    }
}


void ShowTaskWindow(HWND hDlg)
{
    int nIndex = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_TASKLIST));
    if (nIndex == LB_ERR)
        return;

    HWND hwndMain = (HWND)ListBox_GetItemData(GetDlgItem(hDlg, IDC_TASKLIST), nIndex);
    if ((DWORD)hwndMain == LB_ERR)
        return;

    if (!IsWindow(hwndMain))
    {
        TCHAR szText[128];
        if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_PROCESS : IDP_ENG_PROCESS,
            szText, _countof(szText)) > 0)
            MessageBox(hDlg, szText, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
        UpdateWndList(hDlg);
        return;
    }

    if (SetForegroundWindow(g_bPocketPC ? (HWND)(((DWORD)hwndMain) | 0x01) : hwndMain) &&
        hwndMain != g_hwndPropSheet)
    {
        ShowWindow(g_hwndPropSheet, SW_HIDE);
        if (g_hwndMenuBar != NULL) ShowWindow(g_hwndMenuBar, SW_HIDE);
        g_bHideState = TRUE;
    }
}


void ShowModuleList(HWND hDlg)
{
    HWND hwndTasklist = GetDlgItem(hDlg, IDC_TASKLIST);
    if (hwndTasklist == NULL)
        return;

    int nIndex = ListBox_GetCurSel(hwndTasklist);
    if (nIndex == LB_ERR)
        return;

    DWORD dwProcessId = ListBox_GetItemData(hwndTasklist, nIndex);
    if (dwProcessId == LB_ERR)
        return;

    HANDLE hProcess = OpenProcess(0, FALSE, dwProcessId);
    if (hProcess == NULL && GetLastError() != ERROR_ACCESS_DENIED)
    {
        TCHAR szMsg[256];
        if (LoadString(g_hInstance, g_bGerUI ? IDP_GER_PROCESS : IDP_ENG_PROCESS,
            szMsg, _countof(szMsg)) > 0)
            MessageBox(hDlg, szMsg, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
        UpdateTaskList(hDlg);
        return;
    }

    TCHAR szProcessName[MAX_PATH];
    szProcessName[0] = TEXT('\0');
    if (!GetModuleFileName(hProcess != NULL ? (HMODULE)hProcess : (HMODULE)dwProcessId,
        szProcessName, MAX_PATH))
        ListBox_GetText(hwndTasklist, nIndex, szProcessName);
    LPTSTR pszExeFile = szProcessName;
    if (_tcsstr(szProcessName, TEXT("\\")))
    {
        pszExeFile = _tcsrchr(szProcessName, TEXT('\\'));
        pszExeFile++;
    }

    if (hProcess != NULL)
        CloseHandle(hProcess);

    if (g_fnCreateToolhelp32Snapshot == NULL || g_fnCloseToolhelp32Snapshot == NULL ||
        g_fnModule32First == NULL || g_fnModule32Next == NULL)
        return;

    MODULEENTRY32 ModuleEntry;
    memset(&ModuleEntry, 0, sizeof(ModuleEntry));
    ModuleEntry.dwSize = sizeof(MODULEENTRY32);

    LPTSTR pszText = (LPTSTR)LocalAlloc(LPTR, (MAX_MODULES+1) * sizeof(TCHAR));
    if (pszText == NULL)
        return;

    HANDLE hSnapshot = g_fnCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        LocalFree((HLOCAL)pszText);
        return;
    }

    BOOL bLoop = g_fnModule32First(hSnapshot, &ModuleEntry);
    while (bLoop)
    {
        _tcsncat(pszText, ModuleEntry.szModule, MAX_MODULES-_tcslen(pszText));
        bLoop = g_fnModule32Next(hSnapshot, &ModuleEntry);
        if (bLoop)
            _tcsncat(pszText, TEXT(", "), MAX_MODULES-_tcslen(pszText));
    }
    g_fnCloseToolhelp32Snapshot(hSnapshot);

    if (_tcslen(pszText) == MAX_MODULES)
    {
        pszText[MAX_MODULES-1] = TEXT('.');
        pszText[MAX_MODULES-2] = TEXT('.');
        pszText[MAX_MODULES-3] = TEXT('.');
    }

    if (pszText[0] != TEXT('\0'))
        MessageBox(hDlg, pszText, pszExeFile, MB_OK);

    LocalFree((HLOCAL)pszText);
}


void UpdateSystemInfo(HWND hDlg)
{
    DWORD dwValue;
    TCHAR szText[256];
    TCHAR szText1[128];
    TCHAR szUnknown[64];

    BOOL bIsWinCE30 = FALSE;
    
    _tcscpy(szUnknown, TEXT("("));
    LoadString(g_hInstance, g_bGerUI ? IDS_GER_UNKNOWN : IDS_ENG_UNKNOWN, szText, _countof(szText));
    _tcscat(szUnknown, szText);
    _tcscat(szUnknown, TEXT(")"));

    {
        UINT uParam = sizeof(szText);
        if (!SystemParametersInfo(SPI_GETOEMINFO, uParam, (PVOID)szText, 0))
#ifdef _WIN32_WCE_EMULATION
            _tcscpy(szText, TEXT("Emulator"));
#else
            _tcscpy(szText, szUnknown);
#endif
        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_OEMINFO), szText);
    }

    {
        UINT uPlatform = sizeof(szText1);
        memset(szText1, 0, sizeof(szText1));
        if (SystemParametersInfo(SPI_GETPLATFORMTYPE, uPlatform, (PVOID)szText1, 0) ||
            SystemParametersInfo(SPI_GETPROJECTNAME, uPlatform, (PVOID)szText1, 0))
        {
            OSVERSIONINFO osvi;
            PLATFORMVERSION PlatformVer[2] = {{0, 0}, {0, 0}};
            UINT uiVersion = sizeof(PlatformVer);
            _tcscpy(szText, szText1);
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&osvi);
            if (((osvi.dwMajorVersion > 4) || (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion >= 20)) &&
                SystemParametersInfo(SPI_GETPLATFORMVERSION, uiVersion, (PVOID)&PlatformVer, 0))
            {
                LPTSTR pPlatformName = szText1;
                wsprintf(szText, TEXT("%s %u.%u"), pPlatformName,
                    PlatformVer[0].dwMajor, PlatformVer[0].dwMinor);
                if (PlatformVer[1].dwMajor || PlatformVer[1].dwMinor)
                {
                    pPlatformName += _tcslen(pPlatformName) + 1;
                    if (pPlatformName[0] != TEXT('\0'))
                    {
                        TCHAR szVersion[160];
                        wsprintf(szVersion, TEXT(", %s %u.%u"), pPlatformName,
                            PlatformVer[1].dwMajor, PlatformVer[1].dwMinor);
                        _tcsncat(szText, szVersion, 255-_tcslen(szText));
                    }
                }
            }
            if (g_bPocketPC && osvi.dwMajorVersion >= 4)
            {
                HKEY hKey;
                LONG lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegAku, 0, 0, &hKey);
                if (lStatus == ERROR_SUCCESS)
                {
                    szText1[0] = TEXT('\0');
                    DWORD dwType = REG_SZ;  
                    DWORD dwSize = sizeof(szText1);
                    lStatus = RegQueryValueEx(hKey, TEXT("Aku"), NULL, &dwType, (LPBYTE)szText1, &dwSize);
                    RegCloseKey(hKey);
                    if (lStatus == ERROR_SUCCESS && szText1[0] != TEXT('\0'))
                    {
                        _tcsncat(szText, TEXT(", AKU "), 255-_tcslen(szText));
                        _tcsncat(szText, (szText1[0] == TEXT('.')) ? szText1+1 : szText1,
                            255-_tcslen(szText));
                    }
                }
            }
        }
        else
            _tcscpy(szText, szUnknown);

        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_PLATFORM), szText);
    }

    {
        OSVERSIONINFO WinVer;
        WinVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx((LPOSVERSIONINFO)&WinVer))
        {
            if (WinVer.dwPlatformId == VER_PLATFORM_WIN32_CE)
            {
                if (WinVer.dwMajorVersion >= 6)
                    wsprintf(szText, g_szFmtWinEmbCe, WinVer.dwMajorVersion, WinVer.dwMinorVersion);
                else if (WinVer.dwMajorVersion == 4)
                    wsprintf(szText, g_szFmtWinCeNet, WinVer.dwMajorVersion, WinVer.dwMinorVersion);
                else
                    wsprintf(szText, g_szFmtWinCe, WinVer.dwMajorVersion, WinVer.dwMinorVersion);

                if (WinVer.dwMajorVersion >= 3)
                    bIsWinCE30 = TRUE;
            }
            else
                wsprintf(szText, g_szFmtWin, WinVer.dwMajorVersion, WinVer.dwMinorVersion);
            
            if (WinVer.dwBuildNumber)
            {
                wsprintf(szText1, TEXT(".%u"), WinVer.dwBuildNumber);
                _tcscat(szText, szText1);
            }
            
            if (WinVer.szCSDVersion[0] != TEXT('\0'))
            {
                _tcscat(szText, TEXT(" "));
                _tcscat(szText, WinVer.szCSDVersion);
            }
        }
        else
            _tcscpy(szText, szUnknown);

        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_OSVERSION), szText);
    }

    {
        dwValue = 0;
        if (g_fnGetSystemMemoryDivision != NULL)
        {
            DWORD dwStorePages, dwRamPages, dwPageSize;
            dwStorePages = dwRamPages = dwPageSize = 0;
            if (g_fnGetSystemMemoryDivision(&dwStorePages, &dwRamPages, &dwPageSize))
                dwValue = ((dwStorePages + dwRamPages) * dwPageSize) >> 10;
        }

        if (dwValue == 0)
        {
            if (g_fnGetStoreInformation != NULL)
            {
                STORE_INFORMATION si;
                if (g_fnGetStoreInformation(&si))
                    dwValue = si.dwStoreSize >> 10;
            }

            MEMORYSTATUS MemoryStatus;
            MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
            GlobalMemoryStatus(&MemoryStatus);
            
            dwValue += MemoryStatus.dwTotalPhys >> 10;
        }

        KilobyteToString(dwValue, szText);
        _tcscat(szText, TEXT(" RAM"));
        SetDlgItemText(hDlg, IDC_MEMORY, szText);
    }

    {
        dwValue = 0;
        if (bIsWinCE30 && GetProcessorInformation(szText1, szText, &dwValue))
        {
            TCHAR szCPUInfo[128];
            dwValue /= 1000000;

            szCPUInfo[0] = TEXT('\0');
            if (szText1[0] != TEXT('\0'))
            {
                _tcscpy(szCPUInfo, szText1);
                if (szText[0] != TEXT('\0'))
                {
                    _tcscat(szCPUInfo, TEXT(" "));
                    _tcscat(szCPUInfo, szText);
                }
            }
            else if (szText[0] != TEXT('\0'))
                _tcscpy(szCPUInfo, szText);

            if (szCPUInfo[0] != TEXT('\0'))
            {
                if (dwValue)
                {
                    wsprintf(szText, TEXT(" (%u MHz)"), dwValue);
                    _tcscat(szCPUInfo, szText);
                }
            }
            else
                _tcscpy(szCPUInfo, szUnknown);

            SetTextWithEllipsis(GetDlgItem(hDlg, IDC_PROCESSOR), szCPUInfo);
        }
        else
        {
            SYSTEM_INFO SystemInfo;
            SystemInfo.dwProcessorType = 0;
            GetSystemInfo(&SystemInfo);

            dwValue = SystemInfo.wProcessorArchitecture;
            switch (dwValue)
            {
                case PROCESSOR_ARCHITECTURE_ARM:
                    _tcscpy(szText1, TEXT("ARM"));
                    if (SystemInfo.wProcessorLevel > 0)
                    {
                        wsprintf(szText, TEXT("v%u"), SystemInfo.wProcessorLevel);
                        _tcscat(szText1, szText);
                    }
                    break;
                case PROCESSOR_ARCHITECTURE_MIPS:
                    _tcscpy(szText1, TEXT("MIPS"));
                    break;
                case PROCESSOR_ARCHITECTURE_SHX:
                    _tcscpy(szText1, TEXT("SuperH"));
                    break;
                case PROCESSOR_ARCHITECTURE_PPC:
                    _tcscpy(szText1, TEXT("PowerPC"));
                    break;
                case PROCESSOR_ARCHITECTURE_INTEL:
                    _tcscpy(szText1, TEXT("x86"));
                    break;
                default:
                    _tcscpy(szText1, szUnknown);
            }

            dwValue = SystemInfo.dwProcessorType;
            switch (dwValue)
            {
                case PROCESSOR_STRONGARM:
                    if (SystemInfo.wProcessorLevel == 4)
                        _tcscpy(szText, TEXT("StrongARM"));
                    else if (SystemInfo.wProcessorLevel == 5)
                        _tcscpy(szText, TEXT("XScale"));
                    else if (SystemInfo.wProcessorLevel == 6)
                        _tcscpy(szText, TEXT("ARM11"));
                    else
                        _tcscpy(szText, TEXT("ARM"));
                    break;
                case PROCESSOR_ARM720:
                    if (SystemInfo.wProcessorLevel == 5)
                        _tcscpy(szText, TEXT("XScale"));
                    else
                        _tcscpy(szText, TEXT("ARM7"));
                    break;
                case PROCESSOR_ARM820:
                    _tcscpy(szText, TEXT("ARM8"));
                    break;
                case PROCESSOR_ARM920:
                    _tcscpy(szText, TEXT("ARM9"));
                    break;
                case PROCESSOR_ARM_7TDMI:
                    _tcscpy(szText, TEXT("ARM7TDMI"));
                    break;
                case PROCESSOR_ARM11:
                    _tcscpy(szText, TEXT("ARM11"));
                    break;
                case PROCESSOR_ARM11MP:
                    _tcscpy(szText, TEXT("ARM11MP"));
                    break;
                case PROCESSOR_ARM_CORTEX:
                    _tcscpy(szText, TEXT("ARM Cortex"));
                    break;
                case PROCESSOR_MIPS_R5000:
                    _tcscpy(szText, TEXT("R5000"));
                    break;
                case PROCESSOR_MIPS_R4000:
                    _tcscpy(szText, TEXT("R4000"));
                    break;
                case PROCESSOR_MIPS_R3000:
                    _tcscpy(szText, TEXT("R3000"));
                    break;
                case PROCESSOR_MIPS_R2000:
                    _tcscpy(szText, TEXT("R2000"));
                    break;
                case PROCESSOR_HITACHI_SH3:
                case PROCESSOR_SHx_SH3:
                    _tcscpy(szText, TEXT("SH-3"));
                    break;
                case PROCESSOR_HITACHI_SH3E:
                    _tcscpy(szText, TEXT("SH-3E"));
                    break;
                case PROCESSOR_SHx_SH3DSP:
                    _tcscpy(szText, TEXT("SH-3 DSP"));
                    break;
                case PROCESSOR_HITACHI_SH4:
                case PROCESSOR_SHx_SH4:
                    _tcscpy(szText, TEXT("SH-4"));
                    break;
                case PROCESSOR_MOTOROLA_821:
                    _tcscpy(szText, TEXT("MPC821"));
                    break;
                case PROCESSOR_PPC_403:
                    _tcscpy(szText, TEXT("403"));
                    break;
                case PROCESSOR_INTEL_PENTIUMII:
                    _tcscpy(szText, TEXT("i686"));
                    break;
                case PROCESSOR_INTEL_PENTIUM:
                    _tcscpy(szText, TEXT("i586"));
                    break;
                case PROCESSOR_INTEL_486:
                    _tcscpy(szText, TEXT("i486"));
                    break;
                case PROCESSOR_INTEL_386:
                    _tcscpy(szText, TEXT("i386"));
                    break;
                case PROCESSOR_CEF:
                    _tcscpy(szText, TEXT("CEF"));
                    break;
                case 0:
                    _tcscpy(szText, szUnknown);
                    break;
                default:
                    wsprintf(szText, TEXT("%u"), dwValue);
            }

            _tcscat(szText1, TEXT(" "));
            _tcscat(szText1, szText);
            SetDlgItemText(hDlg, IDC_PROCESSOR, szText1);
        }
    }

    {
        UINT uValue1 = GetSystemMetrics(SM_CXSCREEN);
        UINT uValue2 = GetSystemMetrics(SM_CYSCREEN);

        wsprintf(szText, TEXT("%u×%u "), uValue1, uValue2);
        if (LoadString(g_hInstance, g_bGerUI ? IDS_GER_PIXELS : IDS_ENG_PIXELS, szText1, _countof(szText1)) > 0)
            _tcscat(szText, szText1);

        HDC hdcScreen = GetDC(NULL);
        UINT uValue3 = GetDeviceCaps(hdcScreen, BITSPIXEL) * GetDeviceCaps(hdcScreen, PLANES);
        COLORREF colref = GetNearestColor(hdcScreen, RGB(255, 0, 0));
        BOOL bGrayScale = ((GetRValue(colref) == GetBValue(colref)) &&
            (GetRValue(colref) == GetGValue(colref)));
        ReleaseDC(NULL, hdcScreen);

        if (uValue3 > 0 && uValue3 <= 128)
        {
            if (uValue3 < 24)
            {
                dwValue = ((DWORD)1 << (DWORD)uValue3);
                wsprintf(szText1, TEXT(", %u "), dwValue);
                _tcscat(szText, szText1);
                if (LoadString(g_hInstance,
                    g_bGerUI ? (bGrayScale ? IDS_GER_GRAYSCALE : IDS_GER_COLORS) : (bGrayScale ? IDS_ENG_GRAYSCALE : IDS_ENG_COLORS),
                    szText1, _countof(szText1)) > 0)
                    _tcscat(szText, szText1);
            }
            else
            {
                wsprintf(szText1, g_bGerUI ? TEXT(", %u Bit") : TEXT(", %u-bit"), uValue3);
                _tcscat(szText, szText1);
            }
        }

        SetTextWithEllipsis(GetDlgItem(hDlg, IDC_DISPLAY), szText);
    }
}


BOOL ResizeWindow(HWND hwndClient)
{
    if (hwndClient == NULL)
        return FALSE;
        
    HWND hwndParent = GetParent(hwndClient);
    if (hwndParent == NULL)
        return FALSE;

    RECT rcWindow, rcClient;
    GetWindowRect(hwndClient, &rcWindow);
    GetClientRect(hwndClient, &rcClient);

    int cxClient = rcClient.right - rcClient.left;
    int cyClient = rcClient.bottom - rcClient.top;
    int cxDiff = cxClient - ((cxClient / 10) * 10);
    int cyDiff = cyClient - ((cyClient / 3) * 3);

    POINT ptUpperLeft, ptLowerRight;
    ptUpperLeft.x = rcWindow.left;
    ptUpperLeft.y = rcWindow.top;
    ScreenToClient(hwndParent, &ptUpperLeft);
    ptLowerRight.x = rcWindow.right;
    ptLowerRight.y = rcWindow.bottom;
    ScreenToClient(hwndParent, &ptLowerRight);

    return MoveWindow(hwndClient,
        ptUpperLeft.x + (cxDiff / 2),
        ptUpperLeft.y + (cyDiff / 2),
        ptLowerRight.x - ptUpperLeft.x - cxDiff + 1,
        ptLowerRight.y - ptUpperLeft.y - cyDiff + 1,
        FALSE);
}


static DWORD MyGetWindowText(HWND hWnd, LPTSTR lpString, int nMaxCount)
{
    DWORD dwResult = 0;

    __try
    {
        if (g_fnSendMessageTimeout != NULL)
        {
            if (g_fnSendMessageTimeout(hWnd, WM_GETTEXT, nMaxCount,
                (LPARAM)lpString, SMTO_NORMAL, 500, &dwResult) == 0)
                dwResult = 0;
        }
        else
            dwResult = GetWindowText(hWnd, lpString, nMaxCount);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { ; }

    return dwResult;
}


static BOOL Stretch16x16To32x32(HDC hdcMem, int cxIcon, int cyIcon, DWORD dwRop)
{
    HDC hdcTemp = CreateCompatibleDC(hdcMem);
    if (hdcTemp == NULL)
        return FALSE;

    HBITMAP hbmTemp = CreateCompatibleBitmap(hdcMem, cxIcon, cyIcon);
    if (hbmTemp == NULL)
    {
        DeleteDC(hdcTemp);
        return FALSE;
    }

    HBITMAP hbmTempOld = (HBITMAP)SelectObject(hdcTemp, hbmTemp);

    PatBlt(hdcTemp, 0, 0, cxIcon, cyIcon, dwRop);
    StretchBlt(hdcTemp, (cxIcon-32)/2, (cyIcon-32)/2, 32, 32,
        hdcMem, (cxIcon-16)/2, (cyIcon-16)/2, 16, 16, SRCCOPY);
    BitBlt(hdcMem, 0, 0, cxIcon, cyIcon, hdcTemp, 0, 0, SRCCOPY);

    if (hbmTempOld != NULL)
        SelectObject(hdcTemp, hbmTempOld);

    DeleteObject(hbmTemp);
    DeleteDC(hdcTemp);

    return TRUE;
}


static BOOL GetProcessorInformation(LPTSTR lpszProcessorCore, LPTSTR lpszProcessorName, LPDWORD lpdwClockSpeed)
{
    DWORD dwReturn = FALSE;
    DWORD dwBytesReturned = 0;
    PROCESSOR_INFO CPUInfo;

    memset(&CPUInfo, 0, sizeof(CPUInfo));
    CPUInfo.wVersion = 1;

    __try
    {
#if !defined(_WIN32_WCE_EMULATION) || (_WIN32_WCE > 200)
        if (!KernelIoControl(IOCTL_PROCESSOR_INFORMATION, NULL, 0,
            &CPUInfo, sizeof(PROCESSOR_INFO), &dwBytesReturned))
#endif
            return FALSE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    if (lpszProcessorCore != NULL)
    {
        lpszProcessorCore[0] = TEXT('\0');
        if (CPUInfo.szProcessorCore[0] != TEXT('\0') &&
            CPUInfo.szProcessorCore[0] != (WCHAR)0xFFFF)
        {
            _tcscpy(lpszProcessorCore, CPUInfo.szProcessorCore);
            dwReturn = TRUE;
        }
    }

    if (lpszProcessorName != NULL)
    {
        lpszProcessorName[0] = TEXT('\0');
        if (CPUInfo.szProcessorName[0] != TEXT('\0') &&
            CPUInfo.szProcessorName[0] != (WCHAR)0xFFFF)
        {
            _tcscpy(lpszProcessorName, CPUInfo.szProcessorName);
            dwReturn = TRUE;
        }
    }

    if (lpdwClockSpeed != NULL)
    {
        if (CPUInfo.dwClockSpeed != 0xFFFFFFFF)
            *lpdwClockSpeed = CPUInfo.dwClockSpeed;
        else
            *lpdwClockSpeed = 0;
    }

    return dwReturn;
}


BOOL GetCPULoad(BYTE* pcPercent, UINT* puThreads)
{
    static DWORD dwTickCountOld = 0;
    static __int64 llTotalTimeOld = 0i64;
    static __int64 llTimeSpentOld = 0i64;

    if (pcPercent == NULL)
        return FALSE;

    if (g_fnCreateToolhelp32Snapshot == NULL || g_fnCloseToolhelp32Snapshot == NULL ||
        g_fnThread32First == NULL || g_fnThread32Next == NULL)
    {
        dwTickCountOld = 0;
        llTotalTimeOld = 0i64;
        llTimeSpentOld = 0i64;
        
        *pcPercent = (BYTE)-1;
        if (puThreads != NULL)
            *puThreads = 0;

        return FALSE;
    }

    HANDLE hSnapShot = g_fnCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE)
    {
        dwTickCountOld = 0;
        llTotalTimeOld = 0i64;
        llTimeSpentOld = 0i64;

        *pcPercent = (BYTE)-1;
        if (puThreads != NULL)
            *puThreads = 0;

        return FALSE;
    }

    DWORD dwTickCount = GetTickCount();
    __int64 llTotalTime = 0;
    __int64 llCreationTime, llExitTime, llKernelTime, llUserTime;

    DWORD dwAccess = 0;
    if (g_fnSetProcPermissions != NULL)
        dwAccess = g_fnSetProcPermissions((DWORD)-1);

    UINT uThreadCount = 0;
    UINT uMeasuredThreads = 0;
    THREADENTRY32 te;
    te.dwSize = sizeof(te);

    if (!g_fnThread32First(hSnapShot, &te))
    {
        dwTickCountOld = 0;
        llTotalTimeOld = 0i64;
        llTimeSpentOld = 0i64;
        
        *pcPercent = (BYTE)-1;
        if (puThreads != NULL)
            *puThreads = 0;

        if (g_fnSetProcPermissions != NULL)
            g_fnSetProcPermissions(dwAccess);
        g_fnCloseToolhelp32Snapshot(hSnapShot);

        return FALSE;
    }

    do
    {
        if (g_fnGetThreadTimes != NULL && g_fnGetThreadTimes((HANDLE)te.th32ThreadID, (FILETIME*)&llCreationTime,
            (FILETIME*)&llExitTime, (FILETIME*)&llKernelTime, (FILETIME*)&llUserTime))
        {
            llTotalTime += (llKernelTime + llUserTime);
            uMeasuredThreads++;
        }
        uThreadCount++;

    } while (g_fnThread32Next(hSnapShot, &te));

    if (g_fnSetProcPermissions != NULL)
        g_fnSetProcPermissions(dwAccess);

    g_fnCloseToolhelp32Snapshot(hSnapShot);

    if (uMeasuredThreads != uThreadCount || llTotalTimeOld == 0i64 || dwTickCountOld == 0i64)
    {
        dwTickCountOld = dwTickCount;
        llTotalTimeOld = llTotalTime;
        llTimeSpentOld = 0i64;

        *pcPercent = (BYTE)-1;
        if (puThreads != NULL)
            *puThreads = 0;

        return FALSE;
    }

    __int64 llTimeSpent = 0i64;
    __int64 llTickSpent = 0i64;

    if (llTotalTime > llTotalTimeOld)
    {
        llTimeSpent = llTotalTime - llTotalTimeOld;
        llTimeSpentOld = llTimeSpent;
    }
    else
    {
        llTimeSpent = llTimeSpentOld;
        llTimeSpentOld = 0i64;
    }

    if (dwTickCount > dwTickCountOld)
        llTickSpent = (dwTickCount - dwTickCountOld) * 10000;

    if (llTimeSpent > llTickSpent)
        llTimeSpent = llTickSpent;

    __try { *pcPercent = (BYTE)(int)(((double)llTimeSpent / (double)llTickSpent) * 100); }
    __except (EXCEPTION_EXECUTE_HANDLER) { *pcPercent = 0; }
    if (puThreads != NULL)
        *puThreads = uMeasuredThreads;

    llTotalTimeOld = llTotalTime;
    dwTickCountOld = dwTickCount;

    return TRUE;
}


static HFONT CreateSystemFont(int nDpi, LONG lHeight, LONG lWeight)
{
    LOGFONT lf;
    TCHAR lfFaceName[LF_FACESIZE];

    lfFaceName[0] = TEXT('\0');

    GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);

    if (lf.lfFaceName[0] != TEXT('\0'))
        _tcscpy(lfFaceName, lf.lfFaceName);

    memset((void *)&lf, 0, sizeof(lf));

    if (lfFaceName[0] != TEXT('\0'))
        _tcscpy(lf.lfFaceName, lfFaceName);

    lf.lfHeight = -((lHeight * nDpi) / 72);
    lf.lfWeight  = lWeight;

    return CreateFontIndirect(&lf);
}


static void KilobyteToString(DWORD dwDriveSize, LPTSTR szString)
{
    if (dwDriveSize < 1024)
        _stprintf(szString, TEXT("%u KB"), dwDriveSize);
    else if (dwDriveSize < 10240)
        _stprintf(szString, TEXT("%.2f MB"), (double)(dwDriveSize / 1024.0));
    else if (dwDriveSize < 102400)
        _stprintf(szString, TEXT("%.1f MB"), (double)(dwDriveSize / 1024.0));
    else if (dwDriveSize < 1048576)
        _stprintf(szString, TEXT("%.0f MB"), (double)(dwDriveSize / 1024.0));
    else if (dwDriveSize < 10485760)
        _stprintf(szString, TEXT("%.2f GB"), (double)(dwDriveSize / 1048576.0));
    else if (dwDriveSize < 104857600)
        _stprintf(szString, TEXT("%.1f GB"), (double)(dwDriveSize / 1048576.0));
    else if (dwDriveSize < 1073741824)
        _stprintf(szString, TEXT("%.0f GB"), (double)(dwDriveSize / 1048576.0));
    else
        _stprintf(szString, TEXT("%.2f TB"), (double)(dwDriveSize / 1073741824.0));
}


static void ByteToString(DWORD dwFileSize, LPTSTR szString)
{
    if (dwFileSize < 1024)
        _stprintf(szString, g_bGerUI ? TEXT("%u Byte") : TEXT("%u byte"), dwFileSize);
    else if (dwFileSize < 10240)
        _stprintf(szString, TEXT("%.2f KB"), (double)(dwFileSize / 1024.0));
    else if (dwFileSize < 102400)
        _stprintf(szString, TEXT("%.1f KB"), (double)(dwFileSize / 1024.0));
    else if (dwFileSize < 1048576)
        _stprintf(szString, TEXT("%.0f KB"), (double)(dwFileSize / 1024.0));
    else if (dwFileSize < 10485760)
        _stprintf(szString, TEXT("%.2f MB"), (double)(dwFileSize / 1048576.0));
    else if (dwFileSize < 104857600)
        _stprintf(szString, TEXT("%.1f MB"), (double)(dwFileSize / 1048576.0));
    else if (dwFileSize < 1073741824)
        _stprintf(szString, TEXT("%.0f MB"), (double)(dwFileSize / 1048576.0));
    else
        _stprintf(szString, TEXT("%.2f GB"), (double)(dwFileSize / 1073741824.0));
}


static BOOL SetTextWithEllipsis(HWND hwnd, LPCTSTR lpszText)
{
    RECT rc;
    SIZE size;
    int nFit = 0;

    if (hwnd == NULL || lpszText == NULL)
        return FALSE;

    int nTextLen = _tcslen(lpszText);
    GetClientRect(hwnd, &rc);

    HDC hdc = GetDC(hwnd);
    GetTextExtentExPoint(hdc, lpszText, nTextLen, rc.right-rc.left, &nFit, NULL, &size);
    ReleaseDC(hwnd, hdc);

    if (nTextLen <= nFit || nFit <= 6)
        return SetWindowText(hwnd, lpszText);

    TCHAR szNew[128];
    _tcsncpy(szNew, lpszText, 127);
    szNew[nFit-3] = TEXT('.');
    szNew[nFit-2] = TEXT('.');
    szNew[nFit-1] = TEXT('.');
    szNew[nFit] = TEXT('\0');

    return SetWindowText(hwnd, szNew);
}


static BOOL ExecPegHelp(LPCTSTR szHelpCmdPart)
{
    TCHAR szCommand[64];
    PROCESS_INFORMATION ProcInfo;

    _tcscpy(szCommand, g_szHelpCmdPart);
    _tcscat(szCommand, szHelpCmdPart);
    _tcscat(szCommand, g_bGerUI ? g_szHelpGer : g_szHelpEng);

    if (!CreateProcess(g_szPegHelp, szCommand, NULL,
        NULL, FALSE, 0, NULL, NULL, NULL, &ProcInfo))
        return FALSE;

    CloseHandle(ProcInfo.hThread);
    CloseHandle(ProcInfo.hProcess);

    return TRUE;
}
