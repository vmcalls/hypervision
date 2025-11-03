#pragma once

class hv_device
{
public:
    static NTSTATUS create( _In_ PDRIVER_OBJECT driver_object );
    static void destroy( _In_ PDRIVER_OBJECT driver_object );

private:
    static NTSTATUS dispatch_create_close( _In_ PDEVICE_OBJECT device_object, _In_ PIRP irp );
    static NTSTATUS dispatch_device_control( _In_ PDEVICE_OBJECT device_object, _In_ PIRP irp );

    static void complete_irp_success( _In_ PIRP irp, ULONG_PTR information = 0 );
    static void complete_irp_error( _In_ PIRP irp, NTSTATUS status, ULONG_PTR information = 0 );
};