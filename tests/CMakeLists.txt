set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests)
set(CTEST_BINARY_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set(MUNIT_DIR ${CMAKE_SOURCE_DIR}/tests/munit)
set(FIXTURES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
set(TEMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/tmp)

# Build munit+util static library
add_library(munit ${MUNIT_DIR}/munit.c)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(munit PRIVATE MUNIT_NO_FORK)
endif()

add_library(util util.c)
target_compile_definitions(util PRIVATE
    REFERENCE_FILES_DIR=\"${FIXTURES_DIR}\"
    FIXTURES_DIR=\"${FIXTURES_DIR}\"
    TEMP_DIR=\"${TEMP_DIR}\"
    _GNU_SOURCE
)
target_include_directories(util PRIVATE
    ${CMAKE_SOURCE_DIR}/tests
    ${LIBASDF_INCLUDE_DIRS}
)

# test-gwcs
add_executable(test-gwcs test-gwcs.c)
target_compile_definitions(test-gwcs PRIVATE
    REFERENCE_FILES_DIR=\"${FIXTURES_DIR}\"
    FIXTURES_DIR=\"${FIXTURES_DIR}\"
    TEMP_DIR=\"${TEMP_DIR}\"
    _GNU_SOURCE
)
target_include_directories(test-gwcs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
    ${MUNIT_DIR}
    ${LIBASDF_INCLUDE_DIRS}
)
target_link_libraries(test-gwcs PRIVATE
    libasdf-gwcs
    munit
    util
    ${LIBASDF_LIBRARIES}
)
target_link_directories(test-gwcs PRIVATE ${LIBASDF_LIBRARY_DIRS})

add_test(NAME test-gwcs COMMAND test-gwcs)
set_tests_properties(test-gwcs PROPERTIES
    TIMEOUT 240
    SKIP_RETURN_CODE 127
    ENVIRONMENT_MODIFICATION
        "LD_LIBRARY_PATH=path_list_prepend:${CMAKE_BINARY_DIR}/src;LD_LIBRARY_PATH=path_list_prepend:${LIBASDF_LIBRARY_DIRS}"
)
