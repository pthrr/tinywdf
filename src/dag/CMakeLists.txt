add_library(dag STATIC include/dag/node_arena.hpp)
target_link_libraries(dag PUBLIC common)
target_include_directories(dag PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(dag PROPERTIES LINKER_LANGUAGE CXX)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
