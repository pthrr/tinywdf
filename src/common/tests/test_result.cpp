#include <catch2/catch_test_macros.hpp>

#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <common/result.hpp>
#include <common/types.hpp>

using namespace cmn::result;
using namespace cmn::types;

TEST_CASE( "Error construction and string conversion", "[error]" )
{
    SECTION( "Default constructor" )
    {
        Error e;
        REQUIRE( e.type == Error::ErrorType::GENERIC_ERROR );
        REQUIRE( std::string( e.message ) == "" );
    }

    SECTION( "Constructor with message and type" )
    {
        Error e( "test error", Error::ErrorType::VALUE_ERROR );
        REQUIRE( e.type == Error::ErrorType::VALUE_ERROR );
        REQUIRE( std::string( e.message ) == "test error" );
    }

    SECTION( "Constructor with message only" )
    {
        Error e( "test error" );
        REQUIRE( e.type == Error::ErrorType::GENERIC_ERROR );
        REQUIRE( std::string( e.message ) == "test error" );
    }

    SECTION( "Error type to string conversion - static method" )
    {
        REQUIRE( std::string( Error::toStr( Error::ErrorType::VALUE_ERROR ) ) == "ValueError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::INDEX_ERROR ) ) == "IndexError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::GENERIC_ERROR ) ) == "GenericError" );
    }

    SECTION( "Error instance toStr() method" )
    {
        Error e( "division by zero", Error::ErrorType::ZERO_DIVISION_ERROR );
        auto str_result = e.toStr();
        REQUIRE( std::string( str_result ) == "ZeroDivisionError: division by zero" );
    }

    SECTION( "All error types have string representations" )
    {
        // Test all enum values to ensure none are missing
        REQUIRE( std::string( Error::toStr( Error::ErrorType::ARITHMETIC_ERROR ) ) == "ArithmeticError" );
        REQUIRE(
            std::string( Error::toStr( Error::ErrorType::FLOATING_POINT_ERROR ) ) == "FloatingPointError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::OVERFLOW_ERROR ) ) == "OverflowError" );
        REQUIRE(
            std::string( Error::toStr( Error::ErrorType::ZERO_DIVISION_ERROR ) ) == "ZeroDivisionError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::ASSERTION_ERROR ) ) == "AssertionError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::ATTRIBUTE_ERROR ) ) == "AttributeError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::INDEX_ERROR ) ) == "IndexError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::KEY_ERROR ) ) == "KeyError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::OS_ERROR ) ) == "OSError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::TIMEOUT_ERROR ) ) == "TimeoutError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::RUNTIME_ERROR ) ) == "RuntimeError" );
        REQUIRE(
            std::string( Error::toStr( Error::ErrorType::NOT_IMPLEMENTED_ERROR ) ) == "NotImplementedError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::SYNTAX_ERROR ) ) == "SyntaxError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::SYSTEM_ERROR ) ) == "SystemError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::TYPE_ERROR ) ) == "TypeError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::VALUE_ERROR ) ) == "ValueError" );
        REQUIRE( std::string( Error::toStr( Error::ErrorType::GENERIC_ERROR ) ) == "GenericError" );
    }

    SECTION( "Empty message handling" )
    {
        Error e( "", Error::ErrorType::RUNTIME_ERROR );
        auto str_result = e.toStr();
        REQUIRE( std::string( str_result ) == "RuntimeError: " );
    }
}

TEST_CASE( "Result operations with std::expected", "[result]" )
{
    SECTION( "Success case with copy" )
    {
        auto result = ok( 42 );
        REQUIRE( result.has_value() );
        REQUIRE( result.value() == 42 );
        REQUIRE( unwrap( result ) == 42 );
    }

    SECTION( "Success case with move" )
    {
        auto result = ok( std::string( "movable" ) );
        REQUIRE( result.has_value() );
        auto moved = unwrap( std::move( result ) );
        REQUIRE( moved == "movable" );
    }

    SECTION( "Error case using std::expected interface" )
    {
        auto result = Result< int >( err( "test error", Error::ErrorType::VALUE_ERROR ) );
        REQUIRE( !result.has_value() );
        REQUIRE( result.error().type == Error::ErrorType::VALUE_ERROR );
        REQUIRE( std::string( result.error().message ) == "test error" );
    }

    SECTION( "std::expected specific operations" )
    {
        auto result = ok( 42 );

        // Test operator bool
        REQUIRE( static_cast< bool >( result ) );

        // Test operator*
        REQUIRE( *result == 42 );

        // Test operator->
        auto string_result = ok( std::string( "test" ) );
        REQUIRE( string_result->length() == 4 );

        // Test value_or
        auto error_result = Result< int >( err( "error" ) );
        REQUIRE( error_result.value_or( 999 ) == 999 );
        REQUIRE( result.value_or( 999 ) == 42 );
    }

    SECTION( "Status success" )
    {
        auto status = ok();
        REQUIRE( status.has_value() );
        verify( status ); // should not abort
    }

    SECTION( "Status error" )
    {
        auto status = Status( err( "test error" ) );
        REQUIRE( !status.has_value() );
        REQUIRE( std::string( status.error().message ) == "test error" );
    }

    SECTION( "Result<void> operations" )
    {
        auto void_result = ok< void >();
        REQUIRE( void_result.has_value() );

        auto void_error = Result< void >( err( "void error" ) );
        REQUIRE( !void_error.has_value() );
    }
}

TEST_CASE( "Helper functions", "[helpers]" )
{
    SECTION( "ok() with value - copy semantics" )
    {
        auto result = ok( std::string( "hello" ) );
        REQUIRE( result.has_value() );
        REQUIRE( result.value() == "hello" );
    }

    SECTION( "ok() with value - move semantics" )
    {
        std::string str = "moveable";
        auto result = ok( std::move( str ) );
        REQUIRE( result.has_value() );
        REQUIRE( result.value() == "moveable" );
        // str should be moved from (though we can't reliably test its state)
    }

    SECTION( "ok() template specialization" )
    {
        auto int_result = ok< int >();
        REQUIRE( int_result.has_value() );
        // Default constructed int should be 0
        REQUIRE( int_result.value() == 0 );
    }

    SECTION( "ok() for Status" )
    {
        auto status = ok();
        REQUIRE( status.has_value() );
        static_assert( std::is_same_v< decltype( status ), Status > );
    }

    SECTION( "err() function with explicit type" )
    {
        auto error = err( "test message", Error::ErrorType::RUNTIME_ERROR );
        REQUIRE( error.error().type == Error::ErrorType::RUNTIME_ERROR );
        REQUIRE( std::string( error.error().message ) == "test message" );
    }

    SECTION( "err() function with default type" )
    {
        auto error = err( "generic error" );
        REQUIRE( error.error().type == Error::ErrorType::GENERIC_ERROR );
        REQUIRE( std::string( error.error().message ) == "generic error" );
    }

    SECTION( "Perfect forwarding in ok()" )
    {
        struct NonCopyable
        {
            std::unique_ptr< int > ptr;
            NonCopyable()
                : ptr( std::make_unique< int >( 42 ) )
            {
            }
            NonCopyable( const NonCopyable& ) = delete;
            NonCopyable& operator=( const NonCopyable& ) = delete;
            NonCopyable( NonCopyable&& ) = default;
            NonCopyable& operator=( NonCopyable&& ) = default;
        };

        NonCopyable nc;
        auto result = ok( std::move( nc ) );
        REQUIRE( result.has_value() );
        REQUIRE( *result.value().ptr == 42 );
    }
}

TEST_CASE( "Constexpr functionality", "[constexpr]" )
{
    SECTION( "Constexpr Error construction" )
    {
        constexpr Error e1;
        static_assert( e1.type == Error::ErrorType::GENERIC_ERROR );

        constexpr Error e2( "test", Error::ErrorType::VALUE_ERROR );
        static_assert( e2.type == Error::ErrorType::VALUE_ERROR );
    }

    SECTION( "Constexpr Error::toStr" )
    {
        constexpr auto type_str = Error::toStr( Error::ErrorType::INDEX_ERROR );
        static_assert( std::string_view( type_str ) == "IndexError" );
    }

    SECTION( "Constexpr ok() operations" )
    {
        constexpr auto result = ok( 42 );
        static_assert( result.has_value() );
        static_assert( result.value() == 42 );
    }

    SECTION( "All error types are constexpr convertible" )
    {
        // Test each error type can be converted at compile time
        static_assert( Error::toStr( Error::ErrorType::ARITHMETIC_ERROR )[0] == 'A' );
        static_assert( Error::toStr( Error::ErrorType::FLOATING_POINT_ERROR )[0] == 'F' );
        static_assert( Error::toStr( Error::ErrorType::OVERFLOW_ERROR )[0] == 'O' );
        static_assert( Error::toStr( Error::ErrorType::ZERO_DIVISION_ERROR )[0] == 'Z' );

        // Test string view comparison works at compile time
        constexpr auto runtime_str = Error::toStr( Error::ErrorType::RUNTIME_ERROR );
        static_assert( std::string_view( runtime_str ) == "RuntimeError" );
    }

    SECTION( "Constexpr Result operations" )
    {
        // Test more complex constexpr scenarios
        constexpr auto make_result = []( int x ) -> Result< int > {
            if( x < 0 ) {
                return err( "negative value" );
            }
            return ok( x * 2 );
        };

        constexpr auto result1 = make_result( 5 );
        static_assert( result1.has_value() );
        static_assert( result1.value() == 10 );

        constexpr auto result2 = make_result( -1 );
        static_assert( !result2.has_value() );
    }
}

TEST_CASE( "Thread-local buffer behavior", "[buffer][threading]" )
{
    SECTION( "Buffer reuse within same thread" )
    {
        Error e1( "first error", Error::ErrorType::VALUE_ERROR );
        Error e2( "second error", Error::ErrorType::INDEX_ERROR );

        auto str1 = e1.toStr();
        std::string saved1( str1 ); // Save the string content

        auto str2 = e2.toStr();
        std::string saved2( str2 );

        // Verify both strings were formatted correctly
        REQUIRE( saved1 == "ValueError: first error" );
        REQUIRE( saved2 == "IndexError: second error" );
    }

    SECTION( "Buffer truncation with very long messages" )
    {
        std::string long_msg( 300, 'A' ); // Longer than 256 char buffer
        Error e( long_msg.c_str(), Error::ErrorType::RUNTIME_ERROR );

        auto str_result = e.toStr();
        std::string result( str_result );

        // Should be truncated and still contain the error type
        REQUIRE( result.find( "RuntimeError:" ) == 0 );
        REQUIRE( result.length() < 256 );                     // Should fit in buffer
        REQUIRE( result.find( "AAA" ) != std::string::npos ); // Should contain part of message
    }

    SECTION( "Concurrent access from multiple threads" )
    {
        const int num_threads = 4;
        const int iterations = 100;

        std::vector< std::future< bool > > futures;

        for( int i = 0; i < num_threads; ++i ) {
            futures.push_back( std::async( std::launch::async, [i, iterations]() {
                for( int j = 0; j < iterations; ++j ) {
                    std::string msg = "Thread " + std::to_string( i ) + " iteration " + std::to_string( j );
                    Error e( msg.c_str(), Error::ErrorType::RUNTIME_ERROR );
                    auto str_result = e.toStr();
                    std::string result( str_result );

                    // Verify the result contains our thread-specific message
                    if( result.find( "Thread " + std::to_string( i ) ) == std::string::npos ) {
                        return false;
                    }
                }
                return true;
            } ) );
        }

        // Wait for all threads and verify results
        for( auto& future : futures ) {
            REQUIRE( future.get() );
        }
    }

    SECTION( "Empty string when snprintf fails" )
    {
        // Test edge case where snprintf might fail (though it's hard to trigger)
        Error e( "", Error::ErrorType::GENERIC_ERROR );
        auto str_result = e.toStr();

        // Should handle gracefully - either succeed or return empty string
        std::string result( str_result );
        // At minimum should handle without crashing
        REQUIRE( true ); // If we get here, it didn't crash
    }
}

TEST_CASE( "std::expected integration", "[expected]" )
{
    SECTION( "Transform operations" )
    {
        auto result = ok( 42 );

        // Test transform (maps value if present)
        auto doubled = result.transform( []( int x ) { return x * 2; } );
        REQUIRE( doubled.has_value() );
        REQUIRE( doubled.value() == 84 );

        // Test transform on error
        auto error_result = Result< int >( err( "error" ) );
        auto transformed_error = error_result.transform( []( int x ) { return x * 2; } );
        REQUIRE( !transformed_error.has_value() );
    }

    SECTION( "Transform_error operations" )
    {
        auto error_result = Result< int >( err( "original error", Error::ErrorType::VALUE_ERROR ) );

        // Transform the error
        auto new_error = error_result.transform_error(
            []( const Error& e ) { return Error( "transformed error", Error::ErrorType::RUNTIME_ERROR ); } );

        REQUIRE( !new_error.has_value() );
        REQUIRE( new_error.error().type == Error::ErrorType::RUNTIME_ERROR );
        REQUIRE( std::string( new_error.error().message ) == "transformed error" );
    }

    SECTION( "and_then operations (monadic bind)" )
    {
        auto safe_divide = []( int a, int b ) -> Result< double > {
            if( b == 0 ) {
                return err( "Division by zero", Error::ErrorType::ZERO_DIVISION_ERROR );
            }
            return ok( static_cast< double >( a ) / b );
        };

        auto result = ok( 10 );
        auto chained = result.and_then( [&]( int x ) { return safe_divide( x, 2 ); } );

        REQUIRE( chained.has_value() );
        REQUIRE( chained.value() == 5.0 );

        // Test error propagation
        auto error_chained = result.and_then( [&]( int x ) { return safe_divide( x, 0 ); } );

        REQUIRE( !error_chained.has_value() );
        REQUIRE( error_chained.error().type == Error::ErrorType::ZERO_DIVISION_ERROR );
    }

    SECTION( "or_else operations" )
    {
        auto error_result = Result< int >( err( "error", Error::ErrorType::VALUE_ERROR ) );

        auto recovered = error_result.or_else( []( const Error& ) -> Result< int > {
            return ok( 42 ); // Provide default value
        } );

        REQUIRE( recovered.has_value() );
        REQUIRE( recovered.value() == 42 );

        // Test successful case (should not call or_else)
        auto success_result = ok( 100 );
        auto not_recovered =
            success_result.or_else( []( const Error& ) -> Result< int > { return ok( 42 ); } );

        REQUIRE( not_recovered.has_value() );
        REQUIRE( not_recovered.value() == 100 ); // Original value preserved
    }
}

TEST_CASE( "Error propagation patterns", "[propagation]" )
{
    auto divide = []( int a, int b ) -> Result< double > {
        if( b == 0 ) {
            return err( "Division by zero", Error::ErrorType::ZERO_DIVISION_ERROR );
        }
        return ok( static_cast< double >( a ) / b );
    };

    SECTION( "Success case" )
    {
        auto result = divide( 10, 2 );
        REQUIRE( result.has_value() );
        REQUIRE( result.value() == 5.0 );
    }

    SECTION( "Error case" )
    {
        auto result = divide( 10, 0 );
        REQUIRE( !result.has_value() );
        REQUIRE( result.error().type == Error::ErrorType::ZERO_DIVISION_ERROR );
    }

    SECTION( "Chained operations with early return" )
    {
        auto complex_calculation = [&divide]( int a, int b, int c ) -> Result< double > {
            auto step1 = divide( a, b );
            if( !step1 )
                return step1;

            auto step2 = divide( static_cast< int >( step1.value() ), c );
            if( !step2 )
                return step2;

            return ok( step2.value() + 1.0 );
        };

        // Success case
        auto result1 = complex_calculation( 20, 2, 5 );
        REQUIRE( result1.has_value() );
        REQUIRE( result1.value() == 3.0 ); // (20/2)/5 + 1 = 2 + 1 = 3

        // Error in first step
        auto result2 = complex_calculation( 20, 0, 5 );
        REQUIRE( !result2.has_value() );
        REQUIRE( result2.error().type == Error::ErrorType::ZERO_DIVISION_ERROR );

        // Error in second step
        auto result3 = complex_calculation( 20, 2, 0 );
        REQUIRE( !result3.has_value() );
        REQUIRE( result3.error().type == Error::ErrorType::ZERO_DIVISION_ERROR );
    }
}

TEST_CASE( "Type deduction and templates", "[templates]" )
{
    SECTION( "Auto deduction with ok()" )
    {
        auto int_result = ok( 42 );
        static_assert( std::is_same_v< decltype( int_result ), Result< int > > );

        auto string_result = ok( std::string( "test" ) );
        static_assert( std::is_same_v< decltype( string_result ), Result< std::string > > );
    }

    SECTION( "Custom types" )
    {
        struct CustomType
        {
            int value;
            bool operator==( const CustomType& other ) const
            {
                return value == other.value;
            }
        };

        auto result = ok( CustomType{ 42 } );
        REQUIRE( result.has_value() );
        REQUIRE( result.value().value == 42 );
    }

    SECTION( "Move-only types" )
    {
        auto ptr = std::make_unique< int >( 42 );
        auto result = ok( std::move( ptr ) );
        REQUIRE( result.has_value() );
        REQUIRE( *result.value() == 42 );
    }

    SECTION( "Reference wrapper types" )
    {
        int value = 42;
        auto result = ok( std::ref( value ) );
        REQUIRE( result.has_value() );
        REQUIRE( result.value().get() == 42 );

        value = 100;
        REQUIRE( result.value().get() == 100 ); // Reference should track changes
    }
}

TEST_CASE( "Error message edge cases", "[formatting]" )
{
    SECTION( "Special characters in error messages" )
    {
        Error e( "Error with\nnewlines\tand\ttabs", Error::ErrorType::SYNTAX_ERROR );
        auto str_result = e.toStr();
        std::string result( str_result );

        REQUIRE( result.find( "SyntaxError:" ) == 0 );
        REQUIRE( result.find( "newlines" ) != std::string::npos );
        REQUIRE( result.find( "tabs" ) != std::string::npos );
    }

    SECTION( "Very long error messages" )
    {
        std::string long_msg( 300, 'x' ); // Longer than buffer size
        Error e( long_msg.c_str(), Error::ErrorType::RUNTIME_ERROR );
        auto str_result = e.toStr();
        std::string result( str_result );

        // Should handle gracefully (truncate or return partial)
        REQUIRE( !result.empty() );
        REQUIRE( result.find( "RuntimeError:" ) == 0 );
    }

    SECTION( "Error message with format specifiers" )
    {
        Error e( "Error with %s and %d", Error::ErrorType::RUNTIME_ERROR );
        auto str_result = e.toStr();
        std::string result( str_result );

        // Should not interpret format specifiers
        REQUIRE( result.find( "%s" ) != std::string::npos );
        REQUIRE( result.find( "%d" ) != std::string::npos );
    }
}

// Helper for tracking copy/move operations - defined outside test functions
struct TrackingType
{
    static int copy_count;
    static int move_count;

    int value;

    TrackingType( int v )
        : value( v )
    {
    }

    TrackingType( const TrackingType& other )
        : value( other.value )
    {
        ++copy_count;
    }

    TrackingType( TrackingType&& other ) noexcept
        : value( other.value )
    {
        ++move_count;
    }

    TrackingType& operator=( const TrackingType& other )
    {
        if( this != &other ) {
            value = other.value;
            ++copy_count;
        }
        return *this;
    }

    TrackingType& operator=( TrackingType&& other ) noexcept
    {
        if( this != &other ) {
            value = other.value;
            ++move_count;
        }
        return *this;
    }

    static void reset()
    {
        copy_count = 0;
        move_count = 0;
    }
};

// Static member definitions
int TrackingType::copy_count = 0;
int TrackingType::move_count = 0;

TEST_CASE( "Performance and memory characteristics", "[performance]" )
{
    SECTION( "Copy vs move semantics tracking" )
    {
        TrackingType::reset();

        TrackingType obj( 42 );
        auto result = ok( std::move( obj ) );

        // Should prefer move over copy
        REQUIRE( TrackingType::move_count > 0 );
        REQUIRE( result.has_value() );
        REQUIRE( result.value().value == 42 );
    }

    SECTION( "Self-assignment safety" )
    {
        auto result = ok( 42 );
        result = result; // Self-assignment should be safe
        REQUIRE( result.has_value() );
        REQUIRE( result.value() == 42 );
    }
}

TEST_CASE( "Comprehensive error type verification", "[error_types]" )
{
    SECTION( "Create errors of each type and verify formatting" )
    {
        struct ErrorTestCase
        {
            Error::ErrorType type;
            const char* expected_prefix;
            const char* message;
        };

        const std::array< ErrorTestCase, 17 > test_cases = {
            { { Error::ErrorType::ARITHMETIC_ERROR, "ArithmeticError", "arithmetic failed" },
                { Error::ErrorType::FLOATING_POINT_ERROR, "FloatingPointError", "float error" },
                { Error::ErrorType::OVERFLOW_ERROR, "OverflowError", "overflow occurred" },
                { Error::ErrorType::ZERO_DIVISION_ERROR, "ZeroDivisionError", "division by zero" },
                { Error::ErrorType::ASSERTION_ERROR, "AssertionError", "assertion failed" },
                { Error::ErrorType::ATTRIBUTE_ERROR, "AttributeError", "no such attribute" },
                { Error::ErrorType::INDEX_ERROR, "IndexError", "index out of bounds" },
                { Error::ErrorType::KEY_ERROR, "KeyError", "key not found" },
                { Error::ErrorType::OS_ERROR, "OSError", "operating system error" },
                { Error::ErrorType::TIMEOUT_ERROR, "TimeoutError", "operation timed out" },
                { Error::ErrorType::RUNTIME_ERROR, "RuntimeError", "runtime failure" },
                { Error::ErrorType::NOT_IMPLEMENTED_ERROR, "NotImplementedError", "not implemented" },
                { Error::ErrorType::SYNTAX_ERROR, "SyntaxError", "syntax is invalid" },
                { Error::ErrorType::SYSTEM_ERROR, "SystemError", "system failure" },
                { Error::ErrorType::TYPE_ERROR, "TypeError", "wrong type" },
                { Error::ErrorType::VALUE_ERROR, "ValueError", "invalid value" },
                { Error::ErrorType::GENERIC_ERROR, "GenericError", "generic failure" } } };

        for( const auto& test_case : test_cases ) {
            Error e( test_case.message, test_case.type );
            auto str_result = e.toStr();
            std::string formatted( str_result );

            std::string expected = std::string( test_case.expected_prefix ) + ": " + test_case.message;
            REQUIRE( formatted == expected );
        }
    }
}

// Note: Testing unwrap() and verify() abort behavior requires death tests
// which aren't available in standard Catch2
TEST_CASE( "Unwrap and verify behavior documentation", "[unwrap]" )
{
    SECTION( "unwrap on success returns value" )
    {
        auto result = ok( 123 );
        REQUIRE( unwrap( result ) == 123 );
        REQUIRE( unwrap( ok( 456 ) ) == 456 ); // rvalue case
    }

    SECTION( "verify on success does nothing" )
    {
        auto status = ok();
        verify( status ); // Should not abort
        REQUIRE( true );  // If we get here, verify didn't abort
    }

    SECTION( "unwrap move semantics" )
    {
        auto string_result = ok( std::string( "movable" ) );
        auto moved = unwrap( std::move( string_result ) );
        REQUIRE( moved == "movable" );
    }
}
