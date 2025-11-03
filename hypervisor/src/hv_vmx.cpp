#include "../stdafx.h"

static const ULONG vmx_tag = 'xmvH';

inline bool hv_vmx::cpuid_supports_vmx( ) const
{
    int regs[ 4 ] = { 0 };
    __cpuid( regs, 1 ); // leaf 1
    const int vmx_bit = ( 1 << 5 ); // ECX bit 5
    return ( regs[ 2 ] & vmx_bit ) != 0;
}

inline unsigned __int64 hv_vmx::read_msr( unsigned long msr ) const
{
    return __readmsr( msr );
}

NTSTATUS hv_vmx::initialize( )
{
    hv_logger::log( hv_logger::level::info, "hv_vmx::initialize: beginning capability checks" );

    vmx_supported_ = cpuid_supports_vmx( );
    hv_logger::log( vmx_supported_ ? hv_logger::level::info : hv_logger::level::warning, "hv_vmx::initialize: cpuid reports vmx %s", vmx_supported_ ? "supported" : "NOT supported" );

    // Reading IA32_FEATURE_CONTROL (MSR 0x3A) which logs the raw value
    __try
    {
        ia32_feature_control_ = read_msr( 0x3A );
        hv_logger::log( hv_logger::level::info, "hv_vmx::initialize: IA32_FEATURE_CONTROL MSR (0x3A) = 0x%llx", ia32_feature_control_ );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        hv_logger::log( hv_logger::level::warning, "hv_vmx::initialize: reading IA32_FEATURE_CONTROL caused exception" );
        ia32_feature_control_ = 0;
    }

    // Reading IA32_VMX_BASIC (MSR 0x480)
    __try
    {
        ia32_vmx_basic_ = read_msr( 0x480 );
        hv_logger::log( hv_logger::level::info, "hv_vmx::initialize: IA32_VMX_BASIC MSR (0x480) = 0x%llx", ia32_vmx_basic_ );

        const unsigned long revision_id = ( unsigned long )( ia32_vmx_basic_ & 0xFFFFFFFFULL );
        hv_logger::log( hv_logger::level::info, "hv_vmx::initialize: IA32_VMX_BASIC revision id = 0x%x", revision_id );

        const unsigned long region_field = ( unsigned long )( ( ia32_vmx_basic_ >> 32 ) & 0xFFFULL );
        if ( region_field != 0 )
        {
            ULONG candidate = region_field;
            if ( candidate < PAGE_SIZE ) suggested_region_size_ = PAGE_SIZE;
            else suggested_region_size_ = candidate;
        }

        else
        {
            suggested_region_size_ = PAGE_SIZE;
        }

        hv_logger::log( hv_logger::level::info, "hv_vmx::initialize: suggested region size = %u bytes", suggested_region_size_ );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        hv_logger::log( hv_logger::level::warning, "hv_vmx::initialize: reading IA32_VMX_BASIC caused exception" );
        ia32_vmx_basic_ = 0;
        suggested_region_size_ = PAGE_SIZE;
    }

    processor_count_ = KeQueryActiveProcessorCountEx( ALL_PROCESSOR_GROUPS );
    if ( processor_count_ == 0 )
    {
        hv_logger::log( hv_logger::level::error, "hv_vmx::initialize: KeQueryActiveProcessorCountEx returned 0" );
        return STATUS_UNSUCCESSFUL;
    }

    SIZE_T alloc_size = sizeof( vmx_state ) * ( SIZE_T )processor_count_;
    per_cpu_state_ = reinterpret_cast< vmx_state* >( ExAllocatePoolWithTag( NonPagedPoolNx, alloc_size, vmx_tag ) );
    if ( !per_cpu_state_ )
    {
        hv_logger::log( hv_logger::level::error, "hv_vmx::initialize: failed to allocate per_cpu_state (%llu bytes)",
            ( unsigned long long )alloc_size );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( per_cpu_state_, alloc_size );

    // populate flag per logical processorr
    for ( ULONG i = 0; i < processor_count_; ++i )
    {
        per_cpu_state_[ i ].supported = vmx_supported_;
        per_cpu_state_[ i ].enabled = false;
    }

    hv_logger::log( hv_logger::level::info, "hv_vmx::initialize: completed: cpu_count=%u, vmx_supported=%u, suggested_region=%u", processor_count_, vmx_supported_ ? 1 : 0, suggested_region_size_ );
    return STATUS_SUCCESS;
}

NTSTATUS hv_vmx::allocate_vmxon_region( )
{
    if ( !per_cpu_state_ || processor_count_ == 0 )
    {
        hv_logger::log( hv_logger::level::error, "hv_vmx::allocate_vmxon_region: per_cpu_state not initialized" );
        return STATUS_INVALID_DEVICE_STATE;
    }

    hv_logger::log( hv_logger::level::info, "hv_vmx::allocate_vmxon_region: allocating regions per cpu" );

    // Now we allocate aligned nonpaged memory for each logical processor (so suggested_region_size_ + PAGE_SIZE) and we align them to PAGE_SIZE boundary
    for ( ULONG i = 0; i < processor_count_; ++i )
    {
        SIZE_T raw_alloc = ( SIZE_T )suggested_region_size_ + PAGE_SIZE;
        void* raw = ExAllocatePoolWithTag( NonPagedPoolNx, raw_alloc, vmx_tag );
        if ( !raw )
        {
            hv_logger::log( hv_logger::level::error, "hv_vmx::allocate_vmxon_region: allocation failed on cpu %u", i );
            for ( ULONG j = 0; j < i; ++j )
            {
                if ( per_cpu_state_[ j ].vmxon_virtual )
                {
                    ExFreePoolWithTag( per_cpu_state_[ j ].vmxon_virtual, vmx_tag );
                    per_cpu_state_[ j ].vmxon_virtual = nullptr;
                }
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // now we align to page boundary
        ULONG_PTR raw_addr = reinterpret_cast< ULONG_PTR >( raw );
        ULONG_PTR aligned_addr = ( raw_addr + ( PAGE_SIZE - 1 ) ) & ~( ULONG_PTR )( PAGE_SIZE - 1 );
        void* aligned_ptr = reinterpret_cast< void* >( aligned_addr );

        // always zero the regions to be safe when doing the kind of following operations
        RtlZeroMemory( aligned_ptr, suggested_region_size_ );
        per_cpu_state_[ i ].vmxon_virtual = aligned_ptr;

        // todo: translate virtual to physical
        per_cpu_state_[ i ].vmxon_physical = MmGetPhysicalAddress( aligned_ptr );

        hv_logger::log( hv_logger::level::info, "hv_vmx::allocate_vmxon_region: cpu=%u vmxon_virtual=%p vmxon_physical=0x%llx", i, per_cpu_state_[ i ].vmxon_virtual, per_cpu_state_[ i ].vmxon_physical.QuadPart );
    }

    return STATUS_SUCCESS;
}

void hv_vmx::free_vmxon_region( )
{
    if ( !per_cpu_state_ ) return;
    for ( ULONG i = 0; i < processor_count_; ++i )
    {
        if ( per_cpu_state_[ i ].vmxon_virtual )
        {
            ExFreePoolWithTag( per_cpu_state_[ i ].vmxon_virtual, vmx_tag );
            per_cpu_state_[ i ].vmxon_virtual = nullptr;
            per_cpu_state_[ i ].vmxon_physical.QuadPart = 0;
        }

        if ( per_cpu_state_[ i ].vmcs_virtual )
        {
            ExFreePoolWithTag( per_cpu_state_[ i ].vmcs_virtual, vmx_tag );
            per_cpu_state_[ i ].vmcs_virtual = nullptr;
            per_cpu_state_[ i ].vmcs_physical.QuadPart = 0;
        }
    }
}

NTSTATUS hv_vmx::shutdown( )
{
    hv_logger::log( hv_logger::level::info, "hv_vmx::shutdown: freeing resources" );
    free_vmxon_region( );

    if ( per_cpu_state_ )
    {
        ExFreePoolWithTag( per_cpu_state_, vmx_tag );
        per_cpu_state_ = nullptr;
        processor_count_ = 0;
    }

    hv_logger::log( hv_logger::level::info, "hv_vmx::shutdown: complete" );
    return STATUS_SUCCESS;
}