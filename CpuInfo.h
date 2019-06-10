/*++

Module Name:

    cpuinfo.h

Abstract:

    Definitions for processor information returned by
    IOCTL_PROCESSOR_INFORMATION

Revision History:

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <winioctl.h>

typedef struct _PROCESSOR_INFO {
	WORD	wVersion; 
	WCHAR	szProcessorCore[40];
	WORD	wCoreRevision;
	WCHAR	szProcessorName[40];
	WORD	wProcessorRevision;
	WCHAR	szCatalogNumber[100];
	WCHAR	szVendor[100];
	DWORD	dwInstructionSet;
	DWORD	dwClockSpeed;
} PROCESSOR_INFO, *PPROCESSOR_INFO;


#ifndef IOCTL_PROCESSOR_INFORMATION
#define IOCTL_PROCESSOR_INFORMATION CTL_CODE(FILE_DEVICE_HAL, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)
BOOL KernelIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);
#endif

#ifdef __cplusplus
}
#endif
