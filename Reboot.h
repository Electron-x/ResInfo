#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <winioctl.h>

#ifndef EWX_REBOOT
#define EWX_REBOOT 2
#endif
#ifndef POWER_STATE_RESET
#define POWER_STATE_RESET (DWORD)(0x00800000)
#endif
#ifndef POWER_FORCE
#define POWER_FORCE       (DWORD)(0x00001000)
#endif
#ifndef IOCTL_HAL_REBOOT
#define IOCTL_HAL_REBOOT CTL_CODE(FILE_DEVICE_HAL, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifdef __cplusplus
}
#endif
