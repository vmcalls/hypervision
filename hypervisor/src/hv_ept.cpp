#include "../stdafx.h"

static const ULONG ept_tag = 'tpeH'; // 'Hpet'

NTSTATUS hv_ept::build_identity_map( )
{
    hv_logger::log( hv_logger::level::info, "hv_ept::build_identity_map starting" );

    // In a complete code we would here abuse the 4 level hierarchy of pages but in this case we just allocate four to mimic that ^^
    const ULONG levels = 4;
    ept_pml4_ = ExAllocatePoolWithTag( NonPagedPoolNx, PAGE_SIZE * levels, ept_tag );
    if ( !ept_pml4_ ) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory( ept_pml4_, PAGE_SIZE * levels );
    page_count_ = levels;
    alloc_bytes_ = PAGE_SIZE * levels;

    hv_logger::log( hv_logger::level::info, "hv_ept::build_identity_map allocated %llu bytes (%llu pages)", alloc_bytes_, page_count_ );

    // dummy entries ofc
    ULONG64* table = reinterpret_cast< ULONG64* >( ept_pml4_ );
    for ( ULONG i = 0; i < 512; ++i )
    {
        // we mark entry as "present" and point to a dummy physical address
        table[ i ] = ( static_cast< ULONG64 >( i ) << 12 ) | 0x7ULL; // bits 0-2 as R/W/X
    }

    hv_logger::log( hv_logger::level::info, "hv_ept::build_identity_map: filled dummy entries" );
    return STATUS_SUCCESS;
}

void hv_ept::destroy( )
{
    if ( ept_pml4_ )
    {
        ExFreePoolWithTag( ept_pml4_, ept_tag );
        ept_pml4_ = nullptr;
        page_count_ = 0;
        alloc_bytes_ = 0;
        hv_logger::log( hv_logger::level::info, "hv_ept::destroy: freed memory" );
    }
}