#pragma once

extern "C"
{
    DRIVER_INITIALIZE DriverEntry;
    DRIVER_UNLOAD DriverUnload;
}

#define HV_DEVICE_NAME      L"\\Device\\hv_device"
#define HV_SYMBOLIC_NAME    L"\\DosDevices\\hv_device"