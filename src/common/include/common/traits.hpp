#pragma once

#include <concepts>
#include <type_traits>
#include <variant>

#include "types.hpp"

namespace cmn {
namespace traits {

using namespace cmn::types;

template< typename T >
struct is_variant : std::false_type
{
};

template< typename... Ts >
struct is_variant< std::variant< Ts... > > : std::true_type
{
};

template< typename T >
inline constexpr bool is_variant_v = is_variant< T >::value;

template< typename T >
concept Variant = is_variant< T >::value;

static_assert( Variant< std::variant< float, int > >, "std::variant<float, int> must satisfy Variant" );

template< typename T >
concept Decimal = std::is_floating_point_v< T >;

static_assert( Decimal< float >, "float must satisfy Decimal" );
static_assert( Decimal< double >, "double must satisfy Decimal" );

template< typename T >
concept Integral = std::is_integral_v< T >;

static_assert( Integral< int >, "int must satisfy Integral" );

template< typename T >
concept Numerical = Decimal< T > or Integral< T >;

static_assert( Numerical< float >, "float must satisfy Decimal" );
static_assert( Numerical< double >, "double must satisfy Decimal" );
static_assert( Numerical< int >, "int must satisfy Integral" );

template< typename T >
concept UnsignedIntegral = std::is_integral_v< T > and std::is_unsigned_v< T >;

static_assert( UnsignedIntegral< unsigned int >, "unsigned int must satisfy UnsignedIntegral" );

template< typename T >
concept BooleanTestable = std::convertible_to< T, bool >;

static_assert( BooleanTestable< bool >, "bool must be BooleanTestable" );

template< typename T >
concept BitmaskEnum =
    std::is_enum_v< T > and requires { T::ALL; } and std::is_unsigned_v< std::underlying_type_t< T > > and
    not std::is_convertible_v< T, std::underlying_type_t< T > >;

enum class TestBitmask : unsigned
{
    NONE = 0,
    FLAG1 = 1,
    FLAG2 = 2,
    ALL = 3
};
static_assert( BitmaskEnum< TestBitmask >, "TestBitmask must satisfy BitmaskEnum" );

template< typename T >
concept ScopedEnum = std::is_enum_v< T > and requires { T::LAST; } and
    not std::is_convertible_v< T, std::underlying_type_t< T > >;

enum class TestScoped
{
    FIRST,
    SECOND,
    THIRD,
    LAST = THIRD
};
static_assert( ScopedEnum< TestScoped >, "TestScoped must satisfy ScopedEnum" );

template< ScopedEnum T >
struct enum_count
{
    static constexpr auto value = static_cast< usize >( T::LAST ) + 1;
};

template< ScopedEnum T >
constexpr usize enum_count_v = enum_count< T >::value;

static_assert( enum_count_v< TestScoped > == 3, "enum_count_v<TestScoped> must return 3" );

} // namespace traits
} // namespace cmn
