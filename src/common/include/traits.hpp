#include <concepts>
#include <functional>
#include <type_traits>
#include <variant>

namespace common {

template< typename T >
struct is_variant : std::false_type
{
};

template< typename... Ts >
struct is_variant< std::variant< Ts... > > : std::true_type
{
};

template< typename T >
concept Variant = is_variant< T >::value;

} // namespace common
