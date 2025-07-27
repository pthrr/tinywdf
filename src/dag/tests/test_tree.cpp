
// clang-format off
constexpr std::array<AnyNode, 4> nodes = {
    Node{ .idx = {0} },
    ResistorNode({1}, 0, 0, 220),
    CapacitorNode({2}, 0, 0, 0.01),
    Node{ .idx = {3} }
};
// clang-format on

// clang-format off
constexpr std::array<std::array<int, 3>, 4> incidence = {{
    {1, 0, 0},  // node 0 connects to edge 0
    {1, 1, 0},  // node 1 connects to edge 0 and 1
    {0, 1, 1},  // node 2 connects to edge 1 and 2
    {0, 0, 1}   // node 3 connects to edge 2
}};
// clang-format on

auto tree = build_tree_from_undirected_incidence_matrix< AnyNode, 4, 3, 3 >( nodes, incidence, /*root=*/0 );
