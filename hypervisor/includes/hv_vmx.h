#pragma once

struct vmx_state
{
    bool supported{ false };
    bool enabled{ false };

    void* vmxon_virtual{ nullptr };
    PHYSICAL_ADDRESS vmxon_physical{ 0 };

    void* vmcs_virtual{ nullptr };
    PHYSICAL_ADDRESS vmcs_physical{ 0 };
};

class hv_vmx
{
public:
    hv_vmx( ) = default;
    ~hv_vmx( ) = default;

    NTSTATUS initialize( );
    NTSTATUS shutdown( );
    NTSTATUS allocate_vmxon_region( );

    void free_vmxon_region( );
    bool is_vmx_supported( ) const { return vmx_supported_; }

private:
    bool cpuid_supports_vmx( ) const;
    unsigned __int64 read_msr( unsigned long msr ) const;

private:
    vmx_state* per_cpu_state_{ nullptr };
    ULONG processor_count_{ 0 };

    unsigned __int64 ia32_vmx_basic_{ 0 };
    unsigned __int64 ia32_feature_control_{ 0 };
    ULONG suggested_region_size_{ PAGE_SIZE };

    bool vmx_supported_{ false };
};