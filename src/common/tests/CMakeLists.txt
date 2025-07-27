include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2)
FetchContent_MakeAvailable(catch2)

include(Catch)

file(GLOB TEST_SOURCES "test_*.cpp")

foreach(TEST_SOURCE ${TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})
  target_compile_features(${TEST_NAME} PRIVATE cxx_std_23)
  target_link_libraries(${TEST_NAME} PRIVATE Catch2::Catch2WithMain common)
  catch_discover_tests(${TEST_NAME})
endforeach()

add_executable(common_compile_test compile_test.cpp)
target_compile_options(common_compile_test PRIVATE -ffreestanding -fno-rtti
                                                   -fno-exceptions)
target_compile_features(common_compile_test PRIVATE cxx_std_23)
target_include_directories(common_compile_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/include)
