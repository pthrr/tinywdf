#include <catch2/catch_test_macros.hpp>

#include <common/bits.hpp>
#include <common/result.hpp>
#include <common/types.hpp>

#include <algorithm>
#include <vector>

using namespace cmn::types;
using namespace cmn::bits;
using namespace cmn::result;

enum class Permissions : u32
{
    NONE = 0,
    READ = 1,
    WRITE = 2,
    EXECUTE = 4,
    ALL = READ | WRITE | EXECUTE
};

// Additional test enum for edge cases
enum class FileFlags : u8
{
    NONE = 0,
    HIDDEN = 1,
    READONLY = 2,
    SYSTEM = 4,
    ARCHIVE = 8,
    ALL = HIDDEN | READONLY | SYSTEM | ARCHIVE
};

// Test enum with non-contiguous values
enum class NetworkFlags : u16
{
    NONE = 0,
    TCP = 1,
    UDP = 2,
    IPV6 = 8, // Gap in bit positions
    ENCRYPTED = 16,
    ALL = TCP | UDP | IPV6 | ENCRYPTED
};

TEST_CASE( "FlagSet bitPosition function correctness" )
{
    SECTION( "Single bit positions" )
    {
        // Test that bitPosition correctly identifies single bit positions
        FlagSet< Permissions > flags;

        // These operations should work correctly for single-bit flags
        REQUIRE( flags.set( Permissions::READ ).has_value() );
        auto hasRead = flags.has( Permissions::READ );
        REQUIRE( hasRead.has_value() );
        REQUIRE( hasRead.value() );

        REQUIRE( flags.set( Permissions::WRITE ).has_value() );
        auto hasWrite = flags.has( Permissions::WRITE );
        REQUIRE( hasWrite.has_value() );
        REQUIRE( hasWrite.value() );

        REQUIRE( flags.set( Permissions::EXECUTE ).has_value() );
        auto hasExecute = flags.has( Permissions::EXECUTE );
        REQUIRE( hasExecute.has_value() );
        REQUIRE( hasExecute.value() );
    }

    SECTION( "Combined flag handling" )
    {
        // Test with combined flags - this might reveal issues in bitPosition
        FlagSet< Permissions > flags( Permissions::READ | Permissions::WRITE );

        // Individual flags should still be detectable
        auto hasRead = flags.has( Permissions::READ );
        auto hasWrite = flags.has( Permissions::WRITE );
        auto hasExecute = flags.has( Permissions::EXECUTE );

        REQUIRE( hasRead.has_value() );
        REQUIRE( hasRead.value() );
        REQUIRE( hasWrite.has_value() );
        REQUIRE( hasWrite.value() );
        REQUIRE( hasExecute.has_value() );
        REQUIRE_FALSE( hasExecute.value() );
    }

    SECTION( "ALL flag handling" )
    {
        // Test that ALL flag is handled correctly
        FlagSet< Permissions > flags( Permissions::ALL );

        auto hasRead = flags.has( Permissions::READ );
        auto hasWrite = flags.has( Permissions::WRITE );
        auto hasExecute = flags.has( Permissions::EXECUTE );

        REQUIRE( hasRead.has_value() );
        REQUIRE( hasRead.value() );
        REQUIRE( hasWrite.has_value() );
        REQUIRE( hasWrite.value() );
        REQUIRE( hasExecute.has_value() );
        REQUIRE( hasExecute.value() );
    }
}

TEST_CASE( "FlagSet NUM_BITS calculation verification" )
{
    SECTION( "Permissions enum" )
    {
        FlagSet< Permissions > flags;

        // Should be able to set all individual flags
        REQUIRE( flags.set( Permissions::READ ).has_value() );
        REQUIRE( flags.set( Permissions::WRITE ).has_value() );
        REQUIRE( flags.set( Permissions::EXECUTE ).has_value() );

        // Count should match number of individual flags
        REQUIRE( flags.count() == 3 );

        // setAll should work correctly
        flags.clearAll();
        REQUIRE( flags.setAll().has_value() );
        REQUIRE( flags.count() == 3 );
    }

    SECTION( "FileFlags enum" )
    {
        FlagSet< FileFlags > flags;

        REQUIRE( flags.set( FileFlags::HIDDEN ).has_value() );
        REQUIRE( flags.set( FileFlags::READONLY ).has_value() );
        REQUIRE( flags.set( FileFlags::SYSTEM ).has_value() );
        REQUIRE( flags.set( FileFlags::ARCHIVE ).has_value() );

        REQUIRE( flags.count() == 4 );

        flags.clearAll();
        REQUIRE( flags.setAll().has_value() );
        REQUIRE( flags.count() == 4 );
    }

    SECTION( "NetworkFlags enum with gaps" )
    {
        FlagSet< NetworkFlags > flags;

        // Test that non-contiguous bit positions work
        REQUIRE( flags.set( NetworkFlags::TCP ).has_value() );
        REQUIRE( flags.set( NetworkFlags::UDP ).has_value() );
        REQUIRE( flags.set( NetworkFlags::IPV6 ).has_value() );      // Bit 3 (gap)
        REQUIRE( flags.set( NetworkFlags::ENCRYPTED ).has_value() ); // Bit 4

        auto hasTcp = flags.has( NetworkFlags::TCP );
        auto hasUdp = flags.has( NetworkFlags::UDP );
        auto hasIpv6 = flags.has( NetworkFlags::IPV6 );
        auto hasEncrypted = flags.has( NetworkFlags::ENCRYPTED );

        REQUIRE( hasTcp.has_value() );
        REQUIRE( hasTcp.value() );
        REQUIRE( hasUdp.has_value() );
        REQUIRE( hasUdp.value() );
        REQUIRE( hasIpv6.has_value() );
        REQUIRE( hasIpv6.value() );
        REQUIRE( hasEncrypted.has_value() );
        REQUIRE( hasEncrypted.value() );

        REQUIRE( flags.count() == 4 );
    }
}

TEST_CASE( "FlagSet iteration correctness" )
{
    SECTION( "Basic iteration" )
    {
        FlagSet< Permissions > flags( Permissions::READ | Permissions::EXECUTE );

        std::vector< Permissions > collected;
        for( auto flag : flags ) {
            collected.push_back( flag );
        }

        // Should contain exactly READ and EXECUTE
        REQUIRE( collected.size() == 2 );

        // Verify the flags are correct (order may vary)
        u32 combined = 0;
        for( auto flag : collected ) {
            combined |= static_cast< u32 >( flag );
        }
        REQUIRE( combined == 5u ); // READ(1) | EXECUTE(4) = 5
    }

    SECTION( "Iteration with gaps" )
    {
        FlagSet< NetworkFlags > flags( NetworkFlags::TCP | NetworkFlags::IPV6 );

        std::vector< NetworkFlags > collected;
        for( auto flag : flags ) {
            collected.push_back( flag );
        }

        REQUIRE( collected.size() == 2 );

        u16 combined = 0;
        for( auto flag : collected ) {
            combined |= static_cast< u16 >( flag );
        }
        REQUIRE( combined == 9u ); // TCP(1) | IPV6(8) = 9
    }

    SECTION( "Empty iteration" )
    {
        FlagSet< Permissions > empty;

        usize count = 0;
        for( auto flag : empty ) {
            ++count;
            (void)flag; // Suppress unused warning
        }
        REQUIRE( count == 0 );
    }

    SECTION( "Full iteration" )
    {
        FlagSet< Permissions > all( Permissions::ALL );

        usize count = 0;
        u32 combined = 0;
        for( auto flag : all ) {
            ++count;
            combined |= static_cast< u32 >( flag );
        }

        REQUIRE( count == 3 );
        REQUIRE( combined == 7u ); // READ(1) | WRITE(2) | EXECUTE(4) = 7
    }
}

TEST_CASE( "FlagSet forEach correctness" )
{
    SECTION( "forEach with all flags" )
    {
        FlagSet< NetworkFlags > flags( NetworkFlags::ALL );

        std::vector< NetworkFlags > collected;
        flags.forEach( [&]( NetworkFlags flag ) { collected.push_back( flag ); } );

        REQUIRE( collected.size() == 4 ); // TCP, UDP, IPV6, ENCRYPTED

        // Verify all expected flags are present
        u16 combined = 0;
        for( auto flag : collected ) {
            combined |= static_cast< u16 >( flag );
        }
        REQUIRE( combined == static_cast< u16 >( NetworkFlags::ALL ) );
    }

    SECTION( "forEach consistency with iteration" )
    {
        FlagSet< Permissions > flags( Permissions::READ | Permissions::WRITE );

        // Collect via forEach
        std::vector< Permissions > viaForEach;
        flags.forEach( [&]( Permissions flag ) { viaForEach.push_back( flag ); } );

        // Collect via range-based for
        std::vector< Permissions > viaIterator;
        for( auto flag : flags ) {
            viaIterator.push_back( flag );
        }

        // Should be identical
        REQUIRE( viaForEach.size() == viaIterator.size() );

        std::sort( viaForEach.begin(), viaForEach.end() );
        std::sort( viaIterator.begin(), viaIterator.end() );
        REQUIRE( viaForEach == viaIterator );
    }
}

TEST_CASE( "FlagSet masking and validation" )
{
    SECTION( "Invalid bits are masked correctly" )
    {
        // Test with invalid high bits
        auto flags = FlagSet< Permissions >::fromUnderlying( 0xFF ); // 11111111

        // Should only keep valid bits (0-2 for Permissions)
        REQUIRE( flags.toUnderlying() == 7u ); // 00000111
        REQUIRE( flags.isValid() );
    }

    SECTION( "fromEnum validation" )
    {
        // Valid enum values
        auto validResult = FlagSet< Permissions >::fromEnum( Permissions::READ );
        REQUIRE( validResult.has_value() );

        auto validAllResult = FlagSet< Permissions >::fromEnum( Permissions::ALL );
        REQUIRE( validAllResult.has_value() );

        // Invalid enum value (has bits outside ALL mask)
        auto invalidFlag = static_cast< Permissions >( 16 ); // Bit 4, outside ALL
        auto invalidResult = FlagSet< Permissions >::fromEnum( invalidFlag );
        REQUIRE_FALSE( invalidResult.has_value() );
        REQUIRE( invalidResult.error().type == Error::ErrorType::VALUE_ERROR );
    }

    SECTION( "isValid static vs instance consistency" )
    {
        // Test that static and instance isValid are consistent
        auto testValues = { 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 15u, 16u };

        for( auto value : testValues ) {
            auto enumVal = static_cast< Permissions >( value );
            bool staticValid = FlagSet< Permissions >::isValid( enumVal );

            auto flags = FlagSet< Permissions >::fromUnderlying( value );
            bool instanceValid = flags.isValid();

            if( value <= 7u ) { // Within ALL mask
                REQUIRE( staticValid );
                REQUIRE( instanceValid );
            }
            else { // Outside ALL mask
                REQUIRE_FALSE( staticValid );
                REQUIRE( instanceValid ); // Instance should be valid after masking
            }
        }
    }
}

TEST_CASE( "FlagSet error handling completeness" )
{
    SECTION( "All operations handle invalid enums" )
    {
        FlagSet< Permissions > flags;
        auto invalidFlag = static_cast< Permissions >( 32 ); // Way outside valid range

        // All operations should return errors for invalid flags
        auto hasResult = flags.has( invalidFlag );
        REQUIRE_FALSE( hasResult.has_value() );
        REQUIRE( hasResult.error().type == Error::ErrorType::VALUE_ERROR );

        auto setResult = flags.set( invalidFlag );
        REQUIRE_FALSE( setResult.has_value() );
        REQUIRE( setResult.error().type == Error::ErrorType::VALUE_ERROR );

        auto clearResult = flags.clear( invalidFlag );
        REQUIRE_FALSE( clearResult.has_value() );
        REQUIRE( clearResult.error().type == Error::ErrorType::VALUE_ERROR );

        auto toggleResult = flags.toggle( invalidFlag );
        REQUIRE_FALSE( toggleResult.has_value() );
        REQUIRE( toggleResult.error().type == Error::ErrorType::VALUE_ERROR );
    }

    SECTION( "Error messages are informative" )
    {
        FlagSet< Permissions > flags;
        auto invalidFlag = static_cast< Permissions >( 64 );

        auto hasResult = flags.has( invalidFlag );
        REQUIRE_FALSE( hasResult.has_value() );
        auto errorStr = hasResult.error().toStr();
        REQUIRE( std::string( errorStr ).find( "Invalid enum value" ) != std::string::npos );

        auto fromEnumResult = FlagSet< Permissions >::fromEnum( invalidFlag );
        REQUIRE_FALSE( fromEnumResult.has_value() );
        auto fromEnumErrorStr = fromEnumResult.error().toStr();
        REQUIRE(
            std::string( fromEnumErrorStr ).find( "Invalid enum value for FlagSet" ) != std::string::npos );
    }
}

TEST_CASE( "FlagSet operator correctness and completeness" )
{
    SECTION( "Bitwise operators produce correct results" )
    {
        FlagSet< NetworkFlags > flags1( NetworkFlags::TCP | NetworkFlags::IPV6 ); // 1001
        FlagSet< NetworkFlags > flags2( NetworkFlags::UDP | NetworkFlags::IPV6 ); // 1010

        // OR: should have TCP, UDP, IPV6
        auto orResult = flags1 | flags2;
        REQUIRE( orResult.toUnderlying() == 11u ); // 1011

        // AND: should have only IPV6
        auto andResult = flags1 & flags2;
        REQUIRE( andResult.toUnderlying() == 8u ); // 1000 (IPV6 only)

        // XOR: should have TCP and UDP (not IPV6)
        auto xorResult = flags1 ^ flags2;
        REQUIRE( xorResult.toUnderlying() == 3u ); // 0011 (TCP | UDP)

        // NOT: should flip all valid bits
        auto notResult = ~flags1;
        u16 expected = static_cast< u16 >( NetworkFlags::ALL ) ^ static_cast< u16 >( flags1.toEnum() );
        REQUIRE( notResult.toUnderlying() == expected );
    }

    SECTION( "Assignment operators modify correctly" )
    {
        FlagSet< Permissions > flags( Permissions::READ );
        auto original = flags.toUnderlying();

        // Test |=
        flags |= FlagSet< Permissions >( Permissions::WRITE );
        REQUIRE( flags.toUnderlying() == ( original | 2u ) );

        // Test &=
        flags &= FlagSet< Permissions >( Permissions::READ | Permissions::EXECUTE );
        REQUIRE( flags.toUnderlying() == 1u ); // Only READ remains

        // Test ^=
        flags ^= FlagSet< Permissions >( Permissions::WRITE );
        REQUIRE( flags.toUnderlying() == 3u ); // READ | WRITE

        flags ^= FlagSet< Permissions >( Permissions::READ );
        REQUIRE( flags.toUnderlying() == 2u ); // Only WRITE remains
    }

    SECTION( "Operator chaining works correctly" )
    {
        FlagSet< Permissions > flags1( Permissions::READ );
        FlagSet< Permissions > flags2( Permissions::WRITE );
        FlagSet< Permissions > flags3( Permissions::EXECUTE );

        auto result = ( flags1 | flags2 ) & ( flags2 | flags3 );
        auto hasRead = result.has( Permissions::READ );
        auto hasWrite = result.has( Permissions::WRITE );
        auto hasExecute = result.has( Permissions::EXECUTE );

        REQUIRE( hasRead.has_value() );
        REQUIRE_FALSE( hasRead.value() );
        REQUIRE( hasWrite.has_value() );
        REQUIRE( hasWrite.value() );
        REQUIRE( hasExecute.has_value() );
        REQUIRE_FALSE( hasExecute.value() );
    }
}

TEST_CASE( "FlagSet conversion and explicit operators" )
{
    SECTION( "Explicit bool conversion" )
    {
        FlagSet< Permissions > empty;
        FlagSet< Permissions > nonEmpty( Permissions::READ );

        REQUIRE_FALSE( static_cast< bool >( empty ) );
        REQUIRE( static_cast< bool >( nonEmpty ) );

        // Test that hasAny() and explicit bool are consistent
        REQUIRE( static_cast< bool >( empty ) == empty.hasAny() );
        REQUIRE( static_cast< bool >( nonEmpty ) == nonEmpty.hasAny() );
    }

    SECTION( "Explicit underlying conversion" )
    {
        FlagSet< NetworkFlags > flags( NetworkFlags::TCP | NetworkFlags::ENCRYPTED );
        u16 underlying = static_cast< u16 >( flags );
        REQUIRE( underlying == 17u ); // TCP(1) | ENCRYPTED(16) = 17
        REQUIRE( underlying == flags.toUnderlying() );
    }

    SECTION( "Explicit enum conversion" )
    {
        FlagSet< Permissions > singleFlag( Permissions::WRITE );
        Permissions enumVal = static_cast< Permissions >( singleFlag );
        REQUIRE( enumVal == Permissions::WRITE );
        REQUIRE( enumVal == singleFlag.toEnum() );

        // Test with combined flags
        FlagSet< Permissions > multiFlag( Permissions::READ | Permissions::EXECUTE );
        Permissions combinedEnum = static_cast< Permissions >( multiFlag );
        REQUIRE( combinedEnum == ( Permissions::READ | Permissions::EXECUTE ) );
    }
}

TEST_CASE( "FlagSet bulk operations comprehensive" )
{
    SECTION( "setAll sets exactly the right bits" )
    {
        FlagSet< NetworkFlags > flags;
        REQUIRE( flags.setAll().has_value() );

        // Should have exactly the bits in ALL
        REQUIRE( flags.toUnderlying() == static_cast< u16 >( NetworkFlags::ALL ) );

        // Should have each individual flag
        auto hasTcp = flags.has( NetworkFlags::TCP );
        auto hasUdp = flags.has( NetworkFlags::UDP );
        auto hasIpv6 = flags.has( NetworkFlags::IPV6 );
        auto hasEncrypted = flags.has( NetworkFlags::ENCRYPTED );

        REQUIRE( hasTcp.has_value() );
        REQUIRE( hasTcp.value() );
        REQUIRE( hasUdp.has_value() );
        REQUIRE( hasUdp.value() );
        REQUIRE( hasIpv6.has_value() );
        REQUIRE( hasIpv6.value() );
        REQUIRE( hasEncrypted.has_value() );
        REQUIRE( hasEncrypted.value() );
    }

    SECTION( "toggleAll behaves correctly" )
    {
        FlagSet< Permissions > flags( Permissions::READ );

        REQUIRE( flags.toggleAll().has_value() );

        // READ should now be off, WRITE and EXECUTE should be on
        auto hasRead = flags.has( Permissions::READ );
        auto hasWrite = flags.has( Permissions::WRITE );
        auto hasExecute = flags.has( Permissions::EXECUTE );

        REQUIRE( hasRead.has_value() );
        REQUIRE_FALSE( hasRead.value() );
        REQUIRE( hasWrite.has_value() );
        REQUIRE( hasWrite.value() );
        REQUIRE( hasExecute.has_value() );
        REQUIRE( hasExecute.value() );

        // Toggle again should restore original
        REQUIRE( flags.toggleAll().has_value() );

        auto hasRead2 = flags.has( Permissions::READ );
        auto hasWrite2 = flags.has( Permissions::WRITE );
        auto hasExecute2 = flags.has( Permissions::EXECUTE );

        REQUIRE( hasRead2.has_value() );
        REQUIRE( hasRead2.value() );
        REQUIRE( hasWrite2.has_value() );
        REQUIRE_FALSE( hasWrite2.value() );
        REQUIRE( hasExecute2.has_value() );
        REQUIRE_FALSE( hasExecute2.value() );
    }
}

TEST_CASE( "FlagSet constexpr functionality" )
{
    SECTION( "Constexpr construction and basic operations" )
    {
        constexpr FlagSet< Permissions > flags( Permissions::READ );

        static_assert( flags.toUnderlying() == 1u );
        static_assert( flags.toEnum() == Permissions::READ );
        static_assert( flags.isValid() );
    }

    SECTION( "Constexpr static operations" )
    {
        static_assert( FlagSet< Permissions >::isValid( Permissions::READ ) );
        static_assert( FlagSet< Permissions >::isValid( Permissions::ALL ) );
        static_assert( !FlagSet< Permissions >::isValid( static_cast< Permissions >( 16 ) ) );
    }

    SECTION( "Constexpr operators" )
    {
        constexpr FlagSet< Permissions > flags1( Permissions::READ );
        constexpr FlagSet< Permissions > flags2( Permissions::WRITE );

        constexpr auto orResult = flags1 | flags2;
        static_assert( orResult.toUnderlying() == 3u );

        constexpr auto andResult = flags1 & flags2;
        static_assert( andResult.toUnderlying() == 0u );

        constexpr auto notResult = ~flags1;
        static_assert( notResult.toUnderlying() == 6u ); // ~1 & 7 = 6
    }
}

TEST_CASE( "FlagSet performance characteristics" )
{
    SECTION( "Large enum type performance" )
    {
        // Test with a larger enum to ensure scalability
        enum class LargeFlags : u32
        {
            NONE = 0,
            FLAG0 = 1,
            FLAG1 = 2,
            FLAG2 = 4,
            FLAG3 = 8,
            FLAG4 = 16,
            FLAG5 = 32,
            FLAG6 = 64,
            FLAG7 = 128,
            ALL = FLAG0 | FLAG1 | FLAG2 | FLAG3 | FLAG4 | FLAG5 | FLAG6 | FLAG7
        };

        FlagSet< LargeFlags > flags;

        // Should handle larger flag sets efficiently
        REQUIRE( flags.setAll().has_value() );
        REQUIRE( flags.count() == 8 );

        // Iteration should work with larger sets
        usize iterCount = 0;
        for( auto flag : flags ) {
            ++iterCount;
            (void)flag;
        }
        REQUIRE( iterCount == 8 );
    }
}

// Original basic tests (kept for backward compatibility)
TEST_CASE( "Basic FlagSet functionality" )
{
    FlagSet< Permissions > flags;

    // Test empty
    REQUIRE( flags.hasNone() );
    REQUIRE_FALSE( flags.hasAny() );

    // Test setting a flag
    auto setResult = flags.set( Permissions::READ );
    REQUIRE( setResult.has_value() );

    auto hasRead = flags.has( Permissions::READ );
    REQUIRE( hasRead.has_value() );
    REQUIRE( hasRead.value() );

    auto hasWrite = flags.has( Permissions::WRITE );
    REQUIRE( hasWrite.has_value() );
    REQUIRE_FALSE( hasWrite.value() );
    REQUIRE( flags.hasAny() );

    // Test construction from enum
    FlagSet< Permissions > flags2( Permissions::WRITE );
    auto hasWrite2 = flags2.has( Permissions::WRITE );
    REQUIRE( hasWrite2.has_value() );
    REQUIRE( hasWrite2.value() );

    // Test bitwise OR
    auto combined = flags | flags2;
    auto combinedRead = combined.has( Permissions::READ );
    auto combinedWrite = combined.has( Permissions::WRITE );
    REQUIRE( combinedRead.has_value() );
    REQUIRE( combinedRead.value() );
    REQUIRE( combinedWrite.has_value() );
    REQUIRE( combinedWrite.value() );
}
