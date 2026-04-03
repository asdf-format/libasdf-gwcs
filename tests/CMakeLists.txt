set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests)
set(CTEST_BINARY_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set(FIXTURES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
set(TEMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/tmp)
set(nix_gnu_cflags -Wall -Wextra)
set(nix_clang_cflags -Wall -Wextra)
set(win_msvc_cflags /Wall)

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

# GWCS tests
file(GLOB source_files "test-*.c")
set(ext_pattern "(^.*/|\\.[^.]*$)")

foreach(source_file ${source_files})
    string(REGEX REPLACE ${ext_pattern} "" test_executable ${source_file})
    add_executable(${test_executable} ${source_file})
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${test_executable} PRIVATE ${nix_cflags} ${nix_gnu_cflags})
    elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(${test_executable} PRIVATE ${nix_cflags} ${nix_clang_cflags})
    elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${test_executable} PRIVATE ${win_cflags} ${win_msvc_cflags})
    endif()

    target_compile_definitions(${test_executable} PRIVATE
        REFERENCE_FILES_DIR=\"${FIXTURES_DIR}\"
        FIXTURES_DIR=\"${FIXTURES_DIR}\"
        TEMP_DIR=\"${TEMP_DIR}\"
        _GNU_SOURCE
    )

    target_include_directories(${test_executable} SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
        ${MUNIT_DIR}
        ${LIBASDF_INCLUDE_DIRS}
    )

    target_link_libraries(${test_executable} PRIVATE
        libasdf-gwcs
        munit
        util
        ${LIBASDF_LIBRARIES}
    )

    add_test(${test_executable} ${test_executable})
    set_tests_properties(${test_executable}
        PROPERTIES TIMEOUT 240)
    set_tests_properties(${test_executable}
        PROPERTIES
        SKIP_RETURN_CODE 127)

    set(ENV{OBJC_DISABLE_INITIALIZE_FORK_SAFETY} "YES")
    set_property(TEST ${test_executable}
        PROPERTY ENVIRONMENT_MODIFICATION "${runtime}")
endforeach()
