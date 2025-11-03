#pragma once

class hv_sandbox_manager
{
public:
    hv_sandbox_manager( ) = default;
    ~hv_sandbox_manager( ) = default;

    NTSTATUS initialize( );
    void     shutdown( );

    NTSTATUS create_sandbox( _In_ ULONG id );
    NTSTATUS destroy_sandbox( _In_ ULONG id );

    NTSTATUS list_sandboxes( _Out_writes_opt_( max_ids ) ULONG* out_ids, _In_ ULONG max_ids, _Out_opt_ ULONG* out_count ) const;

    _Must_inspect_result_ ULONG get_active_count( ) const;

private:
    struct sandbox_entry
    {
        BOOLEAN        active{ FALSE };
        ULONG          id{ 0 };
        hv_ept         ept;
        LARGE_INTEGER  created{};
    };

    _IRQL_requires_max_( DISPATCH_LEVEL ) 
    _Must_inspect_result_ LONG find_entry_by_id( _In_ ULONG id ) const;

    _IRQL_requires_max_( DISPATCH_LEVEL )
    _Must_inspect_result_ LONG find_free_slot( ) const;

private:
    static constexpr ULONG max_sandboxes_ = 16;

    mutable KSPIN_LOCK lock_{};
    sandbox_entry      entries_[ max_sandboxes_ ] = {};
};