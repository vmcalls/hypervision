#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" 
{
#endif

#define HV_DEVICE_LINK "\\\\.\\hv_device"
#define HV_IOCTL_BASE 0x800

#define IOCTL_HV_NOP           CTL_CODE(FILE_DEVICE_UNKNOWN, HV_IOCTL_BASE + 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HV_QUERY_CAPS    CTL_CODE(FILE_DEVICE_UNKNOWN, HV_IOCTL_BASE + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HV_BUILD_EPT     CTL_CODE(FILE_DEVICE_UNKNOWN, HV_IOCTL_BASE + 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_HV_SANDBOX_CREATE  CTL_CODE(FILE_DEVICE_UNKNOWN, HV_IOCTL_BASE + 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HV_SANDBOX_DESTROY CTL_CODE(FILE_DEVICE_UNKNOWN, HV_IOCTL_BASE + 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HV_SANDBOX_LIST    CTL_CODE(FILE_DEVICE_UNKNOWN, HV_IOCTL_BASE + 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

    typedef struct _hv_vmx_caps
    {
        BOOLEAN vmx_supported;            // 0 or 1
        ULONG cpu_count;                  // number of logical processors observed by driver
        ULONG suggested_region_size;      // vmxon/vmcs region suggestion (bytes)
        ULONG ept_page_count;             // pages allocated for demo EPT (if any)
        ULONG sandbox_count;              // active sandboxes
    } hv_vmx_caps;

    typedef struct _hv_sandbox_request
    {
        ULONG id;
    } hv_sandbox_request;

    typedef struct _hv_sandbox_list_result
    {
        ULONG count;
    } hv_sandbox_list_result;

#ifdef __cplusplus
}
#endif