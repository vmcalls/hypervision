#pragma once

class hv_ept
{
public:
    hv_ept( ) = default;
    ~hv_ept( ) = default;

    NTSTATUS build_identity_map( );
    void destroy( );

    ULONG64 get_page_count( ) const { return page_count_; }
    ULONG64 get_alloc_bytes( ) const { return alloc_bytes_; }

private:
    void* ept_pml4_{ nullptr };
    ULONG64 page_count_{ 0 };
    ULONG64 alloc_bytes_{ 0 };
};