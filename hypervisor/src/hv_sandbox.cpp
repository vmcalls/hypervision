#include "../stdafx.h"

struct scoped_spin_lock
{
    KSPIN_LOCK* lock{ nullptr };
    KIRQL       old_irql{ PASSIVE_LEVEL };

    explicit scoped_spin_lock( KSPIN_LOCK* l ) : lock( l )
    {
        KeAcquireSpinLock( lock, &old_irql );
    }

    ~scoped_spin_lock( )
    {
        KeReleaseSpinLock( lock, old_irql );
    }
};

NTSTATUS hv_sandbox_manager::initialize( )
{
    KeInitializeSpinLock( &lock_ );
    RtlZeroMemory( entries_, sizeof( entries_ ) );
    hv_logger::log( hv_logger::level::info, "hv_sandbox_manager::initialize: ready (capacity=%u)", max_sandboxes_ );
    return STATUS_SUCCESS;
}

void hv_sandbox_manager::shutdown( )
{
    scoped_spin_lock guard( &lock_ );
    for ( ULONG i = 0; i < max_sandboxes_; ++i )
    {
        if ( entries_[ i ].active )
        {
            entries_[ i ].ept.destroy( );
            entries_[ i ].active = FALSE;
            entries_[ i ].id = 0;
            entries_[ i ].created.QuadPart = 0;
        }
    }

    hv_logger::log( hv_logger::level::info, "hv_sandbox_manager::shutdown: all sandboxes cleared" );
}

NTSTATUS hv_sandbox_manager::create_sandbox( _In_ ULONG id )
{
    if ( id == 0 ) return STATUS_INVALID_PARAMETER;
    scoped_spin_lock guard( &lock_ );
    if ( find_entry_by_id( id ) >= 0 ) return STATUS_OBJECT_NAME_COLLISION;

    LONG free_index = find_free_slot( );
    if ( free_index < 0 ) return STATUS_INSUFFICIENT_RESOURCES;

    // we build a test ept for our sandbox (this is data structure only ofc)
    NTSTATUS status = entries_[ free_index ].ept.build_identity_map( );
    if ( !NT_SUCCESS( status ) )
    {
        hv_logger::log( hv_logger::level::error, "hv_sandbox_manager::create_sandbox: ept build failed (0x%08x)", status );
        return status;
    }

    entries_[ free_index ].id = id;
    entries_[ free_index ].active = TRUE;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    KeQuerySystemTimePrecise( &entries_[ free_index ].created );
#else
    KeQuerySystemTime( &entries_[ free_index ].created );
#endif

    hv_logger::log( hv_logger::level::info,
        "hv_sandbox_manager::create_sandbox: id=%u created (ept_pages=%llu, bytes=%llu)",
        id,
        entries_[ free_index ].ept.get_page_count( ),
        entries_[ free_index ].ept.get_alloc_bytes( ) );

    return STATUS_SUCCESS;
}

NTSTATUS hv_sandbox_manager::destroy_sandbox( _In_ ULONG id )
{
    if ( id == 0 ) return STATUS_INVALID_PARAMETER;

    scoped_spin_lock guard( &lock_ );
    LONG idx = find_entry_by_id( id );
    if ( idx < 0 ) return STATUS_NOT_FOUND;

    entries_[ idx ].ept.destroy( );
    entries_[ idx ].active = FALSE;
    entries_[ idx ].id = 0;
    entries_[ idx ].created.QuadPart = 0;

    hv_logger::log( hv_logger::level::info, "hv_sandbox_manager::destroy_sandbox: id=%u destroyed", id );
    return STATUS_SUCCESS;
}

NTSTATUS hv_sandbox_manager::list_sandboxes( _Out_writes_opt_( max_ids ) ULONG* out_ids, _In_ ULONG max_ids, _Out_opt_ ULONG* out_count ) const
{
    ULONG needed = 0;
    {
        scoped_spin_lock guard( const_cast< KSPIN_LOCK* >( &lock_ ) );

        for ( ULONG i = 0; i < max_sandboxes_; ++i )
        {
            if ( entries_[ i ].active )
            {
                if ( out_ids && needed < max_ids ) 
                out_ids[ needed ] = entries_[ i ].id;
                ++needed;
            }
        }
    }

    if ( out_count ) *out_count = needed;
    if ( out_ids && max_ids < needed ) return STATUS_BUFFER_TOO_SMALL;
    return STATUS_SUCCESS;
}

ULONG hv_sandbox_manager::get_active_count( ) const
{
    ULONG count = 0;
    {
        scoped_spin_lock guard( const_cast< KSPIN_LOCK* >( &lock_ ) );
        for ( ULONG i = 0; i < max_sandboxes_; ++i ) 
        if ( entries_[ i ].active ) ++count;
    }

    return count;
}

LONG hv_sandbox_manager::find_entry_by_id( _In_ ULONG id ) const
{
    for ( ULONG i = 0; i < max_sandboxes_; ++i )
    {
        if ( entries_[ i ].active && entries_[ i ].id == id )
        return static_cast< LONG >( i );
    }

    return -1;
}

LONG hv_sandbox_manager::find_free_slot( ) const
{
    for ( ULONG i = 0; i < max_sandboxes_; ++i )
    {
        if ( !entries_[ i ].active )
        return static_cast< LONG >( i );
    }

    return -1;
}