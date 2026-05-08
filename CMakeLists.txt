cmake_minimum_required(VERSION 3.21)

# Internal full package version
set(PACKAGE_VERSION 0.1.0)

# CMake-compatible project version
set(PROJECT_VERSION 0.1.0)

set(PACKAGE_URL https://github.com/asdf-format/libasdf-gwcs)
set(PACKAGE_BUGREPORT https://github.com/asdf-format/libasdf-gwcs/issues)

# Shared library compatibility version
set(PROJECT_SOVERSION 0.0.0)

project(libasdf-gwcs
    VERSION ${PROJECT_VERSION}
    LANGUAGES C
)

set(PACKAGE_NAME "${PROJECT_NAME}")

# Global includes
include(GNUInstallDirs)
include(FindPkgConfig)

# Local includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SubmoduleInit)

set(STC_DIR ${CMAKE_SOURCE_DIR}/third_party/STC)
set(MUNIT_DIR ${CMAKE_SOURCE_DIR}/tests/munit)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# Update git submodules
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    list(APPEND submodules
        third_party/STC
        tests/munit
    )
    submodule_init("${submodules}")
endif()

include(ASDFGWCSOptions)

add_compile_options(-fvisibility=hidden)
add_compile_definitions(HAVE_CONFIG_H)
add_compile_definitions(ASDF_LOG_ENABLED)

# Find libasdf via pkg-config
pkg_check_modules(LIBASDF libasdf REQUIRED)

# Generate config.h
configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake
    ${CMAKE_BINARY_DIR}/include/config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/include)

if(ASDF_GWCS_ENABLE_AST)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(YAML REQUIRED yaml-0.1)

    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/ast/.git")
        include(ExternalProject)
        set(AST_INSTALL_DIR ${CMAKE_BINARY_DIR}/ast-install)
        ExternalProject_Add(ast_vendored
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/ast
            CONFIGURE_COMMAND
                sh -c "cd <SOURCE_DIR> && [ -f configure ] || ./bootstrap.local"
                COMMAND <SOURCE_DIR>/configure
                    --prefix=<INSTALL_DIR>
                    --disable-shared
                    --enable-static
                    "CFLAGS=-fPIC"
            BUILD_COMMAND make -j${CMAKE_BUILD_PARALLEL_LEVEL}
            INSTALL_DIR ${AST_INSTALL_DIR}
            BUILD_IN_SOURCE TRUE
        )
        set(AST_INCLUDE_DIR ${AST_INSTALL_DIR}/include)
        set(AST_LIBRARY ${AST_INSTALL_DIR}/lib/libast.a)
    else()
        find_library(AST_LIBRARY NAMES ast REQUIRED)
        find_path(AST_INCLUDE_DIR NAMES ast.h REQUIRED)
    endif()
endif()

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(include)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Generate and install pkg-config file
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir ${CMAKE_INSTALL_FULL_LIBDIR})
set(includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})
configure_file(${CMAKE_SOURCE_DIR}/libasdf-gwcs.pc.in
    ${CMAKE_BINARY_DIR}/libasdf-gwcs.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libasdf-gwcs.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(
    FILES
        LICENSE
        CHANGES.rst
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)
