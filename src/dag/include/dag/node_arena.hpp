#pragma once

#include <array>
#include <queue>
#include <span>
#include <vector>

#include <common/result.hpp>
#include <common/traits.hpp>
#include <common/types.hpp>

namespace dag {
namespace node_arena {

using namespace cmn::types;
using namespace cmn::result;

struct NodeIdx
{
    usize value = 0;

    auto operator==( NodeIdx const& other ) const -> bool = default;
    auto operator<( NodeIdx const& other ) const -> bool
    {
        return value < other.value;
    }
};

struct Node
{
    NodeIdx idx = {};
    usize child_offset = 0;
    usize child_count = 0;

    constexpr auto isLeaf( this auto&& self ) -> bool
    {
        return self.child_count == 0;
    }

    constexpr auto isBinary( this auto&& self ) -> bool
    {
        return self.child_count == 2;
    }
};

using cmn::traits::Numerical;
using cmn::traits::Variant;

template< Variant T, usize N, usize M, Numerical W = float >
struct NodeArena
{
    using NodeType = T;
    using DataType = W;

    static constexpr usize MAX_NODES = N;
    static constexpr usize MAX_CHILDREN = M;

    std::array< NodeType, MAX_NODES > nodes;
    std::array< NodeIdx, MAX_CHILDREN > children;
    std::array< DataType, MAX_CHILDREN * MAX_NODES > weights;

    constexpr auto getNodes() -> std::span< NodeType >
    {
        return nodes;
    }

    constexpr auto getNodes() const -> std::span< NodeType const >
    {
        return nodes;
    }

    constexpr auto getChildren() -> std::span< NodeIdx >
    {
        return children;
    }

    constexpr auto getChildren() const -> std::span< NodeIdx const >
    {
        return children;
    }

    constexpr auto getWeights() -> std::span< DataType >
    {
        return weights;
    }

    constexpr auto getWeights() const -> std::span< DataType const >
    {
        return weights;
    }

    constexpr auto getChildrenOf( NodeType const& n ) -> std::span< NodeIdx >
    {
        return std::visit(
            [&]( auto const& node ) {
                return std::span{ children }.subspan( node.child_offset, node.child_count );
            },
            n );
    }

    constexpr auto getChildrenOf( NodeType const& n ) const -> std::span< NodeIdx const >
    {
        return std::visit(
            [&]( auto const& node ) {
                return std::span{ children }.subspan( node.child_offset, node.child_count );
            },
            n );
    }

    constexpr auto getParentsOf( NodeType const& n ) const -> std::vector< NodeIdx >
    {
        std::vector< NodeIdx > parents;
        auto idx_n = std::visit( []( auto const& node ) { return node.idx; }, n );

        for( auto node : nodes ) {
            auto children = getChildrenOf( node );

            for( auto child : children ) {
                if( child.value == idx_n.value ) {
                    auto idx_parent = std::visit( []( auto const& node ) { return node.idx; }, node );
                    parents.push_back( idx_parent );
                    break;
                }
            }
        }

        return parents;
    }

    constexpr auto checkInvariants() const -> bool
    {
        usize sum_nodes = 0;

        for( auto const& node : nodes ) {
            auto const& base = std::visit( []( auto const& n ) -> Node const& { return n; }, node );
            sum_nodes += base.idx.value;

            if( base.idx.value >= MAX_NODES ) {
                return false;
            }

            if( base.child_offset + base.child_count > MAX_CHILDREN ) {
                return false;
            }

            for( auto child : getChildrenOf( node ) ) {
                if( child.value >= MAX_NODES ) {
                    return false;
                }
            }
        }

        // nodes were correctly initialized
        usize sum_max = 0;

        for( usize i = 0; i < MAX_NODES; ++i ) {
            sum_max += i;
        }

        if( sum_nodes != sum_max ) {
            return false;
        }

        return true;
    }

    template< typename Func >
    constexpr auto bfs( NodeIdx root, Func&& visit ) const -> void
    {
        std::array< NodeIdx, MAX_NODES > queue = {};
        std::array< bool, MAX_NODES > visited = {};
        usize head = 0;
        usize tail = 0;
        queue[tail++] = root;
        visited[root.value] = true;

        for( ; head < tail; ) {
            auto idx = queue[head++];
            auto const& node = nodes[idx.value];
            visit( node );

            for( auto child : getChildrenOf( node ) ) {
                if( not visited[child.value] ) {
                    visited[child.value] = true;
                    queue[tail++] = child;
                }
            }
        }
    }

    template< typename Func >
    constexpr auto dfs( NodeIdx root, Func&& visit ) const -> void
    {
        std::array< NodeIdx, MAX_NODES > stack = {};
        std::array< bool, MAX_NODES > visited = {};
        usize top = 0;
        stack[top++] = root;

        while( top > 0 ) {
            auto idx = stack[--top];

            if( visited[idx.value] ) {
                continue;
            }

            const auto& node = nodes[idx.value];
            visit( node );
            visited[idx.value] = true;

            for( auto child : getChildrenOf( node ) ) {
                if( not visited[child.value] ) {
                    stack[top++] = child;
                }
            }
        }
    }

    template< typename Func >
    constexpr auto findMaxFlow( NodeIdx source, NodeIdx sink, Func&& visit ) const -> DataType
    {
        std::array< DataType, MAX_CHILDREN * MAX_NODES > flows = {};
        std::array< DataType, MAX_NODES > excess = {};
        std::array< usize, MAX_NODES > heights = {};

        heights[source.value] = MAX_NODES;

        auto const& source_node = nodes[source.value];
        auto source_children = getChildrenOf( source_node );
        usize child_offset = std::visit( []( auto const& node ) { return node.child_offset; }, source_node );

        for( usize i = 0; i < source_children.size(); ++i ) {
            usize i_edge = child_offset + i;
            auto idx_child = source_children[i];
            auto cap = weights[i_edge];
            flows[i_edge] = cap;
            excess[idx_child.value] += cap;
            excess[source.value] -= cap;
        }

        auto isActiveNode = [&]( NodeIdx idx ) -> bool {
            return idx.value != source.value && idx.value != sink.value && excess[idx.value] > DataType{};
        };

        auto findActiveNode = [&]() -> Result< NodeIdx > {
            for( usize i = 0; i < MAX_NODES; ++i ) {
                NodeIdx idx{ i };

                if( isActiveNode( idx ) ) {
                    return ok( idx );
                }
            }

            return err();
        };

        auto getResidualCapacity = [&]( usize i_edge, bool forward ) -> DataType {
            if( forward ) {
                return weights[i_edge] - flows[i_edge];
            }

            return flows[i_edge];
        };

        auto canPush = [&]( NodeIdx from, NodeIdx to, usize i_edge, bool forward ) -> bool {
            return excess[from.value] > DataType{} && heights[from.value] > heights[to.value] &&
                getResidualCapacity( i_edge, forward ) > DataType{};
        };

        auto pushFlow = [&]( NodeIdx from, NodeIdx to, usize i_edge, bool forward ) -> Status {
            if( not canPush( from, to, i_edge, forward ) ) {
                return err();
            }

            auto residual = getResidualCapacity( i_edge, forward );
            auto delta = std::min( excess[from.value], residual );

            if( forward ) {
                flows[i_edge] += delta;
            }
            else {
                flows[i_edge] -= delta;
            }

            excess[from.value] -= delta;
            excess[to.value] += delta;
            return ok();
        };

        auto relabelNode = [&]( NodeIdx idx ) -> void {
            usize min_height = MAX_NODES + 1;

            auto const& node = nodes[idx.value];
            auto children = getChildrenOf( node );
            usize child_offset = std::visit( []( auto const& n ) { return n.child_offset; }, node );

            // forward edges to children
            for( usize i = 0; i < children.size(); ++i ) {
                auto to = children[i];
                usize i_edge = child_offset + i;

                if( getResidualCapacity( i_edge, true ) > DataType{} ) {
                    min_height = std::min( min_height, heights[to.value] );
                }
            }

            // reverse edges to parents
            auto parents = getParentsOf( node );

            for( auto parent : parents ) {
                auto const& parent_node = nodes[parent.value];
                auto parent_children = getChildrenOf( parent_node );
                usize parent_child_offset =
                    std::visit( []( auto const& n ) { return n.child_offset; }, parent_node );

                for( usize i = 0; i < parent_children.size(); ++i ) {
                    if( parent_children[i].value == idx.value ) {
                        usize i_edge = parent_child_offset + i;

                        if( getResidualCapacity( i_edge, false ) > DataType{} ) {
                            min_height = std::min( min_height, heights[parent.value] );
                        }

                        break;
                    }
                }
            }

            heights[idx.value] = min_height + 1;
        };

        auto findPushableEdge = [&]( NodeIdx from ) -> Result< std::tuple< NodeIdx, usize, bool > > {
            auto const& node = nodes[from.value];
            auto children = getChildrenOf( node );
            usize child_offset = std::visit( []( auto const& n ) { return n.child_offset; }, node );

            // push forward
            for( usize i = 0; i < children.size(); ++i ) {
                auto to = children[i];
                usize i_edge = child_offset + i;

                if( canPush( from, to, i_edge, true ) ) {
                    return ok( std::make_tuple( to, i_edge, true ) );
                }
            }

            // push backward
            auto parents = getParentsOf( node );

            for( auto parent : parents ) {
                auto const& parent_node = nodes[parent.value];
                auto parent_children = getChildrenOf( parent_node );
                usize parent_child_offset =
                    std::visit( []( auto const& n ) { return n.child_offset; }, parent_node );

                for( usize i = 0; i < parent_children.size(); ++i ) {
                    if( parent_children[i].value == from.value ) {
                        usize i_edge = parent_child_offset + i;

                        if( canPush( from, parent, i_edge, false ) ) {
                            return ok( std::make_tuple( parent, i_edge, false ) );
                        }

                        break;
                    }
                }
            }

            return err();
        };

        while( auto active = findActiveNode() ) {
            auto idx_active = active.value();

            if( auto edge = findPushableEdge( idx_active ) ) {
                auto [target, i_edge, forward] = edge.value();
                pushFlow( idx_active, target, i_edge, forward );
            }
            else {
                relabelNode( idx_active );
            }
        }

        for( usize i_node = 0; i_node < MAX_NODES; ++i_node ) {
            auto const& node = nodes[i_node];
            auto children = getChildrenOf( node );
            usize child_offset = std::visit( []( auto const& n ) { return n.child_offset; }, node );

            for( usize i = 0; i < children.size(); ++i ) {
                usize i_edge = child_offset + i;
                NodeIdx from{ i_node };
                NodeIdx to = children[i];
                DataType flow = flows[i_edge];

                visit( from, to, i_edge, flow );
            }
        }

        return excess[sink.value];
    }
};

} // namespace node_arena
} // namespace dag
