#include "stdafx.h"

extern "C" NTSTATUS
DriverEntry( _In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path )
{
    UNUSED( registry_path );

    hv_logger::initialize( );
    hv_logger::log( hv_logger::level::info, "driver_entry: loading hypervisor driver" );

    driver_object->DriverUnload = DriverUnload;

    hv_logger::log( hv_logger::level::info, "driver_entry: initialization complete" );
    return STATUS_SUCCESS;
}

extern "C" VOID
DriverUnload( _In_ PDRIVER_OBJECT driver_object )
{
    UNUSED( driver_object );

    hv_logger::log( hv_logger::level::info, "driver_unload: unloading hypervisor driver" );
    hv_logger::shutdown( );
}