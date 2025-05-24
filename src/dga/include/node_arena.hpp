#pragma once

#include <array>
#include <queue>
#include <span>

#include <traits.hpp>

namespace dga {

struct NodeIdx
{
    std::size_t value = 0;
};

struct Node
{
    NodeIdx idx = { 0 };
    std::size_t child_offset = 0;
    std::size_t child_count = 0;

    constexpr auto isLeaf( this auto&& self )
    {
        return self.child_count == 0;
    }
};

using common::Variant;

template< Variant T, std::size_t N, std::size_t M >
struct NodeArena
{
    using NodeType = T;
    static constexpr std::size_t MaxNodes = N;
    static constexpr std::size_t MaxChildren = M;

    std::array< NodeType, MaxNodes > nodes;
    std::array< NodeIdx, MaxChildren > children;

    constexpr auto getNodes() -> std::span< NodeType >
    {
        return nodes;
    }

    constexpr auto getNodes() const -> std::span< const NodeType >
    {
        return nodes;
    }

    constexpr auto getChildren() -> std::span< NodeIdx >
    {
        return children;
    }

    constexpr auto getChildren() const -> std::span< const NodeIdx >
    {
        return children;
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

    constexpr auto check_invariants() const -> bool
    {
        for( auto const& node : nodes ) {
            auto const& base = std::visit( []( auto const& n ) -> Node const& { return n; }, node );

            if( base.idx.value >= MaxNodes ) {
                return false;
            }

            if( base.child_offset + base.child_count > MaxChildren ) {
                return false;
            }

            for( auto child_idx : getChildrenOf( node ) ) {
                if( child_idx.value >= MaxNodes ) {
                    return false;
                }
            }
        }

        return true;
    }

    template< typename Func >
    constexpr auto dfs( NodeIdx root, Func&& visit ) -> void
    {
        auto& node = nodes[root.value];
        visit( node );

        for( auto child : getChildrenOf( node ) ) {
            dfs( child, std::forward< Func >( visit ) );
        }
    }

    template< typename Func >
    constexpr auto dfs( NodeIdx root, Func&& visit ) const -> void
    {
        const auto& node = nodes[root.value];
        visit( node );

        for( auto child : getChildrenOf( node ) ) {
            dfs( child, std::forward< Func >( visit ) );
        }
    }

    template< typename Func >
    constexpr auto bfs( NodeIdx root, Func&& visit ) -> void
    {
        std::array< NodeIdx, MaxNodes > queue = {};
        std::size_t head = 0;
        std::size_t tail = 0;
        queue[tail++] = root;

        for( ; head < tail; ) {
            auto idx = queue[head++];
            auto& node = nodes[idx.value];
            visit( node );

            for( auto child : getChildrenOf( node ) ) {
                queue[tail++] = child;
            }
        }
    }

    template< typename Func >
    constexpr void bfs( NodeIdx root, Func&& visit ) const
    {
        std::array< NodeIdx, MaxNodes > queue = {};
        std::size_t head = 0;
        std::size_t tail = 0;
        queue[tail++] = root;

        for( ; head < tail; ) {
            auto idx = queue[head++];
            auto const& node = nodes[idx.value];
            visit( node );

            for( auto child : getChildrenOf( node ) ) {
                queue[tail++] = child;
            }
        }
    }
};

} // namespace dga
