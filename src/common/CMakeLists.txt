add_library(common STATIC include/traits.hpp)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(common PROPERTIES LINKER_LANGUAGE CXX)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
