#include <variant>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <node_arena.hpp>

using namespace dga;

TEST_CASE( "NodeArena - Tree Construction and Traversal" )
{
    using AnyNode = std::variant< Node >;
    using Arena = NodeArena< AnyNode, 8, 16 >;

    Arena arena;

    /*
        Tree:

            0
          / | \
         1  2  3
            |
            4
           / \
          5   6
    */

    arena.children = {
        NodeIdx{ 1 }, NodeIdx{ 2 }, NodeIdx{ 3 }, // [0] children of node 0
        NodeIdx{ 4 },                             // [3] child of node 2
        NodeIdx{ 5 }, NodeIdx{ 6 },               // [4,5] children of node 4
        {}, {}, {}, {}, {}, {}, {}, {}, {}        // unused
    };

    arena.nodes[0] = Node{ { 0 }, 0, 3 };
    arena.nodes[1] = Node{ { 1 }, 0, 0 };
    arena.nodes[2] = Node{ { 2 }, 3, 1 };
    arena.nodes[3] = Node{ { 3 }, 0, 0 };
    arena.nodes[4] = Node{ { 4 }, 4, 2 };
    arena.nodes[5] = Node{ { 5 }, 0, 0 };
    arena.nodes[6] = Node{ { 6 }, 0, 0 };

    SECTION( "getNodes() returns correct span" )
    {
        auto span = arena.getNodes();
        REQUIRE( span.size() == 8 );
        REQUIRE( std::get< Node >( span[0] ).idx.value == 0 );
        REQUIRE( std::get< Node >( span[4] ).idx.value == 4 );
    }

    SECTION( "getChildren() returns correct span" )
    {
        auto span = arena.getChildren();
        REQUIRE( span.size() == 16 );
        REQUIRE( span[0].value == 1 );
        REQUIRE( span[5].value == 6 );
    }

    SECTION( "getChildrenOf() returns correct slice" )
    {
        auto children0 = arena.getChildrenOf( arena.nodes[0] );
        REQUIRE( children0.size() == 3 );
        REQUIRE( children0[0].value == 1 );
        REQUIRE( children0[1].value == 2 );
        REQUIRE( children0[2].value == 3 );

        auto children4 = arena.getChildrenOf( arena.nodes[4] );
        REQUIRE( children4.size() == 2 );
        REQUIRE( children4[0].value == 5 );
        REQUIRE( children4[1].value == 6 );
    }

    SECTION( "dfs traversal (non-const)" )
    {
        std::vector< std::size_t > visited;
        arena.dfs(
            NodeIdx{ 0 }, [&]( AnyNode& node ) { visited.push_back( std::get< Node >( node ).idx.value ); } );

        REQUIRE( visited.size() == 7 );
        REQUIRE( visited[0] == 0 );
        REQUIRE( std::find( visited.begin(), visited.end(), 6 ) != visited.end() );
    }

    SECTION( "dfs traversal (const)" )
    {
        Arena const& constArena = arena;
        std::vector< std::size_t > visited;
        constArena.dfs( NodeIdx{ 0 },
            [&]( AnyNode const& node ) { visited.push_back( std::get< Node >( node ).idx.value ); } );

        REQUIRE( visited.size() == 7 );
        REQUIRE( visited[0] == 0 );
    }

    SECTION( "bfs traversal (non-const)" )
    {
        std::vector< std::size_t > visited;
        arena.bfs(
            NodeIdx{ 0 }, [&]( AnyNode& node ) { visited.push_back( std::get< Node >( node ).idx.value ); } );

        REQUIRE( visited.size() == 7 );
        REQUIRE( visited[0] == 0 );
        REQUIRE( visited[1] == 1 );
        REQUIRE( visited[2] == 2 );
        REQUIRE( visited[3] == 3 );
    }

    SECTION( "bfs traversal (const)" )
    {
        Arena const& constArena = arena;
        std::vector< std::size_t > visited;
        constArena.bfs( NodeIdx{ 0 },
            [&]( AnyNode const& node ) { visited.push_back( std::get< Node >( node ).idx.value ); } );

        REQUIRE( visited.size() == 7 );
        REQUIRE( visited[0] == 0 );
    }
}

struct ResistorNode : Node
{
    double resistance;

    constexpr ResistorNode( NodeIdx idx, std::size_t off, std::size_t count, double r )
        : resistance( r )
    {
        this->idx = idx;
        this->child_offset = off;
        this->child_count = count;
    }
};

struct CapacitorNode : Node
{
    double capacitance;

    constexpr CapacitorNode( NodeIdx idx, std::size_t off, std::size_t count, double c )
        : capacitance( c )
    {
        this->idx = idx;
        this->child_offset = off;
        this->child_count = count;
    }
};

template< class... Ts >
struct overload : Ts...
{
    using Ts::operator()...;
};
template< class... Ts >
overload( Ts... ) -> overload< Ts... >;

TEST_CASE( "NodeArena - Tree with ResistorNode and CapacitorNode" )
{
    using AnyNode = std::variant< Node, ResistorNode, CapacitorNode >;
    using Arena = NodeArena< AnyNode, 8, 16 >;

    Arena arena;

    /*
        Tree:

            0 (Resistor)
          / | \
         1  2  3 (Capacitor)
            |
            4 (Resistor)
           / \
         5(Cap) 6(Node)
    */

    arena.children = {
        NodeIdx{ 1 }, NodeIdx{ 2 }, NodeIdx{ 3 }, // 0
        NodeIdx{ 4 },                             // 2
        NodeIdx{ 5 }, NodeIdx{ 6 },               // 4
        {}, {}, {}, {}, {}, {}, {}, {}, {}        // unused
    };

    arena.nodes[0] = ResistorNode( { 0 }, 0, 3, 220 );
    arena.nodes[1] = Node{ .idx = { 1 }, .child_offset = 0, .child_count = 0 };
    arena.nodes[2] = Node{ .idx = { 2 }, .child_offset = 3, .child_count = 1 };
    arena.nodes[3] = CapacitorNode( { 3 }, 0, 0, 0.01 );
    arena.nodes[4] = ResistorNode( { 4 }, 4, 2, 1000 );
    arena.nodes[5] = CapacitorNode( { 5 }, 0, 0, 0.002 );
    arena.nodes[6] = Node{ .idx = { 6 }, .child_offset = 0, .child_count = 0 };

    SECTION( "count and accumulate properties" )
    {
        int total_resistance = 0;
        double total_capacitance = 0.0;
        int node_count = 0;

        arena.dfs( NodeIdx{ 0 }, [&]( const AnyNode& node ) {
            std::visit( overload{ [&]( const ResistorNode& r ) { total_resistance += r.resistance; },
                            [&]( const CapacitorNode& c ) { total_capacitance += c.capacitance; },
                            [&]( const Node& ) { ++node_count; } },
                node );
        } );

        REQUIRE( total_resistance == 1220 );
        REQUIRE( total_capacitance == Catch::Approx( 0.012 ) );
        REQUIRE( node_count == 3 ); // nodes 1, 2, 6
    }

    SECTION( "bfs prints type-tagged trace" )
    {
        std::vector< std::string > trace;
        arena.bfs( NodeIdx{ 0 }, [&]( const AnyNode& node ) {
            std::visit(
                overload{ [&]( const ResistorNode& r ) {
                             trace.push_back( "R(" + std::to_string( r.idx.value ) + ")" );
                         },
                    [&]( const CapacitorNode& c ) {
                        trace.push_back( "C(" + std::to_string( c.idx.value ) + ")" );
                    },
                    [&]( const Node& n ) { trace.push_back( "N(" + std::to_string( n.idx.value ) + ")" ); } },
                node );
        } );

        REQUIRE( trace.front() == "R(0)" );
        REQUIRE( trace.at( 3 ) == "C(3)" );
        REQUIRE( trace.back() == "N(6)" );
    }
}
