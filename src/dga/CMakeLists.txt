add_library(dga STATIC include/node_arena.hpp)
target_link_libraries(dga PUBLIC common)
target_include_directories(dga PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(dga PROPERTIES LINKER_LANGUAGE CXX)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
