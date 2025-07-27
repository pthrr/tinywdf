#pragma once

#include <cstddef>
#include <cstdint>

namespace cmn {
namespace types {

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using isize = std::ptrdiff_t;
using usize = std::size_t;

using str = const char*;

constexpr auto operator""_i8( unsigned long long int value ) -> i8
{
    return static_cast< i8 >( value );
}

constexpr auto operator""_i16( unsigned long long int value ) -> i16
{
    return static_cast< i16 >( value );
}

constexpr auto operator""_i32( unsigned long long int value ) -> i32
{
    return static_cast< i32 >( value );
}

constexpr auto operator""_i64( unsigned long long int value ) -> i64
{
    return static_cast< i64 >( value );
}

constexpr auto operator""_u8( unsigned long long int value ) -> u8
{
    return static_cast< u8 >( value );
}

constexpr auto operator""_u16( unsigned long long int value ) -> u16
{
    return static_cast< u16 >( value );
}

constexpr auto operator""_u32( unsigned long long int value ) -> u32
{
    return static_cast< u32 >( value );
}

constexpr auto operator""_u64( unsigned long long int value ) -> u64
{
    return static_cast< u64 >( value );
}

constexpr auto operator""_isize( unsigned long long int value ) -> isize
{
    return static_cast< isize >( value );
}

constexpr auto operator""_usize( unsigned long long int value ) -> usize
{
    return static_cast< usize >( value );
}

} // namespace types
} // namespace cmn
