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
        asdf/gwcs/transforms/affine.h
        asdf/gwcs/transforms/compose.h
        asdf/gwcs/transforms/concatenate.h
        asdf/gwcs/transforms/constant.h
        asdf/gwcs/transforms/divide.h
        asdf/gwcs/transforms/identity.h
        asdf/gwcs/transforms/polynomial.h
        asdf/gwcs/transforms/remap_axes.h
        asdf/gwcs/transforms/rotate_sequence_3d.h
        asdf/gwcs/transforms/scale.h
        asdf/gwcs/transforms/shift.h
        asdf/gwcs/transforms/spherical_cartesian.h

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/asdf/gwcs/transforms"
)

install(
    FILES
        asdf/gwcs/transforms/property/bounding_box.h

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/asdf/gwcs/transforms/property"
)
