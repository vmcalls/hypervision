#pragma once

class hv_logger
{
public:
    enum class level : unsigned char
    {
        info,
        warning,
        error,
    };

    static void initialize( );
    static void shutdown( );
    static void log( level lv, const char* fmt, ... );

private:
    static const char* level_to_str( level lv );
};