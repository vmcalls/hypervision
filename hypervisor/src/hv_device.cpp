#include "../stdafx.h"

static const ULONG device_tag = 'dVh0';

NTSTATUS hv_device::create( _In_ PDRIVER_OBJECT driver_object )
{
    UNICODE_STRING device_name;
    UNICODE_STRING sym_link;
    PDEVICE_OBJECT device_object = nullptr;
    NTSTATUS status = STATUS_SUCCESS;

    RtlInitUnicodeString( &device_name, HV_DEVICE_NAME );
    RtlInitUnicodeString( &sym_link, HV_SYMBOLIC_NAME );

    status = IoCreateDevice( driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object );
    if ( !NT_SUCCESS( status ) )
    {
        hv_logger::log( hv_logger::level::error, "hv_device::create: IoCreateDevice failed 0x%08x", status );
        return status;
    }

    device_object->Flags |= DO_BUFFERED_IO;
    device_object->Flags &= ~DO_DEVICE_INITIALIZING;

    status = IoCreateSymbolicLink( &sym_link, &device_name );
    if ( !NT_SUCCESS( status ) )
    {
        hv_logger::log( hv_logger::level::error, "hv_device::create: IoCreateSymbolicLink failed 0x%08x", status );
        IoDeleteDevice( device_object );
        return status;
    }

    // IRP handlers
    driver_object->MajorFunction[ IRP_MJ_CREATE ] = [ ]( PDEVICE_OBJECT dev, PIRP irp ) -> NTSTATUS
        {
            UNREFERENCED_PARAMETER( dev );
            return hv_device::dispatch_create_close( dev, irp );
        };

    driver_object->MajorFunction[ IRP_MJ_CLOSE ] = [ ]( PDEVICE_OBJECT dev, PIRP irp ) -> NTSTATUS
        {
            UNREFERENCED_PARAMETER( dev );
            return hv_device::dispatch_create_close( dev, irp );
        };

    driver_object->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = [ ]( PDEVICE_OBJECT dev, PIRP irp ) -> NTSTATUS
        {
            return hv_device::dispatch_device_control( dev, irp );
        };

    hv_logger::log( hv_logger::level::info, "hv_device::create: device and symbolic link created" );
    return STATUS_SUCCESS;
}

void hv_device::destroy( _In_ PDRIVER_OBJECT driver_object )
{
    UNICODE_STRING sym_link;
    PDEVICE_OBJECT device_object = driver_object->DeviceObject;

    RtlInitUnicodeString( &sym_link, HV_SYMBOLIC_NAME );

    NTSTATUS status = IoDeleteSymbolicLink( &sym_link );
    if ( !NT_SUCCESS( status ) )
    {
        hv_logger::log( hv_logger::level::warning, "hv_device::destroy: IoDeleteSymbolicLink returned 0x%08x", status );
    }

    while ( device_object )
    {
        PDEVICE_OBJECT next = device_object->NextDevice;
        IoDeleteDevice( device_object );
        device_object = next;
    }

    hv_logger::log( hv_logger::level::info, "hv_device::destroy: device(s) deleted" );
}

NTSTATUS hv_device::dispatch_create_close( _In_ PDEVICE_OBJECT device_object, _In_ PIRP irp )
{
    UNREFERENCED_PARAMETER( device_object );

    hv_logger::log( hv_logger::level::info, "hv_device::dispatch_create_close: IRP received" );
    complete_irp_success( irp, 0 );
    return STATUS_SUCCESS;
}

NTSTATUS hv_device::dispatch_device_control( _In_ PDEVICE_OBJECT device_object, _In_ PIRP irp )
{
    UNREFERENCED_PARAMETER( device_object );

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation( irp );
    ULONG io_control_code = stack->Parameters.DeviceIoControl.IoControlCode;

    hv_logger::log( hv_logger::level::info, "hv_device::dispatch_device_control: ioctl 0x%08x", io_control_code );

    switch ( io_control_code )
    {
    case IOCTL_HV_NOP:
    {
        hv_logger::log( hv_logger::level::info, "hv_device::dispatch_device_control: IOCTL_HV_NOP" );
        complete_irp_success( irp, 0 );
        return STATUS_SUCCESS;
    }

    default:
    {
        hv_logger::log( hv_logger::level::warning, "hv_device::dispatch_device_control: unknown ioctl 0x%08x", io_control_code );
        complete_irp_error( irp, STATUS_INVALID_DEVICE_REQUEST, 0 );
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    }
}

void hv_device::complete_irp_success( _In_ PIRP irp, ULONG_PTR information )
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = information;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
}

void hv_device::complete_irp_error( _In_ PIRP irp, NTSTATUS status, ULONG_PTR information )
{
    irp->IoStatus.Status = status;
    irp->IoStatus.Information = information;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
}