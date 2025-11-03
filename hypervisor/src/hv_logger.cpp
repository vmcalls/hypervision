#include "../stdafx.h"

void hv_logger::initialize( )
{
    DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "hv_logger: initialized\n" );
}

void hv_logger::shutdown( )
{
    DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "hv_logger: shutdown\n" );
}

const char* hv_logger::level_to_str( level lv )
{
    switch ( lv )
    {
        case level::info:    return "INFO";
        case level::warning: return "WARN";
        case level::error:   return "ERR";
        default:             return "UNK";
    }
}

void hv_logger::log( level lv, const char* fmt, ... )
{
    va_list args;
    va_start( args, fmt );

    char buffer[ 512 ];
    RtlZeroMemory( buffer, sizeof( buffer ) );

    RtlStringCbPrintfA( buffer, sizeof( buffer ), "[hv][%s] ", level_to_str( lv ) );
    size_t prefix_len = strlen( buffer );

    RtlStringCbVPrintfA( buffer + prefix_len, sizeof( buffer ) - prefix_len, fmt, args );
    va_end( args );
    DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%s\n", buffer );
}