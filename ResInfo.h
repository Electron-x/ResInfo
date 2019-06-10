
#if !defined(AFX_RESINFO_H__14DC14BB_78FD_4036_891C_926D44DDFD25__INCLUDED_)
#define AFX_RESINFO_H__14DC14BB_78FD_4036_891C_926D44DDFD25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

#ifndef ICON_SMALL
#define ICON_SMALL 0
#endif
#ifndef ICON_BIG
#define ICON_BIG   1
#endif
#ifndef SETTINGCHANGE_RESET
#define SETTINGCHANGE_RESET 0x3002
#endif
#ifndef PSH_MAXIMIZE
#define PSH_MAXIMIZE      0x2000
#endif
#ifndef PSCB_GETVERSION
#define PSCB_GETVERSION   3
#endif
#ifndef PSCB_GETTITLE
#define PSCB_GETTITLE     4
#endif
#ifndef PSCB_GETLINKTEXT
#define PSCB_GETLINKTEXT  5
#endif
#ifndef COMCTL32_VERSION
#define COMCTL32_VERSION  0x020c
#endif
#ifndef BATTERY_CHEMISTRY_ALKALINE
#define BATTERY_CHEMISTRY_ALKALINE 0x01
#endif
#ifndef BATTERY_CHEMISTRY_NICD
#define BATTERY_CHEMISTRY_NICD     0x02
#endif
#ifndef BATTERY_CHEMISTRY_NIMH
#define BATTERY_CHEMISTRY_NIMH     0x03
#endif
#ifndef BATTERY_CHEMISTRY_LION
#define BATTERY_CHEMISTRY_LION     0x04
#endif
#ifndef BATTERY_CHEMISTRY_LIPOLY
#define BATTERY_CHEMISTRY_LIPOLY   0x05
#endif
#ifndef BATTERY_CHEMISTRY_ZINCAIR
#define BATTERY_CHEMISTRY_ZINCAIR  0x06
#endif
#ifndef BATTERY_CHEMISTRY_UNKNOWN
#define BATTERY_CHEMISTRY_UNKNOWN  0xFF
#endif
#ifndef SPI_GETPROJECTNAME
#define SPI_GETPROJECTNAME  259
#endif
#ifndef SPI_GETPLATFORMNAME
#define SPI_GETPLATFORMNAME 260
#endif

#ifndef TH32CS_SNAPNOHEAPS
#define TH32CS_SNAPNOHEAPS  0x40000000
#endif

#ifndef SMTO_NORMAL
#define SMTO_NORMAL 0x0000
#endif


#define MAX_TASKS       42
#define MAX_WND_ENUM   124
#define MAX_MODULES   1022
#define MAX_WND_TITLE  128

typedef struct _TASK_LIST
{
    DWORD dwProcessId;
    TCHAR szProcessName[MAX_PATH];
    TCHAR szWindowTitle[MAX_PATH];
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM
{
    PTASK_LIST pTaskList;
    DWORD      dwNumTasks;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;

typedef struct _WINDOW_LIST
{
    HWND hwndMain;
    TCHAR szWndTitle[MAX_WND_TITLE];
} WINDOW_LIST, *PWINDOW_LIST;

typedef struct _WND_LIST_ENUM
{
    PWINDOW_LIST pWindowList;
    UINT         uWndCount;
} WND_LIST_ENUM, *PWND_LIST_ENUM;

typedef struct _FILE_LIST
{
    DWORD dwSize1;
    DWORD dwSize2;
    DWORD dwSize3;
    DWORD dwSize4;
    DWORD dwSize5;
    TCHAR szFileName1[MAX_PATH];
    TCHAR szFileName2[MAX_PATH];
    TCHAR szFileName3[MAX_PATH];
    TCHAR szFileName4[MAX_PATH];
    TCHAR szFileName5[MAX_PATH];
} FILE_LIST, *PFILE_LIST;

#if (_WIN32_WCE <= 211)
typedef struct _SYSTEM_POWER_STATUS_EX2
{
    BYTE ACLineStatus;
    BYTE BatteryFlag;
    BYTE BatteryLifePercent;
    BYTE Reserved1;
    DWORD BatteryLifeTime;
    DWORD BatteryFullLifeTime;
    BYTE Reserved2;
    BYTE BackupBatteryFlag;
    BYTE BackupBatteryLifePercent;
    BYTE Reserved3;
    DWORD BackupBatteryLifeTime;
    DWORD BackupBatteryFullLifeTime;
    DWORD BatteryVoltage;
    DWORD BatteryCurrent;
    DWORD BatteryAverageCurrent;
    DWORD BatteryAverageInterval;
    DWORD BatterymAHourConsumed;
    DWORD BatteryTemperature;
    DWORD BackupBatteryVoltage;
    BYTE  BatteryChemistry;
} SYSTEM_POWER_STATUS_EX2, *PSYSTEM_POWER_STATUS_EX2, *LPSYSTEM_POWER_STATUS_EX2;
#endif

#define SHCMBF_EMPTYBAR      0x0001
#define SHCMBF_HIDDEN        0x0002
#define SHCMBF_HIDESIPBUTTON 0x0004

typedef struct tagSHMENUBARINFO
{
    DWORD cbSize;
    HWND hwndParent;
    DWORD dwFlags;
    UINT nToolBarId;
    HINSTANCE hInstRes;
    int nBmpId;
    int cBmpImages;
    HWND hwndMB;
} SHMENUBARINFO, *PSHMENUBARINFO;

#ifndef SPI_GETPLATFORMVERSION
#define SPI_GETPLATFORMVERSION 224

typedef struct tagPLATFORMVERSION
{
    DWORD dwMajor;
    DWORD dwMinor;
} PLATFORMVERSION, *PPLATFORMVERSION;

#endif

#endif // !defined(AFX_RESINFO_H__14DC14BB_78FD_4036_891C_926D44DDFD25__INCLUDED_)
