include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(catch2)

add_executable(test_dag test_nodearena.cpp)
target_link_libraries(test_dag PRIVATE Catch2::Catch2WithMain dag)

include(CTest)
include(Catch)
catch_discover_tests(test_dag)
