#include "node_arena.hpp"

namespace dag {
namespace tree {

using common::Variant;

template< Variant T, std::size_t N, std::size_t M >
using Tree = node_arena::NodeArena< T, N, M >;

using BinaryTree;

constexpr auto makeRootedTree( nodes, edges ) -> RootedTree
{
    std::array weights = ();
}

constexpr auto makeRootedTree( nodes, edges, weights ) -> RootedTree
{
    auto tree = RootedTree<>();

    if( not auto res = tree.checkInvariants() ) {
        return err( res );
    }

    return ok();
}

} // namespace tree
} // namespace dag
