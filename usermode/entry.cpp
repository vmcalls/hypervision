#include "includes/driver_interface.h"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

static HANDLE open_device( )
{
    HANDLE h = CreateFileA( HV_DEVICE_LINK, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( h == INVALID_HANDLE_VALUE ) return nullptr;
    return h;
}

static void print_usage( const char* prog )
{
    std::cout << "usage: " << prog << " <command> [args]\n\n";
    std::cout << "commands:\n";
    std::cout << "  query                 - query driver VMX/EPT capabilities\n";
    std::cout << "  build-ept             - ask driver to build demo EPT\n";
    std::cout << "  sandbox-create <id>   - create sandbox with id\n";
    std::cout << "  sandbox-destroy <id>  - destroy sandbox with id\n";
    std::cout << "  sandbox-list          - list active sandbox ids\n";
    std::cout << "  nop                   - ping driver (fast test)\n";
    std::cout << std::endl;
}

static bool ioctl_nop( HANDLE h )
{
    DWORD returned = 0;
    BOOL ok = DeviceIoControl( h, IOCTL_HV_NOP, nullptr, 0, nullptr, 0, &returned, nullptr );
    if ( !ok ) std::cerr << "ioctl_nop failed: " << GetLastError( ) << "\n";
    return ok != FALSE;
}

static bool ioctl_query_caps( HANDLE h )
{
    hv_vmx_caps caps = {};
    DWORD out_sz = sizeof( caps );
    DWORD returned = 0;
    BOOL ok = DeviceIoControl( h, IOCTL_HV_QUERY_CAPS, nullptr, 0, &caps, out_sz, &returned, nullptr );
    if ( !ok )
    {
        std::cerr << "ioctl_query_caps failed: " << GetLastError( ) << "\n";
        return false;
    }

    std::cout << "VMX supported: " << ( caps.vmx_supported ? "yes" : "no" ) << "\n";
    std::cout << "cpu count: " << caps.cpu_count << "\n";
    std::cout << "suggested region size: " << caps.suggested_region_size << " bytes\n";
    std::cout << "ept pages (demo): " << caps.ept_page_count << "\n";
    std::cout << "active sandboxes: " << caps.sandbox_count << "\n";

    return true;
}

static bool ioctl_build_ept( HANDLE h )
{
    DWORD returned = 0;
    BOOL ok = DeviceIoControl( h, IOCTL_HV_BUILD_EPT, nullptr, 0, nullptr, 0, &returned, nullptr );
    if ( !ok )
    {
        std::cerr << "ioctl_build_ept failed: " << GetLastError( ) << "\n";
        return false;
    }
    std::cout << "build-ept request succeeded\n";
    return true;
}

static bool ioctl_sandbox_create( HANDLE h, ULONG id )
{
    hv_sandbox_request req = {};
    req.id = id;
    DWORD returned = 0;
    BOOL ok = DeviceIoControl( h, IOCTL_HV_SANDBOX_CREATE, &req, sizeof( req ), nullptr, 0, &returned, nullptr );
    if ( !ok )
    {
        std::cerr << "ioctl_sandbox_create failed: " << GetLastError( ) << "\n";
        return false;
    }
    std::cout << "sandbox-create succeeded (id=" << id << ")\n";
    return true;
}

static bool ioctl_sandbox_destroy( HANDLE h, ULONG id )
{
    hv_sandbox_request req = {};
    req.id = id;
    DWORD returned = 0;
    BOOL ok = DeviceIoControl( h, IOCTL_HV_SANDBOX_DESTROY, &req, sizeof( req ), nullptr, 0, &returned, nullptr );
    if ( !ok )
    {
        std::cerr << "ioctl_sandbox_destroy failed: " << GetLastError( ) << "\n";
        return false;
    }
    std::cout << "sandbox-destroy succeeded (id=" << id << ")\n";
    return true;
}

static bool ioctl_sandbox_list( HANDLE h )
{
    const ULONG max_ids = 64;
    std::vector<ULONG> ids( max_ids );
    DWORD out_sz = ( DWORD )( max_ids * sizeof( ULONG ) );
    DWORD returned = 0;

    BOOL ok = DeviceIoControl( h, IOCTL_HV_SANDBOX_LIST, nullptr, 0, ids.data( ), out_sz, &returned, nullptr );
    if ( !ok )
    {
        std::cerr << "ioctl_sandbox_list failed: " << GetLastError( ) << "\n";
        return false;
    }

    if ( returned == 0 )
    {
        std::cout << "no sandboxes active\n";
        return true;
    }

    ULONG written_count = returned / sizeof( ULONG );
    std::cout << "active sandboxes (" << written_count << "): ";
    for ( ULONG i = 0; i < written_count; ++i )
    {
        std::cout << ids[ i ];
        if ( i + 1 < written_count ) std::cout << ", ";
    }
    std::cout << "\n";
    return true;
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        print_usage( argv[ 0 ] );
        return 1;
    }

    std::string cmd = argv[ 1 ];

    HANDLE h = open_device( );
    if ( !h )
    {
        std::cerr << "failed to open " << HV_DEVICE_LINK << " (is driver loaded?)\n";
        return 1;
    }

    bool ok = false;
    if ( cmd == "nop" )
    {
        ok = ioctl_nop( h );
    }
    else if ( cmd == "query" )
    {
        ok = ioctl_query_caps( h );
    }
    else if ( cmd == "build-ept" )
    {
        ok = ioctl_build_ept( h );
    }
    else if ( cmd == "sandbox-create" )
    {
        if ( argc < 3 ) { std::cerr << "sandbox-create requires id\n"; print_usage( argv[ 0 ] ); }
        else
        {
            ULONG id = ( ULONG )std::stoul( argv[ 2 ] );
            ok = ioctl_sandbox_create( h, id );
        }
    }
    else if ( cmd == "sandbox-destroy" )
    {
        if ( argc < 3 ) { std::cerr << "sandbox-destroy requires id\n"; print_usage( argv[ 0 ] ); }
        else
        {
            ULONG id = ( ULONG )std::stoul( argv[ 2 ] );
            ok = ioctl_sandbox_destroy( h, id );
        }
    }
    else if ( cmd == "sandbox-list" )
    {
        ok = ioctl_sandbox_list( h );
    }
    else
    {
        std::cerr << "unknown command: " << cmd << "\n";
        print_usage( argv[ 0 ] );
    }

    CloseHandle( h );
    return ok ? 0 : 2;
}