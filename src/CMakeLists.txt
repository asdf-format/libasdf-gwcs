project(libasdf-gwcs)

set(libasdf_gwcs_sources
    fitswcs_imaging.c
    frame.c
    frame2d.c
    frame_celestial.c
    step.c
    transform.c
    transforms/property/bounding_box.c
    wcs.c
)

add_library(libasdf_gwcs_objs OBJECT ${libasdf_gwcs_sources})
set_property(TARGET libasdf_gwcs_objs PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(libasdf-gwcs SHARED $<TARGET_OBJECTS:libasdf_gwcs_objs>)

target_include_directories(libasdf_gwcs_objs PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
    ${STC_DIR}/include
    ${LIBASDF_INCLUDE_DIRS}
)

target_compile_options(libasdf_gwcs_objs PUBLIC ${LIBASDF_CFLAGS_OTHER})

# Make __FILE__ names relative to source root
target_compile_options(libasdf_gwcs_objs PRIVATE
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
)

target_link_libraries(libasdf-gwcs PUBLIC
    stc
    ${LIBASDF_LIBRARIES}
)
target_link_directories(libasdf-gwcs PUBLIC ${LIBASDF_LIBRARY_DIRS})

set_target_properties(libasdf-gwcs PROPERTIES
    SOVERSION ${PROJECT_SOVERSION}
    OUTPUT_NAME "asdf-gwcs"
)

install(TARGETS libasdf-gwcs
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
