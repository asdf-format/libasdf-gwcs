project(stc)
add_library(stc STATIC
    ${STC_DIR}/src/fmt.c
    ${STC_DIR}/src/csview.c
    ${STC_DIR}/src/cstr_io.c
    ${STC_DIR}/src/cspan.c
    ${STC_DIR}/src/cstr_core.c
    ${STC_DIR}/src/cstr_utf8.c
    ${STC_DIR}/src/random.c
    ${STC_DIR}/src/cregex.c
    ${STC_DIR}/src/stc_core.c
)
set_property(TARGET stc PROPERTY POSITION_INDEPENDENT_CODE 1)
# libasdf vendors its own copy of STC, and if these symbols are exported the
# dynamic linker binds libasdf's cregex calls to the copy here, so libasdf
# compiles regexes with one STC and matches them with another (how this was
# noticed).  More generally, shouldn't export those symbols.
# See also https://github.com/asdf-format/libasdf/pull/247
# which fixes this on the libasdf side
set_property(TARGET stc PROPERTY C_VISIBILITY_PRESET hidden)
target_link_libraries(stc PUBLIC m)
target_include_directories(stc PRIVATE
        STC/include
)
install(
    FILES STC/LICENSE
    RENAME LICENSE_STC.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}
)

# AST is LGPL-3.0-or-later and is statically linked into libasdf-gwcs, so its
# license text must accompany any binary distribution (LGPLv3 section 4b).
if (ASDF_GWCS_ENABLE_AST AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ast/COPYING)
    install(
        FILES ast/COPYING
        RENAME LICENSE_AST.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}
    )
endif()
