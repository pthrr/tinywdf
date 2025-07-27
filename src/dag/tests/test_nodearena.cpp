#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <set>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <dag/node_arena.hpp>

using namespace dag::node_arena;

// ============================================================================
// Test Node Types
// ============================================================================

struct TestNode : Node
{
    int data = 0;
    TestNode() = default;
    TestNode( NodeIdx idx, usize offset = 0, usize count = 0, int d = 0 )
        : Node{ idx, offset, count }
        , data( d )
    {
    }

    constexpr auto getData( this auto&& self ) -> decltype( auto )
    {
        return ( self.data );
    }
};

struct ResistorNode : Node
{
    double resistance;

    constexpr ResistorNode( NodeIdx idx, usize off, usize count, double r )
        : resistance( r )
    {
        this->idx = idx;
        this->child_offset = off;
        this->child_count = count;
    }

    constexpr auto getResistance( this auto&& self ) -> decltype( auto )
    {
        return ( self.resistance );
    }
};

struct CapacitorNode : Node
{
    double capacitance;

    constexpr CapacitorNode( NodeIdx idx, usize off, usize count, double c )
        : capacitance( c )
    {
        this->idx = idx;
        this->child_offset = off;
        this->child_count = count;
    }

    constexpr auto getCapacitance( this auto&& self ) -> decltype( auto )
    {
        return ( self.capacitance );
    }
};

// Visitor helper
template< class... Ts >
struct overload : Ts...
{
    using Ts::operator()...;
};
template< class... Ts >
overload( Ts... ) -> overload< Ts... >;

// Type aliases for different test scenarios
using SimpleNodeVariant = std::variant< Node >;
using TestNodeVariant = std::variant< TestNode >;
using ElectronicNodeVariant = std::variant< Node, ResistorNode, CapacitorNode >;

using SimpleArena = NodeArena< SimpleNodeVariant, 8, 16 >;
using TestArena = NodeArena< TestNodeVariant, 10, 20, float >;
using ElectronicArena = NodeArena< ElectronicNodeVariant, 8, 16 >;

// ============================================================================
// Helper Functions for Well-Formed Graphs
// ============================================================================

template< typename Arena >
void setupLinearChain( Arena& arena, usize length )
{
    for( usize i = 0; i < length - 1; ++i ) {
        arena.getNodes()[i] =
            typename Arena::NodeType{ TestNode{ NodeIdx{ i }, i, 1, static_cast< int >( i * 10 ) } };
        arena.getChildren()[i] = NodeIdx{ i + 1 };
        if( i < arena.getWeights().size() ) {
            arena.getWeights()[i] = 1.0f;
        }
    }
    arena.getNodes()[length - 1] = typename Arena::NodeType{
        TestNode{ NodeIdx{ length - 1 }, 0, 0, static_cast< int >( ( length - 1 ) * 10 ) } };
}

template< typename Arena >
void setupBinaryTree( Arena& arena )
{
    // Complete binary tree: nodes 0-6
    //       0
    //      / \
    //     1   2
    //    / \ / \
    //   3  4 5  6

    for( usize i = 0; i < 7; ++i ) {
        usize left_child = 2 * i + 1;
        usize right_child = 2 * i + 2;

        if( left_child < 7 && right_child < 7 ) {
            arena.getNodes()[i] =
                typename Arena::NodeType{ TestNode{ NodeIdx{ i }, 2 * i, 2, static_cast< int >( i ) } };
            arena.getChildren()[2 * i] = NodeIdx{ left_child };
            arena.getChildren()[2 * i + 1] = NodeIdx{ right_child };
        }
        else {
            arena.getNodes()[i] =
                typename Arena::NodeType{ TestNode{ NodeIdx{ i }, 0, 0, static_cast< int >( i ) } };
        }
    }
}

template< typename Arena >
void setupDiamondGraph( Arena& arena )
{
    // Diamond: 0 -> {1,2} -> 3
    arena.getNodes()[0] = typename Arena::NodeType{ TestNode{ NodeIdx{ 0 }, 0, 2, 0 } };
    arena.getNodes()[1] = typename Arena::NodeType{ TestNode{ NodeIdx{ 1 }, 2, 1, 1 } };
    arena.getNodes()[2] = typename Arena::NodeType{ TestNode{ NodeIdx{ 2 }, 3, 1, 2 } };
    arena.getNodes()[3] = typename Arena::NodeType{ TestNode{ NodeIdx{ 3 }, 0, 0, 3 } };

    arena.getChildren()[0] = NodeIdx{ 1 }; // 0 -> 1
    arena.getChildren()[1] = NodeIdx{ 2 }; // 0 -> 2
    arena.getChildren()[2] = NodeIdx{ 3 }; // 1 -> 3
    arena.getChildren()[3] = NodeIdx{ 3 }; // 2 -> 3
}

// ============================================================================
// Basic Component Tests
// ============================================================================

TEST_CASE( "NodeIdx operations", "[NodeIdx]" )
{
    SECTION( "Construction and comparison" )
    {
        NodeIdx default_idx;
        NodeIdx explicit_idx{ 42 };
        NodeIdx copy_idx{ 42 };

        REQUIRE( default_idx.value == 0 );
        REQUIRE( explicit_idx.value == 42 );
        REQUIRE( explicit_idx == copy_idx );
        REQUIRE( default_idx < explicit_idx );
        REQUIRE_FALSE( explicit_idx < copy_idx );
    }
}

TEST_CASE( "Node base functionality", "[Node]" )
{
    SECTION( "Construction and properties" )
    {
        Node leaf{ NodeIdx{ 1 }, 0, 0 };
        Node binary{ NodeIdx{ 2 }, 5, 2 };
        Node ternary{ NodeIdx{ 3 }, 10, 3 };

        REQUIRE( leaf.isLeaf() );
        REQUIRE( binary.isBinary() );
        REQUIRE_FALSE( ternary.isBinary() );
        REQUIRE_FALSE( leaf.isBinary() );
        REQUIRE_FALSE( binary.isLeaf() );
    }

    SECTION( "Const correctness with deducing this" )
    {
        const Node const_leaf{ NodeIdx{ 1 }, 0, 0 };
        const Node const_binary{ NodeIdx{ 2 }, 5, 2 };

        REQUIRE( const_leaf.isLeaf() );
        REQUIRE( const_binary.isBinary() );

        // Test with rvalues
        REQUIRE( Node{ NodeIdx{ 3 }, 0, 0 }.isLeaf() );
        REQUIRE( Node{ NodeIdx{ 4 }, 5, 2 }.isBinary() );
    }
}

// ============================================================================
// NodeArena Core Functionality
// ============================================================================

TEST_CASE( "NodeArena construction and properties", "[NodeArena][core]" )
{
    TestArena arena;

    SECTION( "Template parameters and sizes" )
    {
        REQUIRE( TestArena::MAX_NODES == 10 );
        REQUIRE( TestArena::MAX_CHILDREN == 20 );
        static_assert( std::is_same_v< TestArena::NodeType, TestNodeVariant > );
        static_assert( std::is_same_v< TestArena::DataType, float > );

        REQUIRE( arena.getNodes().size() == 10 );
        REQUIRE( arena.getChildren().size() == 20 );
        REQUIRE( arena.getWeights().size() == 200 ); // MAX_CHILDREN * MAX_NODES
    }

    SECTION( "Accessor methods with deducing this" )
    {
        // Mutable accessors
        auto nodes = arena.getNodes();
        auto children = arena.getChildren();
        auto weights = arena.getWeights();

        static_assert( std::is_same_v< decltype( nodes ), std::span< TestNodeVariant > > );
        static_assert( std::is_same_v< decltype( children ), std::span< NodeIdx > > );
        static_assert( std::is_same_v< decltype( weights ), std::span< float > > );

        // Const accessors
        const auto& const_arena = arena;
        auto const_nodes = const_arena.getNodes();
        auto const_children = const_arena.getChildren();
        auto const_weights = const_arena.getWeights();

        static_assert( std::is_same_v< decltype( const_nodes ), std::span< const TestNodeVariant > > );
        static_assert( std::is_same_v< decltype( const_children ), std::span< const NodeIdx > > );
        static_assert( std::is_same_v< decltype( const_weights ), std::span< const float > > );
    }
}

TEST_CASE( "NodeArena getChildrenOf functionality", "[NodeArena][children]" )
{
    TestArena arena;

    SECTION( "Leaf node children" )
    {
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 0, 42 };

        auto children = arena.getChildrenOf( arena.getNodes()[0] );
        REQUIRE( children.empty() );

        // Test const version
        const auto& const_arena = arena;
        auto const_children = const_arena.getChildrenOf( arena.getNodes()[0] );
        REQUIRE( const_children.empty() );
    }

    SECTION( "Node with children" )
    {
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 5, 3, 100 };
        arena.getChildren()[5] = NodeIdx{ 2 };
        arena.getChildren()[6] = NodeIdx{ 3 };
        arena.getChildren()[7] = NodeIdx{ 4 };

        auto children = arena.getChildrenOf( arena.getNodes()[1] );
        REQUIRE( children.size() == 3 );
        REQUIRE( children[0].value == 2 );
        REQUIRE( children[1].value == 3 );
        REQUIRE( children[2].value == 4 );
    }

    SECTION( "Node with many children" )
    {
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 5, 0 };
        for( usize i = 0; i < 5; ++i ) {
            arena.getChildren()[i] = NodeIdx{ i + 1 };
        }

        auto children = arena.getChildrenOf( arena.getNodes()[0] );
        REQUIRE( children.size() == 5 );
        for( usize i = 0; i < 5; ++i ) {
            REQUIRE( children[i].value == i + 1 );
        }
    }
}

// ============================================================================
// Parent Relationship Tests
// ============================================================================

TEST_CASE( "NodeArena getParentsOf functionality", "[NodeArena][parents]" )
{
    TestArena arena;

    SECTION( "Root node (no parents)" )
    {
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 0, 0, 1 };
        arena.getChildren()[0] = NodeIdx{ 1 };

        auto parents = arena.getParentsOf( arena.getNodes()[0] );
        REQUIRE( parents.empty() );
    }

    SECTION( "Single parent relationship" )
    {
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 0, 0, 1 };
        arena.getChildren()[0] = NodeIdx{ 1 };

        auto parents = arena.getParentsOf( arena.getNodes()[1] );
        REQUIRE( parents.size() == 1 );
        REQUIRE( parents[0].value == 0 );
    }

    SECTION( "Multiple parents (DAG structure)" )
    {
        // Setup: 0 -> 2, 1 -> 2 (node 2 has two parents)
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 1, 1, 1 };
        arena.getNodes()[2] = TestNode{ NodeIdx{ 2 }, 0, 0, 2 };

        arena.getChildren()[0] = NodeIdx{ 2 };
        arena.getChildren()[1] = NodeIdx{ 2 };

        auto parents = arena.getParentsOf( arena.getNodes()[2] );
        REQUIRE( parents.size() == 2 );

        std::sort( parents.begin(), parents.end(), []( NodeIdx a, NodeIdx b ) { return a.value < b.value; } );
        REQUIRE( parents[0].value == 0 );
        REQUIRE( parents[1].value == 1 );
    }

    SECTION( "Complex parent relationships in binary tree" )
    {
        setupBinaryTree( arena );

        // Node 3 should have parent 1
        auto parents_3 = arena.getParentsOf( arena.getNodes()[3] );
        REQUIRE( parents_3.size() == 1 );
        REQUIRE( parents_3[0].value == 1 );

        // Node 0 should have no parents
        auto parents_0 = arena.getParentsOf( arena.getNodes()[0] );
        REQUIRE( parents_0.empty() );

        // Node 1 should have parent 0
        auto parents_1 = arena.getParentsOf( arena.getNodes()[1] );
        REQUIRE( parents_1.size() == 1 );
        REQUIRE( parents_1[0].value == 0 );
    }
}

// ============================================================================
// Graph Traversal Tests
// ============================================================================

TEST_CASE( "NodeArena traversal algorithms", "[NodeArena][traversal]" )
{
    TestArena arena;

    SECTION( "DFS traversal order" )
    {
        setupBinaryTree( arena );

        std::vector< int > visited_data;
        arena.dfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            visited_data.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );

        REQUIRE( visited_data.size() == 7 );
        REQUIRE( visited_data[0] == 0 ); // Root always first

        // Verify all nodes visited exactly once (iterative DFS has different order)
        std::set< int > visited_set( visited_data.begin(), visited_data.end() );
        REQUIRE( visited_set == std::set< int >{ 0, 1, 2, 3, 4, 5, 6 } );
    }

    SECTION( "BFS traversal order" )
    {
        setupBinaryTree( arena );

        std::vector< int > visited_data;
        arena.bfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            visited_data.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );

        REQUIRE( visited_data.size() == 7 );
        // BFS explores level by level: 0 -> {1, 2} -> {3, 4, 5, 6}
        REQUIRE( visited_data == std::vector< int >{ 0, 1, 2, 3, 4, 5, 6 } );
    }

    SECTION( "Single node traversal" )
    {
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 0, 42 };

        int dfs_count = 0, bfs_count = 0;
        arena.dfs( NodeIdx{ 0 }, [&]( const auto& ) { ++dfs_count; } );
        arena.bfs( NodeIdx{ 0 }, [&]( const auto& ) { ++bfs_count; } );

        REQUIRE( dfs_count == 1 );
        REQUIRE( bfs_count == 1 );
    }

    SECTION( "Linear chain traversal" )
    {
        setupLinearChain( arena, 5 );

        std::vector< int > dfs_order, bfs_order;
        arena.dfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            dfs_order.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );
        arena.bfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            bfs_order.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );

        // For linear chains, DFS and BFS should be identical
        REQUIRE( dfs_order == bfs_order );
        REQUIRE( dfs_order == std::vector< int >{ 0, 10, 20, 30, 40 } );
    }

    SECTION( "Const arena traversal" )
    {
        setupBinaryTree( arena );
        const auto& const_arena = arena;

        std::vector< int > visited;
        const_arena.dfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            visited.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );

        REQUIRE( visited.size() == 7 );
        REQUIRE( visited[0] == 0 );
    }

    SECTION( "DFS behavior on DAG structures" )
    {
        setupDiamondGraph( arena );

        std::vector< usize > visited_nodes;
        arena.dfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            auto idx = std::visit( []( const auto& n ) { return n.idx; }, node );
            visited_nodes.push_back( idx.value );
        } );

        // With DAG-aware DFS, each node is visited exactly once
        REQUIRE( visited_nodes.size() == 4 );
        std::set< usize > visited_set( visited_nodes.begin(), visited_nodes.end() );
        REQUIRE( visited_set == std::set< usize >{ 0, 1, 2, 3 } );

        // BFS should also visit each node exactly once
        std::vector< usize > bfs_nodes;
        arena.bfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            auto idx = std::visit( []( const auto& n ) { return n.idx; }, node );
            bfs_nodes.push_back( idx.value );
        } );

        REQUIRE( bfs_nodes.size() == 4 );
        std::set< usize > bfs_set( bfs_nodes.begin(), bfs_nodes.end() );
        REQUIRE( bfs_set == std::set< usize >{ 0, 1, 2, 3 } );
    }
    {
        // Setup two disconnected components
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 0, 0, 1 };
        arena.getNodes()[2] = TestNode{ NodeIdx{ 2 }, 1, 1, 2 };
        arena.getNodes()[3] = TestNode{ NodeIdx{ 3 }, 0, 0, 3 };

        arena.getChildren()[0] = NodeIdx{ 1 }; // 0 -> 1
        arena.getChildren()[1] = NodeIdx{ 3 }; // 2 -> 3

        // Traverse from component 1
        std::vector< int > component1;
        arena.dfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            component1.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );

        // Traverse from component 2
        std::vector< int > component2;
        arena.dfs( NodeIdx{ 2 }, [&]( const auto& node ) {
            component2.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );

        REQUIRE( component1 == std::vector< int >{ 0, 1 } );
        REQUIRE( component2 == std::vector< int >{ 2, 3 } );
    }
}

// ============================================================================
// Max Flow Algorithm Tests
// ============================================================================

TEST_CASE( "NodeArena max flow algorithm", "[NodeArena][maxflow]" )
{
    TestArena arena;

    SECTION( "Simple two-node flow" )
    {
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 0, 0, 1 };
        arena.getChildren()[0] = NodeIdx{ 1 };
        arena.getWeights()[0] = 10.0f;

        struct EdgeFlow
        {
            NodeIdx from, to;
            float flow;
        };
        std::vector< EdgeFlow > flows;

        float max_flow = arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 1 },
            [&]( NodeIdx from, NodeIdx to, usize, float flow ) { flows.push_back( { from, to, flow } ); } );

        REQUIRE( max_flow == Catch::Approx( 10.0f ) );
        REQUIRE( flows.size() >= 1 );

        auto flow_01 = std::find_if( flows.begin(), flows.end(),
            []( const EdgeFlow& ef ) { return ef.from.value == 0 && ef.to.value == 1; } );
        REQUIRE( flow_01 != flows.end() );
        REQUIRE( flow_01->flow == Catch::Approx( 10.0f ) );
    }

    SECTION( "Linear bottleneck network" )
    {
        // 0 -> 1 -> 2 with capacities 5, 3 (bottleneck at 3)
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 1, 1, 1 };
        arena.getNodes()[2] = TestNode{ NodeIdx{ 2 }, 0, 0, 2 };

        arena.getChildren()[0] = NodeIdx{ 1 };
        arena.getChildren()[1] = NodeIdx{ 2 };
        arena.getWeights()[0] = 5.0f;
        arena.getWeights()[1] = 3.0f; // Bottleneck

        float max_flow =
            arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 2 }, []( NodeIdx, NodeIdx, usize, float ) {} );

        REQUIRE( max_flow == Catch::Approx( 3.0f ) );
    }

    SECTION( "Diamond network" )
    {
        setupDiamondGraph( arena );

        arena.getWeights()[0] = 10.0f; // 0->1
        arena.getWeights()[1] = 10.0f; // 0->2
        arena.getWeights()[2] = 15.0f; // 1->3
        arena.getWeights()[3] = 6.0f;  // 2->3

        struct EdgeFlow
        {
            NodeIdx from, to;
            float flow;
        };
        std::vector< EdgeFlow > flows;

        float max_flow = arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 3 },
            [&]( NodeIdx from, NodeIdx to, usize, float flow ) { flows.push_back( { from, to, flow } ); } );

        // Path 0->1->3 can carry min(10,15)=10
        // Path 0->2->3 can carry min(10,6)=6, total = 16
        REQUIRE( max_flow == Catch::Approx( 16.0f ) );

        // Verify flow conservation
        auto get_flow = [&]( usize from, usize to ) -> float {
            auto it = std::find_if( flows.begin(), flows.end(),
                [=]( const EdgeFlow& ef ) { return ef.from.value == from && ef.to.value == to; } );
            return it != flows.end() ? it->flow : 0.0f;
        };

        float flow_01 = get_flow( 0, 1 );
        float flow_02 = get_flow( 0, 2 );
        float flow_13 = get_flow( 1, 3 );
        float flow_23 = get_flow( 2, 3 );

        REQUIRE( flow_01 == Catch::Approx( flow_13 ) );                     // Conservation at node 1
        REQUIRE( flow_02 == Catch::Approx( flow_23 ) );                     // Conservation at node 2
        REQUIRE( flow_01 + flow_02 == Catch::Approx( flow_13 + flow_23 ) ); // Total flow
    }

    SECTION( "Parallel paths" )
    {
        setupDiamondGraph( arena );

        arena.getWeights()[0] = 7.0f; // 0->1
        arena.getWeights()[1] = 3.0f; // 0->2
        arena.getWeights()[2] = 5.0f; // 1->3
        arena.getWeights()[3] = 8.0f; // 2->3

        float max_flow =
            arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 3 }, []( NodeIdx, NodeIdx, usize, float ) {} );

        // Path 0->1->3 gives min(7,5)=5, Path 0->2->3 gives min(3,8)=3
        // Total = 5 + 3 = 8
        REQUIRE( max_flow == Catch::Approx( 8.0f ) );
    }

    SECTION( "Zero capacity and degenerate cases" )
    {
        // Zero capacity edge
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 0, 0, 1 };
        arena.getChildren()[0] = NodeIdx{ 1 };
        arena.getWeights()[0] = 0.0f;

        float zero_flow =
            arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 1 }, []( NodeIdx, NodeIdx, usize, float ) {} );
        REQUIRE( zero_flow == Catch::Approx( 0.0f ) );

        // Source equals sink
        float self_flow =
            arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 0 }, []( NodeIdx, NodeIdx, usize, float ) {} );
        REQUIRE( self_flow == Catch::Approx( 0.0f ) );
    }

    SECTION( "Multiple bottlenecks" )
    {
        // 0 -> 1 -> 2 -> 3 with varying capacities
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 1, 1, 1 };
        arena.getNodes()[2] = TestNode{ NodeIdx{ 2 }, 2, 1, 2 };
        arena.getNodes()[3] = TestNode{ NodeIdx{ 3 }, 0, 0, 3 };

        arena.getChildren()[0] = NodeIdx{ 1 };
        arena.getChildren()[1] = NodeIdx{ 2 };
        arena.getChildren()[2] = NodeIdx{ 3 };

        arena.getWeights()[0] = 100.0f; // 0->1
        arena.getWeights()[1] = 2.0f;   // 1->2 (bottleneck)
        arena.getWeights()[2] = 50.0f;  // 2->3

        float max_flow =
            arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 3 }, []( NodeIdx, NodeIdx, usize, float ) {} );

        REQUIRE( max_flow == Catch::Approx( 2.0f ) ); // Limited by tightest bottleneck
    }

    SECTION( "Const arena max flow" )
    {
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 1, 0 };
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 0, 0, 1 };
        arena.getChildren()[0] = NodeIdx{ 1 };
        arena.getWeights()[0] = 5.0f;

        const auto& const_arena = arena;
        float max_flow =
            const_arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 1 }, []( NodeIdx, NodeIdx, usize, float ) {} );

        REQUIRE( max_flow == Catch::Approx( 5.0f ) );
    }
}

// ============================================================================
// Variant Node Type Tests
// ============================================================================

TEST_CASE( "NodeArena with variant node types", "[NodeArena][variants]" )
{
    ElectronicArena arena;

    SECTION( "Mixed component types" )
    {
        arena.nodes[0] = ResistorNode( { 0 }, 0, 2, 220.0 );
        arena.nodes[1] = CapacitorNode( { 1 }, 0, 0, 0.01 );
        arena.nodes[2] = Node{ .idx = { 2 }, .child_offset = 0, .child_count = 0 };

        arena.children[0] = NodeIdx{ 1 };
        arena.children[1] = NodeIdx{ 2 };

        // Test property access
        const auto& resistor = std::get< ResistorNode >( arena.nodes[0] );
        const auto& capacitor = std::get< CapacitorNode >( arena.nodes[1] );

        REQUIRE( resistor.getResistance() == 220.0 );
        REQUIRE( capacitor.getCapacitance() == Catch::Approx( 0.01 ) );
        REQUIRE_FALSE( resistor.isLeaf() );
        REQUIRE( capacitor.isLeaf() );
    }

    SECTION( "Traversal with mixed types" )
    {
        arena.nodes[0] = ResistorNode( { 0 }, 0, 1, 100.0 );
        arena.nodes[1] = CapacitorNode( { 1 }, 0, 0, 0.1 );
        arena.children[0] = NodeIdx{ 1 };

        double total_resistance = 0.0;
        double total_capacitance = 0.0;
        int node_count = 0;

        arena.dfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            std::visit( overload{ [&]( const ResistorNode& r ) { total_resistance += r.getResistance(); },
                            [&]( const CapacitorNode& c ) { total_capacitance += c.getCapacitance(); },
                            [&]( const Node& ) { ++node_count; } },
                node );
        } );

        REQUIRE( total_resistance == 100.0 );
        REQUIRE( total_capacitance == Catch::Approx( 0.1 ) );
        REQUIRE( node_count == 0 );
    }

    SECTION( "Complex electronic circuit" )
    {
        // Build a small circuit: R1 -> {C1, R2} -> C2
        arena.nodes[0] = ResistorNode( { 0 }, 0, 2, 1000.0 ); // R1
        arena.nodes[1] = CapacitorNode( { 1 }, 0, 0, 0.001 ); // C1
        arena.nodes[2] = ResistorNode( { 2 }, 2, 1, 2200.0 ); // R2
        arena.nodes[3] = CapacitorNode( { 3 }, 0, 0, 0.002 ); // C2

        arena.children[0] = NodeIdx{ 1 }; // R1 -> C1
        arena.children[1] = NodeIdx{ 2 }; // R1 -> R2
        arena.children[2] = NodeIdx{ 3 }; // R2 -> C2

        // Verify structure
        auto r1_children = arena.getChildrenOf( arena.nodes[0] );
        REQUIRE( r1_children.size() == 2 );

        auto c1_parents = arena.getParentsOf( arena.nodes[1] );
        auto c2_parents = arena.getParentsOf( arena.nodes[3] );
        REQUIRE( c1_parents.size() == 1 );
        REQUIRE( c2_parents.size() == 1 );
        REQUIRE( c1_parents[0].value == 0 );
        REQUIRE( c2_parents[0].value == 2 );
    }
}

// ============================================================================
// Template Parameter Variations
// ============================================================================

TEST_CASE( "NodeArena template parameter variations", "[NodeArena][templates]" )
{
    SECTION( "Different node and edge counts" )
    {
        NodeArena< TestNodeVariant, 5, 10, int > small_arena;
        NodeArena< TestNodeVariant, 3, 6, double > tiny_arena;

        REQUIRE( small_arena.getNodes().size() == 5 );
        REQUIRE( small_arena.getChildren().size() == 10 );
        REQUIRE( small_arena.getWeights().size() == 50 );

        REQUIRE( tiny_arena.getNodes().size() == 3 );
        REQUIRE( tiny_arena.getChildren().size() == 6 );
        REQUIRE( tiny_arena.getWeights().size() == 18 );

        static_assert( std::is_same_v< decltype( small_arena )::DataType, int > );
        static_assert( std::is_same_v< decltype( tiny_arena )::DataType, double > );
    }

    SECTION( "Different weight types" )
    {
        NodeArena< TestNodeVariant, 4, 8, double > double_arena;

        // Test weight assignment and retrieval
        double_arena.getWeights()[0] = 3.14159;
        double_arena.getWeights()[1] = 2.71828;

        REQUIRE( double_arena.getWeights()[0] == Catch::Approx( 3.14159 ) );
        REQUIRE( double_arena.getWeights()[1] == Catch::Approx( 2.71828 ) );
    }

    SECTION( "Single variant type" )
    {
        SimpleArena simple_arena;

        simple_arena.nodes[0] = Node{ { 0 }, 0, 1 };
        simple_arena.nodes[1] = Node{ { 1 }, 0, 0 };
        simple_arena.children[0] = NodeIdx{ 1 };

        auto children = simple_arena.getChildrenOf( simple_arena.nodes[0] );
        REQUIRE( children.size() == 1 );
        REQUIRE( children[0].value == 1 );
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE( "NodeArena integration scenarios", "[NodeArena][integration]" )
{
    TestArena arena;

    SECTION( "Complete workflow: build, traverse, analyze" )
    {
        setupBinaryTree( arena );

        // Set up weights for flow analysis
        arena.getWeights()[0] = 4.0f; // 0->1
        arena.getWeights()[1] = 6.0f; // 0->2
        arena.getWeights()[2] = 3.0f; // 1->3
        arena.getWeights()[3] = 5.0f; // 1->4

        // Traverse and collect structure info
        std::vector< usize > node_ids;
        std::vector< usize > child_counts;

        arena.dfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            auto idx = std::visit( []( const auto& n ) { return n.idx; }, node );
            auto children = arena.getChildrenOf( node );
            node_ids.push_back( idx.value );
            child_counts.push_back( children.size() );
        } );

        // DFS should visit each node exactly once in tree
        REQUIRE( node_ids.size() == 7 );
        // Verify all nodes visited exactly once (order may vary with iterative DFS)
        REQUIRE( std::set< usize >( node_ids.begin(), node_ids.end() ) ==
            std::set< usize >{ 0, 1, 2, 3, 4, 5, 6 } );

        // Verify structure properties regardless of traversal order
        auto root_pos = std::find( node_ids.begin(), node_ids.end(), 0 );
        auto node1_pos = std::find( node_ids.begin(), node_ids.end(), 1 );
        auto node2_pos = std::find( node_ids.begin(), node_ids.end(), 2 );
        REQUIRE( root_pos != node_ids.end() );
        REQUIRE( child_counts[root_pos - node_ids.begin()] == 2 );  // Root has 2 children
        REQUIRE( child_counts[node1_pos - node_ids.begin()] == 2 ); // Node 1 has 2 children
        REQUIRE( child_counts[node2_pos - node_ids.begin()] == 2 ); // Node 2 has 2 children

        // Test max flow on tree structure: 0->1->3
        float flow = arena.findMaxFlow( NodeIdx{ 0 }, NodeIdx{ 3 }, []( NodeIdx, NodeIdx, usize, float ) {} );

        // Path 0->1->3: min(4,3)=3
        REQUIRE( flow == Catch::Approx( 3.0f ) );
    }

    SECTION( "Multi-level hierarchy" )
    {
        // Create a 3-level tree
        arena.getNodes()[0] = TestNode{ NodeIdx{ 0 }, 0, 2, 0 }; // Root
        arena.getNodes()[1] = TestNode{ NodeIdx{ 1 }, 2, 2, 1 }; // Level 1
        arena.getNodes()[2] = TestNode{ NodeIdx{ 2 }, 4, 2, 2 }; // Level 1
        arena.getNodes()[3] = TestNode{ NodeIdx{ 3 }, 0, 0, 3 }; // Level 2
        arena.getNodes()[4] = TestNode{ NodeIdx{ 4 }, 0, 0, 4 }; // Level 2
        arena.getNodes()[5] = TestNode{ NodeIdx{ 5 }, 0, 0, 5 }; // Level 2
        arena.getNodes()[6] = TestNode{ NodeIdx{ 6 }, 0, 0, 6 }; // Level 2

        arena.getChildren()[0] = NodeIdx{ 1 };
        arena.getChildren()[1] = NodeIdx{ 2 };
        arena.getChildren()[2] = NodeIdx{ 3 };
        arena.getChildren()[3] = NodeIdx{ 4 };
        arena.getChildren()[4] = NodeIdx{ 5 };
        arena.getChildren()[5] = NodeIdx{ 6 };

        // Test level-order traversal
        std::vector< int > bfs_data;
        arena.bfs( NodeIdx{ 0 }, [&]( const auto& node ) {
            bfs_data.push_back( std::visit( []( const auto& n ) { return n.getData(); }, node ) );
        } );

        REQUIRE( bfs_data == std::vector< int >{ 0, 1, 2, 3, 4, 5, 6 } );

        // Verify parent relationships at each level
        auto parents_3 = arena.getParentsOf( arena.getNodes()[3] );
        auto parents_5 = arena.getParentsOf( arena.getNodes()[5] );

        REQUIRE( parents_3.size() == 1 );
        REQUIRE( parents_3[0].value == 1 );
        REQUIRE( parents_5.size() == 1 );
        REQUIRE( parents_5[0].value == 2 );
    }
}
