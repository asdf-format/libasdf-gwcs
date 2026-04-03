install(
    FILES
        asdf/gwcs/core.h
        asdf/gwcs/fitswcs_imaging.h
        asdf/gwcs/frame.h
        asdf/gwcs/frame2d.h
        asdf/gwcs/frame_celestial.h
        asdf/gwcs/gwcs.h
        asdf/gwcs/step.h
        asdf/gwcs/transform.h
        asdf/gwcs/wcs.h

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/asdf/gwcs"
)

install(
    FILES
        asdf/gwcs/transforms/property/bounding_box.h

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/asdf/gwcs/transforms/property"
)
