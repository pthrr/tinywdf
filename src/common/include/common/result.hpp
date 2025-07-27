#pragma once

#include <array>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <utility>

#include "types.hpp"

namespace cmn {
namespace result {

using namespace cmn::types;

struct [[nodiscard]] Error
{
    enum struct ErrorType : u8
    {
        ARITHMETIC_ERROR,
        FLOATING_POINT_ERROR,
        OVERFLOW_ERROR,
        ZERO_DIVISION_ERROR,
        ASSERTION_ERROR,
        ATTRIBUTE_ERROR,
        INDEX_ERROR,
        KEY_ERROR,
        OS_ERROR,
        TIMEOUT_ERROR,
        RUNTIME_ERROR,
        NOT_IMPLEMENTED_ERROR,
        SYNTAX_ERROR,
        SYSTEM_ERROR,
        TYPE_ERROR,
        VALUE_ERROR,
        GENERIC_ERROR,
    };

    str message;
    ErrorType type;

    constexpr Error() noexcept
        : message( "" )
        , type( ErrorType::GENERIC_ERROR )
    {
    }

    constexpr explicit Error( str msg, ErrorType typ ) noexcept
        : message( msg )
        , type( typ )
    {
    }

    constexpr explicit Error( str msg ) noexcept
        : message( msg )
        , type( ErrorType::GENERIC_ERROR )
    {
    }

    // clang-format off
    static constexpr auto toStr( ErrorType typ ) -> str {
        switch ( typ ) {
            case Error::ErrorType::ARITHMETIC_ERROR: return "ArithmeticError";
            case Error::ErrorType::FLOATING_POINT_ERROR: return "FloatingPointError";
            case Error::ErrorType::OVERFLOW_ERROR: return "OverflowError";
            case Error::ErrorType::ZERO_DIVISION_ERROR: return "ZeroDivisionError";
            case Error::ErrorType::ASSERTION_ERROR: return "AssertionError";
            case Error::ErrorType::ATTRIBUTE_ERROR: return "AttributeError";
            case Error::ErrorType::INDEX_ERROR: return "IndexError";
            case Error::ErrorType::KEY_ERROR: return "KeyError";
            case Error::ErrorType::OS_ERROR: return "OSError";
            case Error::ErrorType::TIMEOUT_ERROR: return "TimeoutError";
            case Error::ErrorType::RUNTIME_ERROR: return "RuntimeError";
            case Error::ErrorType::NOT_IMPLEMENTED_ERROR: return "NotImplementedError";
            case Error::ErrorType::SYNTAX_ERROR: return "SyntaxError";
            case Error::ErrorType::SYSTEM_ERROR: return "SystemError";
            case Error::ErrorType::TYPE_ERROR: return "TypeError";
            case Error::ErrorType::VALUE_ERROR: return "ValueError";
            case Error::ErrorType::GENERIC_ERROR: return "GenericError";
            default: std::unreachable();
        }
    }
    // clang-format on

    [[nodiscard]] auto toStr() const -> str
    {
        thread_local std::array< char, 256 > s_buffer{};
        s_buffer.fill( '\0' ); // Clear entire buffer
        auto written = ::snprintf( s_buffer.data(), s_buffer.size(), "%s: %s", toStr( type ), message );

        if( written <= 0 ) {
            return {};
        }

        return s_buffer.data();
    }
};

template< typename T >
using Result = std::expected< T, Error >;

using Status = Result< void >;

template< typename T >
inline constexpr auto ok( T&& value ) -> Result< std::decay_t< T > >
{
    return Result< std::decay_t< T > >( std::forward< T >( value ) );
}

template< typename T >
inline constexpr auto ok() -> Result< T >
{
    return Result< T >{};
}

inline constexpr auto ok() -> Status
{
    return {};
}

inline constexpr auto err( str msg = "", Error::ErrorType typ = Error::ErrorType::GENERIC_ERROR )
    -> std::unexpected< Error >
{
    return std::unexpected< Error >( Error( msg, typ ) );
}

template< typename T >
inline auto unwrap( Result< T > const& res ) -> T
{
    if( not res ) [[unlikely]] {
        std::abort();
    }

    return res.value();
}

template< typename T >
inline auto unwrap( Result< T >&& res ) -> T
{
    if( not res ) [[unlikely]] {
        std::abort();
    }

    return std::move( res.value() );
}

inline auto verify( Status const& sta ) -> void
{
    if( not sta ) [[unlikely]] {
        std::abort();
    }
}

} // namespace result
} // namespace cmn
