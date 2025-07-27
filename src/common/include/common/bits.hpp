#pragma once

#include <bitset>
#include <type_traits>

#include "result.hpp"
#include "traits.hpp"
#include "types.hpp"

namespace cmn {
namespace bits {

using namespace cmn::types;
using namespace cmn::result;
using cmn::traits::BitmaskEnum;

template< BitmaskEnum Enum >
constexpr auto operator|( Enum left, Enum right ) -> Enum
{
    return static_cast< Enum >( static_cast< std::underlying_type_t< Enum > >( left ) |
        static_cast< std::underlying_type_t< Enum > >( right ) );
}

template< BitmaskEnum Enum >
constexpr auto operator&( Enum left, Enum right ) -> Enum
{
    return static_cast< Enum >( static_cast< std::underlying_type_t< Enum > >( left ) &
        static_cast< std::underlying_type_t< Enum > >( right ) );
}

template< BitmaskEnum Enum >
constexpr auto operator~( Enum num ) -> Enum
{
    return static_cast< Enum >( ~static_cast< std::underlying_type_t< Enum > >( num ) );
}

template< BitmaskEnum Enum >
struct FlagSet
{
    using Underlying = std::underlying_type_t< Enum >;

private:
    static constexpr auto bitPosition( Underlying value ) -> usize
    {
        if( value == 0 ) {
            return NUM_BITS;
        }

        usize pos = 0;

        for( ; ( value & 1U ) == 0U and value; ) {
            ++pos;
            value >>= 1U;
        }

        return pos;
    }

    static constexpr auto NUM_BITS = [] {
        auto all_bits = static_cast< Underlying >( Enum::ALL );
        usize count = 0;

        for( ; all_bits; ) {
            ++count;
            all_bits >>= 1;
        }

        return count;
    }();

    static constexpr auto VALID_MASK = static_cast< Underlying >( Enum::ALL );
    std::bitset< NUM_BITS > m_bits{};

public:
    constexpr FlagSet() = default;

    constexpr explicit FlagSet( Enum num )
        : m_bits( static_cast< Underlying >( num ) & VALID_MASK )
    {
    }

    // Factory methods
    static constexpr auto fromUnderlying( Underlying raw ) -> FlagSet
    {
        return FlagSet( static_cast< Enum >( raw & VALID_MASK ) );
    }

    static constexpr auto fromEnum( Enum num ) -> Result< FlagSet >
    {
        if( not isValid( num ) ) {
            return err( "Invalid enum value for FlagSet", Error::ErrorType::VALUE_ERROR );
        }

        return ok( FlagSet( num ) );
    }

    // Single flag operations
    constexpr auto has( Enum num ) const -> Result< bool >
    {
        if( not isValid( num ) ) {
            return err( "Invalid enum value", Error::ErrorType::VALUE_ERROR );
        }

        return ok( m_bits.test( bitPosition( static_cast< Underlying >( num ) ) ) );
    }

    constexpr auto set( Enum num ) -> Status
    {
        if( not isValid( num ) ) {
            return err( "Invalid enum value", Error::ErrorType::VALUE_ERROR );
        }

        m_bits.set( bitPosition( static_cast< Underlying >( num ) ) );
        return ok();
    }

    constexpr auto clear( Enum num ) -> Status
    {
        if( not isValid( num ) ) {
            return err( "Invalid enum value", Error::ErrorType::VALUE_ERROR );
        }

        m_bits.reset( bitPosition( static_cast< Underlying >( num ) ) );
        return ok();
    }

    constexpr auto toggle( Enum num ) -> Status
    {
        if( not isValid( num ) ) {
            return err( "Invalid enum value", Error::ErrorType::VALUE_ERROR );
        }

        m_bits.flip( bitPosition( static_cast< Underlying >( num ) ) );
        return ok();
    }

    // Bulk operations
    constexpr auto setAll() -> Status
    {
        m_bits = std::bitset< NUM_BITS >( VALID_MASK );
        return ok();
    }

    constexpr auto clearAll() -> Status
    {
        m_bits.reset();
        return ok();
    }

    constexpr auto toggleAll() -> Status
    {
        m_bits ^= std::bitset< NUM_BITS >( VALID_MASK );
        return ok();
    }

    // Conversion methods
    constexpr auto toUnderlying() const -> Underlying
    {
        return static_cast< Underlying >( m_bits.to_ullong() );
    }

    constexpr auto toEnum() const -> Enum
    {
        return static_cast< Enum >( toUnderlying() );
    }

    // Query methods
    auto hasAny() const -> bool
    {
        return m_bits.any();
    }

    auto hasNone() const -> bool
    {
        return m_bits.none();
    }

    auto count() const -> usize
    {
        return m_bits.count();
    }

    // Validation methods
    static constexpr auto isValid( Enum value ) -> bool
    {
        return ( static_cast< Underlying >( value ) & ~VALID_MASK ) == 0;
    }

    constexpr auto isValid() const -> bool
    {
        return ( toUnderlying() & ~VALID_MASK ) == 0;
    }

    // Iteration
    template< typename Func >
    auto forEach( Func&& func ) const -> void
    {
        for( size_t i = 0; i < NUM_BITS; ++i ) {
            if( m_bits.test( i ) ) {
                func( static_cast< Enum >( 1 << i ) );
            }
        }
    }

    // Operators
    explicit operator bool() const
    {
        return hasAny();
    }

    explicit constexpr operator Underlying() const
    {
        return toUnderlying();
    }

    explicit constexpr operator Enum() const
    {
        return toEnum();
    }

    constexpr auto operator|( const FlagSet& other ) const -> FlagSet
    {
        return FlagSet( static_cast< Enum >( toUnderlying() | other.toUnderlying() ) );
    }

    constexpr auto operator&( const FlagSet& other ) const -> FlagSet
    {
        return FlagSet( static_cast< Enum >( toUnderlying() & other.toUnderlying() ) );
    }

    constexpr auto operator~() const -> FlagSet
    {
        return FlagSet( static_cast< Enum >( ~toUnderlying() & VALID_MASK ) );
    }

    constexpr auto operator|=( const FlagSet& other ) -> FlagSet&
    {
        m_bits |= other.m_bits;
        return *this;
    }

    constexpr auto operator&=( const FlagSet& other ) -> FlagSet&
    {
        m_bits &= other.m_bits;
        return *this;
    }

    constexpr auto operator^( const FlagSet& other ) const -> FlagSet
    {
        return FlagSet( static_cast< Enum >( toUnderlying() ^ other.toUnderlying() ) );
    }

    constexpr auto operator^=( const FlagSet& other ) -> FlagSet&
    {
        m_bits ^= other.m_bits;
        return *this;
    }

    struct Iterator
    {
        Iterator( std::bitset< NUM_BITS > const& b, usize i )
            : m_bits( b )
            , m_index( i )
        {
            advance();
        }

        auto operator++() -> Iterator&
        {
            ++m_index;
            advance();
            return *this;
        }

        auto operator*() const -> Enum
        {
            return static_cast< Enum >( 1 << m_index );
        }

        auto operator!=( Iterator const& other ) const -> bool
        {
            return m_index != other.m_index;
        }

    private:
        usize m_index;
        std::bitset< NUM_BITS > const& m_bits;

        auto advance() -> void
        {
            for( ; m_index < NUM_BITS and not m_bits.test( m_index ); ) {
                ++m_index;
            }
        }
    };

    auto begin() const -> Iterator
    {
        return Iterator( m_bits, 0 );
    }

    auto end() const -> Iterator
    {
        return Iterator( m_bits, NUM_BITS );
    }
};

} // namespace bits
} // namespace cmn
